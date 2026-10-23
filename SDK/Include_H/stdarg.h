/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef STDARG_H
#define STDARG_H 1

/*
 * stdarg - classic stack-based varargs for CClib stub/auto-lib UX.
 * Library LVOs for formatted I/O use register conventions via <proto/cclib.h>.
 */

typedef unsigned char *va_list;

#define va_start(ap, lastarg) ((ap) = (va_list)(&lastarg + 1))
#define va_arg(ap, type) ((ap) += sizeof(type), ((type *)(ap))[-1])
#define va_end(ap) ((ap) = 0L)

#endif
