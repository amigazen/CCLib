/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cc_util.c - task userdata helpers and ctype classification table
 */

#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* ctype bit flags - must match archive ctype.h macros (type[(X)+1] & mask) */
#define CC_U 0x01 /* upper */
#define CC_L 0x02 /* lower */
#define CC_D 0x04 /* digit */
#define CC_X 0x08 /* xdigit */
#define CC_S 0x10 /* space */
#define CC_C 0x20 /* cntrl */
#define CC_P 0x40 /* punct */

/*
 * Index style: tab[0] is EOF sentinel; tab[c+1] classifies unsigned char c.
 * SetupSTDIO may hand apps a pointer to this table as the ctype "type" array.
 * Shared library resident - READ-ONLY for all tasks (CClib.Doc section 4).
 */
UBYTE cc_ctype_tab[257] = {
	0, /* EOF / index 0 */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /*   0..7   */
	0x20, 0xB0, 0x30, 0x30, 0x30, 0x30, 0x20, 0x20, /*   8..15  */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /*  16..23  */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /*  24..31  */
	0x90, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /*  32..39  */
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /*  40..47  */
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, /*  48..55  */
	0x0C, 0x0C, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /*  56..63  */
	0x40, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x01, /*  64..71  */
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /*  72..79  */
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /*  80..87  */
	0x01, 0x01, 0x01, 0x40, 0x40, 0x40, 0x40, 0x40, /*  88..95  */
	0x40, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x02, /*  96..103 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 104..111 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 112..119 */
	0x02, 0x02, 0x02, 0x40, 0x40, 0x40, 0x40, 0x20, /* 120..127 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 128..135 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 136..143 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 144..151 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 152..159 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 160..167 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 168..175 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 176..183 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 184..191 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 192..199 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 200..207 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 208..215 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 216..223 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 224..231 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 232..239 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 240..247 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 248..255 */
};

/*
 * Invoke the session abort hook for Ctrl-C (CRT exit).  Used after Wait()
 * has already cleared SIGBREAKF_CTRL_C from the task's received mask.
 * No-op when abort_func is NULL (ClearAbortFunc / teardown).
 */
void
cc_raise_ctrl_c(void)
{
	struct Task *t;
	cc_task_UserData *ud;

	t = FindTask(NULL);
	if (t == (struct Task *)NULL)
		return;
	ud = (cc_task_UserData *)t->tc_UserData;
	if (ud == (cc_task_UserData *)NULL
	    || ud->abort_func == (cc_ABORT_FUNC)NULL)
		return;
	(*ud->abort_func)(20L);
}

/*
 * Shared break poll for LVO entry and the CheckBreak LVO.  Watches
 * CC_BREAK_MASK (Ctrl-C..F).  If abort_func is armed, clears the pending
 * bits and calls abort_func(20).  If disarmed, leaves the bits set and
 * returns them for soft handling.
 */
ULONG
cc_poll_break(void)
{
	ULONG pending;
	struct Task *t;
	cc_task_UserData *ud;

	pending = SetSignal(0L, 0L) & CC_BREAK_MASK;
	if (pending == 0UL)
		return 0UL;

	t = FindTask(NULL);
	if (t == (struct Task *)NULL)
		return pending;
	ud = (cc_task_UserData *)t->tc_UserData;
	if (ud == (cc_task_UserData *)NULL
	    || ud->abort_func == (cc_ABORT_FUNC)NULL)
		return pending;

	SetSignal(0L, pending);
	(*ud->abort_func)(20L);
	return pending;
}

/*
 * Poll CC_BREAK_MASK and jump to the session abort hook (CRT exit).
 * If abort_func is NULL, leave the signal set so a later SetAbortFunc
 * still sees the break (do not silently eat breaks after ClearAbortFunc).
 * ClearSTDIO nulls abort_func before teardown so this cannot re-enter.
 *
 * Workers must not leave a ThreadPair in tc_UserData across CClib LVOs
 * (clear it before SetupSTDIO); this path assumes UserData is a session
 * or NULL.
 */
void
cc_check_break(void)
{
	(void)cc_poll_break();
}

/*
 * Private LVO: companion-callable break poll (parm=r libcall).
 * Same policy as cc_check_break; returns pending mask bits.
 */
ULONG __ASM__ __SAVE_DS__ CheckBreak(void)
{
	return cc_poll_break();
}

/*
 * Private LVO: assert break bits on this task (tests / inject).
 * Mask is clipped to CC_BREAK_MASK.
 */
void __ASM__ __SAVE_DS__ SignalBreak(
	__REG__(d0, ULONG mask))
{
	mask &= CC_BREAK_MASK;
	if (mask != 0UL)
		SetSignal(mask, mask);
}

cc_task_UserData *cc_get_ud(void)
{
	struct Task *t;

	/* Most LVOs fetch ud on entry - that is the Ctrl-C checkpoint. */
	cc_check_break();

	t = FindTask(NULL);
	if (!t)
		return NULL;
	return (cc_task_UserData *)t->tc_UserData;
}

