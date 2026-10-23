/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef SETJMP_H
#define SETJMP_H 1

/*
 * setjmp/longjmp - implemented in the CClib stub library (not library LVOs).
 *
 * jmp_buf layout after setjmp:
 *   0  return address
 *   1..6  D2..D7
 *   7..9  A2..A4
 *  10  A5 frame pointer
 *  11  A6 library base pointer
 *  12  A7 stack pointer
 *
 * NOTE: this implementation does not fully comply with ANSI C regarding
 * automatic (non-register) locals across longjmp.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

typedef unsigned long jmp_buf[13];

#ifdef ANSIC
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int return_code);
#else
int setjmp();
void longjmp();
#endif

#endif
