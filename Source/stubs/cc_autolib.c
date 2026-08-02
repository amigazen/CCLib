/*
 * stubs/cc_autolib.c - auto-open glue for cclib.library
 *
 * Defines CCLibBase and opens/closes the shared library.  Clients include
 * <proto/cclib.h> for #pragma libcall.  Optional FDToLib stubs from the SFD
 * provide stack-callable wrappers when pragmas are unavailable.
 */

#include <exec/types.h>
#include <proto/exec.h>

#include "private/cc_build.h"

struct Library *CCLibBase;

LONG
INIT_3_CCLib(void)
{
	CCLibBase = OpenLibrary(CCLIBNAME, 4);
	if (CCLibBase == (struct Library *)NULL)
		return 0;
	return 1;
}

void
EXIT_3_CCLib(void)
{
	if (CCLibBase != (struct Library *)NULL)
	{
		CloseLibrary(CCLibBase);
		CCLibBase = (struct Library *)NULL;
	}
}

#ifdef __SASC
LONG __stdargs _STI_3000_CCLib(void)
{
	return INIT_3_CCLib();
}

void __stdargs _STD_3000_CCLib(void)
{
	EXIT_3_CCLib();
}
#endif
