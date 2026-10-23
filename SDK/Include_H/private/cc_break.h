/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * Private companion LVOs for AmigaDOS break flow control.
 *
 * Not in public pragmas (SFD ==private).  Static libs and CRT include this
 * for parm=r libcalls to CheckBreak / SignalBreak.  Requires CCLibBase
 * (include <proto/cclib.h> first).
 */

#ifndef CC_BREAK_H
#define CC_BREAK_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif

/*
 * Poll/clear CC_BREAK_MASK (Ctrl-C..F).  If SetAbortFunc is armed, invoke
 * it (may not return).  Returns the bits that were pending (0 if none).
 */
ULONG CheckBreak(void);

/* Assert break bits on this task (mask clipped to CC_BREAK_MASK). */
void SignalBreak(ULONG mask);

#if defined(__SASC) || defined(LATTICE) || defined(_DCC)
#pragma libcall CCLibBase CheckBreak 594 00
#pragma libcall CCLibBase SignalBreak 59A 001
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(CCLibBase, 0x594, CheckBreak())
#pragma amicall(CCLibBase, 0x59A, SignalBreak(d0))
#endif

#endif /* CC_BREAK_H */
