/*
 * stubs/cxamemcpy.c - SAS/C compiler helper without sc.lib
 *
 * With OPTIMIZE, SAS/C turns bulk copy loops into calls to __CXAMEMCPY
 * (stock definition in LIB:sc.lib). Matcha / CClib apps must not link sc.lib.
 *
 * Matcha Source uses PARAMETERS=REGISTERS:
 *   a0 = dest, a1 = src, d0 = length
 * C identifier _CXAMEMCPY -> linker symbol __CXAMEMCPY.
 *
 * Non-overlapping copy via exec CopyMem / CopyMemQuick (same constraints
 * as libc memcpy).
 */

#include <exec/types.h>
#include <proto/exec.h>

#ifdef __SASC
void __asm _CXAMEMCPY(
	register __a0 UBYTE *dest,
	register __a1 UBYTE *src,
	register __d0 unsigned long n)
#else
void _CXAMEMCPY(UBYTE *dest, UBYTE *src, unsigned long n)
#endif
{
	if (n == 0UL || dest == (UBYTE *)NULL || src == (UBYTE *)NULL
	    || dest == src)
	{
		return;
	}

	if (((((ULONG)dest) | ((ULONG)src) | n) & 3UL) == 0UL)
		CopyMemQuick(src, dest, n);
	else
		CopyMem(src, dest, n);
}
