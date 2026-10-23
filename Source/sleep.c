/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * sleep.c - sleep / usleep / nanosleep for cclib.library
 *
 * timer.device TR_ADDREQUEST (timer.doc).  Not realtime.library.
 *
 * OpenDevice + SendIO/Wait (not DoIO) so Ctrl-C can AbortIO.  Always
 * WaitIO once, then CloseDevice before DeleteIORequest/DeleteMsgPort.
 * Do not call amiga.lib TimeDelay from an LVO - it can clobber A6.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <devices/timer.h>
#include <dos/dos.h>

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* Interrupted by Ctrl-C after local cleanup (abort hook may return). */
#define CC_TIMER_INTR  (-2L)

static void
cc_timer_cleanup(struct TimeRequest *tr, struct MsgPort *mp, LONG opened)
{
	if (tr != (struct TimeRequest *)NULL)
	{
		if (opened)
			CloseDevice((struct IORequest *)tr);
		DeleteIORequest((struct IORequest *)tr);
	}
	if (mp != (struct MsgPort *)NULL)
	{
		/* Drop a stale port signal so it cannot wake a later Wait. */
		if (mp->mp_SigBit >= 0 && mp->mp_SigBit < 32)
			SetSignal(0L, 1UL << mp->mp_SigBit);
		while (GetMsg(mp) != (struct Message *)NULL)
			;
		DeleteMsgPort(mp);
	}
}

/*
 * Block until the timer fires or SIGBREAKF_CTRL_C.  On Ctrl-C: AbortIO,
 * tear down, then cc_raise_ctrl_c() (abort_func / CRT exit).
 * Returns 0 ok, device/setup io_Error, CC_TIMER_INTR if hook returned.
 */
static LONG
cc_timer_delay(ULONG unit, ULONG secs, ULONG micro)
{
	struct MsgPort *mp;
	struct TimeRequest *tr;
	ULONG sigs;
	LONG opened;
	LONG err;

	cc_check_break();

	mp = CreateMsgPort();
	if (mp == (struct MsgPort *)NULL)
		return -1L;

	tr = (struct TimeRequest *)CreateIORequest(mp, (ULONG)sizeof(*tr));
	if (tr == (struct TimeRequest *)NULL)
	{
		cc_timer_cleanup((struct TimeRequest *)NULL, mp, 0L);
		return -1L;
	}

	opened = 0L;
	err = OpenDevice((STRPTR)TIMERNAME, unit, (struct IORequest *)tr, 0UL);
	if (err != 0L)
	{
		cc_timer_cleanup(tr, mp, 0L);
		return err;
	}
	opened = 1L;

	tr->tr_node.io_Command = (UWORD)TR_ADDREQUEST;
	tr->tr_node.io_Flags = 0;
	tr->tr_time.tv_secs = secs;
	tr->tr_time.tv_micro = micro;

	SendIO((struct IORequest *)tr);
	sigs = Wait((1UL << mp->mp_SigBit) | SIGBREAKF_CTRL_C);

	/*
	 * One WaitIO after optional AbortIO - never WaitIO twice, never
	 * CloseDevice while the request is still pending.
	 */
	if (CheckIO((struct IORequest *)tr) == (struct IORequest *)NULL)
		AbortIO((struct IORequest *)tr);
	err = (LONG)WaitIO((struct IORequest *)tr);

	cc_timer_cleanup(tr, mp, opened);

	if (sigs & SIGBREAKF_CTRL_C)
	{
		/* Port/device are gone; Wait already cleared the break bit. */
		cc_raise_ctrl_c();
		return CC_TIMER_INTR;
	}

	return err;
}

ULONG __ASM__ __SAVE_DS__ sleep(
	__REG__(d0, ULONG seconds))
{
	LONG err;

	if (seconds == 0UL)
	{
		cc_check_break();
		return 0UL;
	}

	err = cc_timer_delay((ULONG)UNIT_VBLANK, seconds, 0UL);
	if (err == CC_TIMER_INTR)
		return seconds;
	if (err != 0L)
	{
		cc_set_errno(CC_EINVAL);
		return seconds;
	}
	return 0UL;
}

LONG __ASM__ __SAVE_DS__ usleep(
	__REG__(d0, ULONG usec))
{
	ULONG secs;
	ULONG micro;
	LONG err;

	if (usec == 0UL)
	{
		cc_check_break();
		return 0L;
	}

	secs = usec / 1000000UL;
	micro = usec % 1000000UL;

	err = cc_timer_delay((ULONG)UNIT_MICROHZ, secs, micro);
	if (err == CC_TIMER_INTR)
	{
		cc_set_errno(CC_EINTR);
		return -1L;
	}
	if (err != 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ nanosleep(
	__REG__(a0, APTR req),
	__REG__(a1, APTR rem))
{
	LONG *ts;
	ULONG secs;
	ULONG nsec;
	ULONG micro;
	LONG err;

	cc_check_break();

	ts = (LONG *)req;
	if (ts == (LONG *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	secs = (ULONG)ts[0];
	nsec = (ULONG)ts[1];
	if (nsec >= 1000000000UL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	micro = (nsec + 999UL) / 1000UL;
	if (micro >= 1000000UL)
	{
		secs++;
		micro -= 1000000UL;
	}

	if (secs == 0UL && micro == 0UL)
	{
		if (rem != (APTR)NULL)
		{
			((LONG *)rem)[0] = 0L;
			((LONG *)rem)[1] = 0L;
		}
		return 0L;
	}

	err = cc_timer_delay((ULONG)UNIT_MICROHZ, secs, micro);
	if (err == CC_TIMER_INTR)
	{
		if (rem != (APTR)NULL)
		{
			/* Remaining unknown after AbortIO; report full request. */
			((LONG *)rem)[0] = (LONG)secs;
			((LONG *)rem)[1] = (LONG)nsec;
		}
		cc_set_errno(CC_EINTR);
		return -1L;
	}
	if (rem != (APTR)NULL)
	{
		((LONG *)rem)[0] = 0L;
		((LONG *)rem)[1] = 0L;
	}
	if (err != 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	return 0L;
}