void cc_set_errno(LONG err)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud && ud->errno_p)
		*ud->errno_p = err;
}

/*
 * Decimal LONG -> ASCII for console breadcrumbs.  buf must hold at least
 * 12 bytes (sign + 10 digits + NUL).
 */
static void
cc_fmt_long(LONG v, char *buf)
{
	ULONG u;
	char tmp[12];
	LONG i;
	LONG j;

	if (v < 0L)
	{
		buf[0] = '-';
		u = (ULONG)(-v);
		j = 1L;
	}
	else
	{
		u = (ULONG)v;
		j = 0L;
	}

	i = 0L;
	do
	{
		tmp[i] = (char)('0' + (u % 10UL));
		u /= 10UL;
		i++;
	} while (u != 0UL && i < 11L);

	while (i > 0L)
	{
		i--;
		buf[j] = tmp[i];
		j++;
	}
	buf[j] = '\0';
}

/* Raw console write - stdio may already be corrupt during panic. */
static void
cc_panic_msg(STRPTR msg)
{
	BPTR out;
	LONG n;

	if (DOSBase == (struct DosLibrary *)NULL || msg == (STRPTR)NULL)
		return;
	n = 0L;
	while (msg[n] != '\0')
		n++;
	out = Output();
	if (out != (BPTR)NULL)
		(void)Write(out, msg, n);
}

/*
 * Abnormal session teardown entry.  Prefer ud->abort_func (CRT sets this
 * to exit, which runs atexit handlers then ClearSTDIO).  Apps register
 * their own cleanup with atexit() - no separate library cleanup list.
 * If there is no hook, reap here and Exit so resources cannot leak.
 *
 * panicking is library-global: clear it before abort_func/Exit so a
 * shared library image is not left latched for the next client process.
 *
 * Status for the Shell: SetIoErr -> Result2 / WHY; Exit(code) -> RC.
 * Never SetVar("RC") / SetVar("Result2") while still inside RunCommand.
 */
void
cc_panic(LONG code)
{
	cc_task_UserData *ud;
	cc_ABORT_FUNC hook;
	char cbuf[12];
	static LONG panicking;

	if (panicking)
	{
		/*
		 * Nested panic: do not re-enter abort_func.  Clear the latch
		 * before Exit so the shared library stays reusable.
		 */
		panicking = 0;
		if (DOSBase != (struct DosLibrary *)NULL)
		{
			SetIoErr(ERROR_ACTION_NOT_KNOWN);
			Exit(code);
		}
		return;
	}
	panicking = 1;

	/* Secondary result for WHY; primary RC comes from Exit(code). */
	if (DOSBase != (struct DosLibrary *)NULL)
		SetIoErr(ERROR_ACTION_NOT_KNOWN);

	cc_panic_msg((STRPTR)"CClib: panic - invalid capability or programming error (code ");
	cc_fmt_long(code, cbuf);
	cc_panic_msg(cbuf);
	cc_panic_msg((STRPTR)")\n");

	/*
	 * Resolve ud without cc_get_ud(): that polls Ctrl-C and could
	 * recurse into abort_func while panicking is still set.
	 */
	ud = (cc_task_UserData *)NULL;
	{
		struct Task *t;

		t = FindTask((STRPTR)NULL);
		if (t != (struct Task *)NULL)
			ud = (cc_task_UserData *)t->tc_UserData;
	}

	hook = (cc_ABORT_FUNC)NULL;
	if (ud != (cc_task_UserData *)NULL)
	{
		hook = ud->abort_func;
		/* Stack dump is CRT-owned and must run even when abort_func
		 * is a returning test hook (CCPanic).
		 */
		if (ud->trace_dump != (void (*)(void))NULL)
			(*ud->trace_dump)();
	}

	/* Drop latch before hook: production exit() never returns. */
	panicking = 0;

	if (hook != (cc_ABORT_FUNC)NULL)
	{
		(*hook)(code);
		/* Production exit() does not return.  A test hook may return. */
		return;
	}

	/*
	 * CC_PANIC_HOOK with no abort_func: report already done; do not
	 * ClearSTDIO/Exit (servers / harnesses that refuse libc Exit).
	 */
	if (ud != (cc_task_UserData *)NULL && ud->panic_mode == 1)
		return;

	ClearSTDIO();
	if (DOSBase != (struct DosLibrary *)NULL)
		Exit(code);
}

LONG cc_get_errno(void)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud && ud->errno_p)
		return *ud->errno_p;
	return 0;
}

/* True if data payload pointer is a node on list (add_head layout). */
static int
cc_list_has_data(cc_list *list, APTR data)
{
	cc_node *n;

	if (list == (cc_list *)NULL || data == (APTR)NULL)
		return 0;
	n = list->first;
	while (n != (cc_node *)NULL)
	{
		if ((APTR)(n + 1) == data)
			return 1;
		n = n->next;
	}
	return 0;
}

