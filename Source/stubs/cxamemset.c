/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * stubs/cxamemset.c - SAS/C compiler helper without sc.lib
 *
 * With OPTIMIZE, SAS/C turns bulk memory fills (e.g. clear loops) into
 * calls to __CXAMEMSET. Stock definition lives in LIB:sc.lib; Matcha /
 * CClib apps must not link sc.lib (NOSTARTUP + CClib CRT).
 *
 * Matcha Source uses PARAMETERS=REGISTERS, so the helper is registerized:
 *   a0 = dest, d0 = fill byte, d1 = length
 * C identifier _CXAMEMSET -> linker symbol __CXAMEMSET.
 *
 * Fast path mirrors CClib memset: longword stores, then CopyMemQuick
 * doubling for large fills. BltClear is not used here (needs GfxBase and
 * chip/blitter-accessible memory only).
 */

#include <exec/types.h>
#include <proto/exec.h>

#ifdef __SASC
void __asm _CXAMEMSET(
	register __a0 UBYTE *dest,
	register __d0 int c,
	register __d1 unsigned long n)
#else
void _CXAMEMSET(UBYTE *dest, int c, unsigned long n)
#endif
{
	UBYTE *p;
	UBYTE ch;
	ULONG fill;
	ULONG seed;
	ULONG left;
	ULONG half;

	if (n == 0UL || dest == (UBYTE *)NULL)
		return;

	p = dest;
	ch = (UBYTE)c;

	while (n > 0UL && (((ULONG)p) & 3UL) != 0UL) {
		*p = ch;
		p++;
		n--;
	}

	if (n >= 4UL) {
		fill = (ULONG)ch;
		fill |= (fill << 8);
		fill |= (fill << 16);

		if (n >= 64UL) {
			*(ULONG *)p = fill;
			seed = 4UL;
			left = n;
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
			return;
		}

		while (n >= 4UL) {
			*(ULONG *)p = fill;
			p += 4;
			n -= 4UL;
		}
	}

	while (n > 0UL) {
		*p = ch;
		p++;
		n--;
	}
}
