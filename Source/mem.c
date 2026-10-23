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
	cc_check_break();

	return memcmp(b1, b2, len);
}

void __ASM__ __SAVE_DS__ bcopy(
	__REG__(a0, APTR src),
	__REG__(a1, APTR dst),
	__REG__(d0, LONG len))
{
	/* BSD arg order (src, dst); overlap-safe via memmove. */
	cc_check_break();

	(void)memmove(dst, src, len);
}

void __ASM__ __SAVE_DS__ bzero(
	__REG__(a0, APTR b),
	__REG__(d0, LONG len))
{
	cc_check_break();
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
	cc_check_break();
	if (n > 0L && (!cc_buf_arg(dst, (ULONG)n) || !cc_buf_arg(src, (ULONG)n)))
		return dst;

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
	if (((((ULONG)dst) | ((ULONG)src) | (ULONG)n) & 3UL) == 0UL)
		CopyMemQuick(src, dst, (ULONG)n);
	else
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
	cc_check_break();
	if (n > 0L && (!cc_buf_arg(dst, (ULONG)n) || !cc_buf_arg(src, (ULONG)n)))
		return (APTR)NULL;

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
	cc_check_break();
	if (n > 0L && !cc_buf_arg(b, (ULONG)n))
		return (APTR)NULL;

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
	cc_check_break();
	if (n > 0L && (!cc_buf_arg(b1, (ULONG)n) || !cc_buf_arg(b2, (ULONG)n)))
		return 0L;

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
	cc_check_break();
	if (n > 0L && (!cc_buf_arg(dst, (ULONG)n) || !cc_buf_arg(src, (ULONG)n)))
		return dst;
	if (n > 0 && dst != src) {
		/* CopyMemQuick when src, dst, and size are all longword-aligned. */
		if (((((ULONG)dst) | ((ULONG)src) | (ULONG)n) & 3UL) == 0UL)
			CopyMemQuick(src, dst, (ULONG)n);
		else
			CopyMem(src, dst, (ULONG)n);
	}
	return dst;
}

APTR __ASM__ __SAVE_DS__ memmem(
	__REG__(a0, APTR haystack),
	__REG__(d0, LONG hlen),
	__REG__(a1, APTR needle),
	__REG__(d1, LONG nlen))
{
	UBYTE *h;
	UBYTE *n;
	LONG i;
	LONG j;

	/* Pure C byte search (like strstr over a counted region). */
	cc_check_break();

	if (haystack == (APTR)NULL || needle == (APTR)NULL
	    || hlen < 0L || nlen < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return (APTR)NULL;
	}
	if ((hlen > 0L && !cc_buf_mapped(haystack, (ULONG)hlen))
	    || (nlen > 0L && !cc_buf_mapped(needle, (ULONG)nlen)))
		return (APTR)NULL;
	if (nlen == 0L)
		return haystack;
	if (nlen > hlen)
		return (APTR)NULL;

	h = (UBYTE *)haystack;
	n = (UBYTE *)needle;
	i = 0L;
	while (i <= hlen - nlen)
	{
		j = 0L;
		while (j < nlen && h[i + j] == n[j])
			j++;
		if (j == nlen)
			return (APTR)(h + i);
		i++;
	}
	return (APTR)NULL;
}

APTR __ASM__ __SAVE_DS__ memset(
	__REG__(a0, APTR b),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n))
{
	UBYTE *p;
	UBYTE ch;
	ULONG fill;
	ULONG seed;
	ULONG left;
	ULONG half;

	/*
	 * Fast paths (see docs/LIBC_TO_AMIGA.md - memset strategies):
	 *   align + ULONG stores for small/medium blocks;
	 *   seed one longword then CopyMemQuick/CopyMem doubling for large fills.
	 * BltClear is chip/blitter-only and needs graphics.library - not used
	 * in general libc; call BltClear directly for large chip buffers.
	 */
	cc_check_break();
	if (n > 0L && !cc_buf_arg(b, (ULONG)n))
		return b;

	if (n <= 0L)
		return b;

	p = (UBYTE *)b;
	ch = (UBYTE)c;

	while (n > 0L && (((ULONG)p) & 3UL) != 0UL) {
		*p = ch;
		p++;
		n--;
	}

	if (n >= 4L) {
		fill = (ULONG)ch;
		fill |= (fill << 8);
		fill |= (fill << 16);

		if (n >= 64L) {
			*(ULONG *)p = fill;
			seed = 4UL;
			left = (ULONG)n;
			/* p is long-aligned; seed doubles from 4 -> always CopyMemQuick-legal. */
			while ((seed << 1) <= left) {
				half = seed;
				CopyMemQuick(p, p + half, half);
				seed = half << 1;
			}
			if (seed < left) {
				left -= seed;
				if ((left & 3UL) == 0UL)
					CopyMemQuick(p, p + seed, left);
				else
					CopyMem(p, p + seed, left);
			}
			return b;
		}

		while (n >= 4L) {
			*(ULONG *)p = fill;
			p += 4;
			n -= 4L;
		}
	}

	while (n > 0L) {
		*p = ch;
		p++;
		n--;
	}
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
	cc_check_break();
	if (n > 0L && (!cc_buf_arg(b1, (ULONG)n) || !cc_buf_arg(b2, (ULONG)n)))
		return;

	p1 = (UBYTE *)b1;
	p2 = (UBYTE *)b2;
	for (i = 0; i < n; i++) {
		tmp = p1[i];
		p1[i] = p2[i];
		p2[i] = tmp;
	}
}
