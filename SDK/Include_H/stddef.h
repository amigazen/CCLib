/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef STDDEF_H
#define STDDEF_H 1

/*
 * stddef - common C types for CClib clients.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef __SIZE_T
#define __SIZE_T 1
typedef unsigned long size_t;
#endif

#ifndef __FPOS_T
#define __FPOS_T 1
typedef long fpos_t;
#endif

#ifndef __PTRDIFF_T
#define __PTRDIFF_T 1
typedef long ptrdiff_t;
#endif

#ifndef __WCHAR_T
#define __WCHAR_T
/* unsigned int so WEOF (-1) is distinct from every wchar_t value */
typedef unsigned int wchar_t;
#endif

#ifndef __WINT_T
#define __WINT_T
typedef int wint_t;
#endif

#ifndef __TIME_T
#define __TIME_T 1
typedef long time_t;
#endif

#ifndef __CLOCK_T
#define __CLOCK_T 1
typedef long clock_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef offsetof
#define offsetof(P,M) ((size_t)&((P *)NULL)->M)
#endif

#endif
