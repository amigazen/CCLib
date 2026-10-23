/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * Private companion LVOs for panic policy.
 *
 * Not in public pragmas (SFD ==private).  CRT / tests / servers that must
 * not Exit from libc include this for parm=r libcalls.  Requires CCLibBase
 * (include <proto/cclib.h> first).
 *
 * CC_PANIC_ABORT — default: call SetAbortFunc hook, else ClearSTDIO+Exit.
 * CC_PANIC_HOOK  — call hook if armed; never Exit from the library
 *                  (forged-handle detection still runs).
 */

#ifndef CC_PANIC_H
#define CC_PANIC_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif

#define CC_PANIC_ABORT 0
#define CC_PANIC_HOOK  1

/* Set session panic policy; returns previous mode (or ABORT if no session). */
LONG SetPanicMode(LONG mode);

/* Current session panic policy (ABORT if no session). */
LONG GetPanicMode(void);

#if defined(__SASC) || defined(LATTICE) || defined(_DCC)
#pragma libcall CCLibBase SetPanicMode 5A0 001
#pragma libcall CCLibBase GetPanicMode 5A6 00
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(CCLibBase, 0x5A0, SetPanicMode(d0))
#pragma amicall(CCLibBase, 0x5A6, GetPanicMode())
#endif

#endif /* CC_PANIC_H */
