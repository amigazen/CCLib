/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef CC_TRACE_H
#define CC_TRACE_H

#include <exec/types.h>

/*
 * CRT panic backtrace.  Always linked.  After the panic line it prints
 * "the call into CClib" then each "which was called from" down to
 * program start.  file:line from HUNK_DEBUG/'LINE'; names from
 * HUNK_SYMBOL (ADDSYM).  A load file with neither hunk is probed
 * once and then left alone (no tables, no AllocMem).
 * cc_trace_omit() hides a returning abort hook.
 */
void cc_trace_init(void);
void __saveds cc_trace_dump(void);
void cc_trace_clear(void);
void cc_trace_omit(APTR fn);

/* SetupSTDIO abort hook: dump, then CRT exit().  __stdargs matches
 * cc_ABORT_FUNC; __saveds so near data is valid when the library calls.
 */
void __stdargs __saveds cc_crt_abort(LONG code);

#endif
