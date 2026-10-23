/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_break.c - private CheckBreak / SignalBreak LVOs
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <private/cc_break.h>
#include "cctest_util.h"

void
ct_suite_break(void)
{
	ULONG pending;
	ULONG got;
	LONG hits;

	ct_suite_begin((STRPTR)"break",
		(STRPTR)"CheckBreak / SignalBreak (CC_BREAK_MASK)");

	/* Ensure no leftover break bits before soft-poll tests. */
	SetSignal(0L, SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D |
		SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F);

	pending = CheckBreak();
	ct_check_ulong((STRPTR)"break", (STRPTR)"CheckBreak_clear",
		(STRPTR)"CheckBreak() with no bits",
		(STRPTR)"Idle CheckBreak must return 0",
		0UL, pending);

	/*
	 * Soft path: abort disarmed.  SignalBreak asserts bits; CheckBreak
	 * must report them and leave them set (no abort_func).
	 */
	ClearAbortFunc();
	SignalBreak(SIGBREAKF_CTRL_D);
	got = SetSignal(0L, 0L) & SIGBREAKF_CTRL_D;
	ct_check_true((STRPTR)"break", (STRPTR)"SignalBreak_sets",
		(STRPTR)"SignalBreak(CTRL_D)",
		(STRPTR)"SignalBreak must assert the requested break bit",
		got != 0UL);

	pending = CheckBreak();
	ct_check_ulong((STRPTR)"break", (STRPTR)"CheckBreak_soft",
		(STRPTR)"CheckBreak() with CTRL_D, abort disarmed",
		(STRPTR)"Disarmed CheckBreak returns pending bits",
		(ULONG)SIGBREAKF_CTRL_D, pending);

	got = SetSignal(0L, 0L) & SIGBREAKF_CTRL_D;
	ct_check_true((STRPTR)"break", (STRPTR)"CheckBreak_soft_keeps",
		(STRPTR)"CTRL_D still set after soft CheckBreak",
		(STRPTR)"Disarmed CheckBreak must not clear break bits",
		got != 0UL);

	/* Consume leftover soft bits before arming the panic hook. */
	SetSignal(0L, SIGBREAKF_CTRL_D);

	/*
	 * Armed path: returning panic hook.  CheckBreak clears bits and
	 * invokes abort_func without killing the process.
	 */
	ct_install_panic_hook();
	hits = ct_panic_hits;
	SignalBreak(SIGBREAKF_CTRL_C);
	pending = CheckBreak();
	ct_check_ulong((STRPTR)"break", (STRPTR)"CheckBreak_armed_bits",
		(STRPTR)"CheckBreak() after SignalBreak(CTRL_C)",
		(STRPTR)"Armed CheckBreak returns the bits that were pending",
		(ULONG)SIGBREAKF_CTRL_C, pending);
	ct_check_long((STRPTR)"break", (STRPTR)"CheckBreak_armed_hook",
		(STRPTR)"ct_panic_hits after CheckBreak",
		(STRPTR)"Armed CheckBreak must call SetAbortFunc hook once",
		hits + 1L, ct_panic_hits);

	got = SetSignal(0L, 0L) & SIGBREAKF_CTRL_C;
	ct_check_true((STRPTR)"break", (STRPTR)"CheckBreak_armed_clears",
		(STRPTR)"CTRL_C clear after armed CheckBreak",
		(STRPTR)"Armed CheckBreak must clear the consumed break bits",
		got == 0UL);

	/* Mask clipping: bits outside C..F must not be asserted. */
	SignalBreak(1UL); /* SIGBREAKB_CTRL_C is bit 12; bit 0 is unused here */
	got = SetSignal(0L, 0L) & 1UL;
	ct_check_true((STRPTR)"break", (STRPTR)"SignalBreak_clip",
		(STRPTR)"SignalBreak(1)",
		(STRPTR)"SignalBreak must ignore bits outside CC_BREAK_MASK",
		got == 0UL);

	ct_restore_panic_hook();
}