/*
 * Resolve an open() fd (pointer-as-capability) against OpenFiles.
 * Sentinels (-1/0) -> EBADF.  Forged/stale pointers -> panic then EBADF.
 */
cc_FileDesc *
cc_fd_from_list(cc_task_UserData *ud, LONG fd)
{
	cc_FileDesc *desc;

	if (ud == (cc_task_UserData *)NULL || fd == 0L || fd == -1L)
	{
		cc_set_errno(CC_EBADF);
		return (cc_FileDesc *)NULL;
	}
	desc = (cc_FileDesc *)fd;
	if (!cc_list_has_data(&ud->OpenFiles, (APTR)desc)
	    || desc->magic != CC_FD_MAGIC)
	{
		cc_set_errno(CC_EBADF);
		cc_panic(20L);
		return (cc_FileDesc *)NULL;
	}
	return desc;
}

/*
 * Resolve a FILE* against StreamFiles.  NULL -> EINVAL.
 * Forged/stale streams -> panic then EBADF.
 */
cc_FILE *
cc_file_from_list(cc_task_UserData *ud, APTR stream)
{
	cc_FILE *fp;

	if (ud == (cc_task_UserData *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (cc_FILE *)NULL;
	}
	if (stream == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (cc_FILE *)NULL;
	}
	fp = (cc_FILE *)stream;
	if (!cc_list_has_data(&ud->StreamFiles, (APTR)fp)
	    || fp->magic != CC_FP_MAGIC)
	{
		cc_set_errno(CC_EBADF);
		cc_panic(20L);
		return (cc_FILE *)NULL;
	}
	return fp;
}

/*
 * Resolve a DIR* against ud->open_dirs.  NULL -> EBADF (soft).
 * Forged/stale cookies -> panic then EBADF.
 */
cc_DIR *
cc_dir_from_list(cc_task_UserData *ud, APTR dirp)
{
	cc_DIR *dir;
	cc_DIR *p;

	if (ud == (cc_task_UserData *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (cc_DIR *)NULL;
	}
	if (dirp == (APTR)NULL)
	{
		cc_set_errno(CC_EBADF);
		return (cc_DIR *)NULL;
	}
	dir = (cc_DIR *)dirp;
	p = (cc_DIR *)ud->open_dirs;
	while (p != (cc_DIR *)NULL)
	{
		if (p == dir)
		{
			if (dir->magic != CC_DIR_MAGIC)
			{
				cc_set_errno(CC_EBADF);
				cc_panic(20L);
				return (cc_DIR *)NULL;
			}
			return dir;
		}
		p = p->next;
	}
	cc_set_errno(CC_EBADF);
	cc_panic(20L);
	return (cc_DIR *)NULL;
}

/*
 * malloc user pointer -> MemBlk if that arena is on ud->RAM.
 * Layout: [cc_MemBlk][ULONG nbytes][user...].
 */
cc_MemBlk *
cc_blk_from_malloc_ptr(cc_task_UserData *ud, APTR ptr)
{
	ULONG *hdr;
	cc_MemBlk *blk;
	cc_node *n;

	if (ud == (cc_task_UserData *)NULL || ptr == (APTR)NULL)
		return (cc_MemBlk *)NULL;

	hdr = ((ULONG *)ptr) - 1;
	blk = ((cc_MemBlk *)hdr) - 1;
	n = ud->RAM.first;
	while (n != (cc_node *)NULL)
	{
		if (n == &blk->n)
			return blk;
		n = n->next;
	}
	return (cc_MemBlk *)NULL;
}

LONG
cc_str_mapped(STRPTR s)
{
	LONG n;

	if (TypeOfMem((APTR)s) == 0UL)
	{
		cc_set_errno(CC_EINVAL);
		cc_panic(20L);
		return 0L;
	}
	n = 0L;
	while (n < CC_STR_PROBE && s[n] != '\0')
		n++;
	if (n == CC_STR_PROBE && s[n] != '\0')
	{
		cc_set_errno(CC_EINVAL);
		cc_panic(20L);
		return 0L;
	}
	return 1L;
}

LONG
cc_str_arg(STRPTR s)
{
	if (s == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return 0L;
	}
	return cc_str_mapped(s);
}

LONG
cc_buf_mapped(APTR p, ULONG n)
{
	if (TypeOfMem(p) == 0UL
	    || (n > 1UL && TypeOfMem((APTR)((UBYTE *)p + (n - 1UL))) == 0UL))
	{
		cc_set_errno(CC_EINVAL);
		cc_panic(20L);
		return 0L;
	}
	return 1L;
}

LONG
cc_buf_arg(APTR p, ULONG n)
{
	if (n == 0UL)
		return 1L;
	if (p == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return 0L;
	}
	return cc_buf_mapped(p, n);
}
