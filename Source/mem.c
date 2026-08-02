/* SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED.
 *
 * mem.c - memory block LVOs for cclib.library
 */

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

LONG __ASM__ __SAVE_DS__ bcmp(
	__REG__(a0, APTR b1),
	__REG__(a1, APTR b2),
	__REG__(d0, LONG len))
{
	/* BSD name; same ordered compare as memcmp. */
	return memcmp(b1, b2, len);
}

void __ASM__ __SAVE_DS__ bcopy(
	__REG__(a0, APTR src),
	__REG__(a1, APTR dst),
	__REG__(d0, LONG len))
{
	/* BSD arg order (src, dst); overlap-safe via memmove. */
	(void)memmove(dst, src, len);
}

void __ASM__ __SAVE_DS__ bzero(
	__REG__(a0, APTR b),
	__REG__(d0, LONG len))
{
	(void)memset(b, 0, len);
}

APTR __ASM__ __SAVE_DS__ memmove(
	__REG__(a0, APTR dst),
	__REG__(a1, APTR src),
	__REG__(d0, LONG n))
{
	UBYTE *s;
	UBYTE *d;
	LONG i;

	/* C89 memmove: overlap-safe; CopyMem when regions do not overlap. */
	s = (UBYTE *)src;
	d = (UBYTE *)dst;
	if (n <= 0 || s == d)
		return dst;
	if (d > s && d < s + n) {
		for (i = n - 1; i >= 0; i--)
			d[i] = s[i];
		return dst;
	}
	if (s > d && s < d + n) {
		for (i = 0; i < n; i++)
			d[i] = s[i];
		return dst;
	}
	CopyMem(src, dst, (ULONG)n);
	return dst;
}

APTR __ASM__ __SAVE_DS__ memccpy(
	__REG__(a0, APTR dst),
	__REG__(a1, APTR src),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n))
{
	UBYTE *d;
	UBYTE *s;
	UBYTE stop;
	LONG i;

	d = (UBYTE *)dst;
	s = (UBYTE *)src;
	stop = (UBYTE)c;
	for (i = 0; i < n; i++) {
		d[i] = s[i];
		if (s[i] == stop)
			return (APTR)(d + i + 1);
	}
	return NULL;
}

APTR __ASM__ __SAVE_DS__ memchr(
	__REG__(a0, APTR b),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n))
{
	UBYTE *p;
	UBYTE ch;
	LONG i;

	p = (UBYTE *)b;
	ch = (UBYTE)c;
	for (i = 0; i < n; i++) {
		if (p[i] == ch)
			return (APTR)(p + i);
	}
	return NULL;
}

LONG __ASM__ __SAVE_DS__ memcmp(
	__REG__(a0, APTR b1),
	__REG__(a1, APTR b2),
	__REG__(d0, LONG n))
{
	UBYTE *p1;
	UBYTE *p2;
	LONG i;
	LONG c1;
	LONG c2;

	p1 = (UBYTE *)b1;
	p2 = (UBYTE *)b2;
	if (n <= 0 || p1 == p2)
		return 0;
	for (i = 0; i < n; i++) {
		c1 = (LONG)p1[i];
		c2 = (LONG)p2[i];
		if (c1 != c2)
			return c1 - c2;
	}
	return 0;
}

APTR __ASM__ __SAVE_DS__ memcpy(
	__REG__(a0, APTR dst),
	__REG__(a1, APTR src),
	__REG__(d0, LONG n))
{
	if (n > 0 && dst != src)
		CopyMem(src, dst, (ULONG)n);
	return dst;
}

APTR __ASM__ __SAVE_DS__ memset(
	__REG__(a0, APTR b),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n))
{
	UBYTE *p;
	UBYTE ch;
	LONG i;

	p = (UBYTE *)b;
	ch = (UBYTE)c;
	for (i = 0; i < n; i++)
		p[i] = ch;
	return b;
}

void __ASM__ __SAVE_DS__ memswap(
	__REG__(a0, APTR b1),
	__REG__(a1, APTR b2),
	__REG__(d0, LONG n))
{
	UBYTE *p1;
	UBYTE *p2;
	UBYTE tmp;
	LONG i;

	p1 = (UBYTE *)b1;
	p2 = (UBYTE *)b2;
	for (i = 0; i < n; i++) {
		tmp = p1[i];
		p1[i] = p2[i];
		p2[i] = tmp;
	}
}
