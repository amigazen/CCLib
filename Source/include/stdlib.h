/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef STDLIB_H
#define STDLIB_H 1

/*
 * stdlib - allocation, conversion, env (LVOs via proto).
 * exit / abort / atexit are CRT, not library LVOs.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef STDDEF_H
#include <stddef.h>
#endif

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 20
#define RAND_MAX 32767

/* div_t / ldiv_t come from <sys/types.h> (also required by SFD). */

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#ifdef ANSIC
void exit(int);
void abort(void);
int atexit(void (*)(void));
#else
void exit();
void abort();
int atexit();
#endif
extern void (*exit_fcn)(void);

#define abs(X) (((X) < 0) ? -(X) : (X))
#define system(COMMAND) (Execute(COMMAND, 0L, 0L))

#endif /* STDLIB_H */
