/*
 * cc_cxutil.c - SAS/C integer mul/div helpers via utility.library
 *
 * With nostartup and no LIB:sc.lib, SLINK does not get __CXM33 / __CXD33 /
 * __CXM22 / __CXD22 from the compiler runtime.  Those are the helpers the
 * compiler emits for 32-bit and 16-bit integer * and /.
 *
 * Map them to utility.library (opened in LibInit).  Calling convention is
 * the SAS/C register stub ABI: operands in d0/d1, result in d0.  Do not use
 * __SAVE_DS__ — callers already have A4 set, and A6 must stay the CClib
 * base across the JSR.
 */

#include <exec/types.h>
#include <proto/utility.h>
#include <clib/compiler-specific.h>

#include "private/cc_internal.h"

LONG __ASM__ __CXM33(__REG__(d0, LONG a), __REG__(d1, LONG b))
{
	return SMult32(a, b);
}

LONG __ASM__ __CXD33(__REG__(d0, LONG a), __REG__(d1, LONG b))
{
	/* Quotient in d0; remainder in d1 is unused by the compiler stub ABI. */
	return SDivMod32(a, b);
}

LONG __ASM__ __CXM22(__REG__(d0, LONG a), __REG__(d1, LONG b))
{
	return SMult32((LONG)((WORD)a), (LONG)((WORD)b));
}

LONG __ASM__ __CXD22(__REG__(d0, LONG a), __REG__(d1, LONG b))
{
	return SDivMod32((LONG)((WORD)a), (LONG)((WORD)b));
}
