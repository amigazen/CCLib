/*
 * LibInit.c - ROMTag and version strings for cclib.library
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <clib/compiler-specific.h>

#include "private/cc_build.h"
#include "Rev.h"

const char AS_LibName[] = "CClib.library";
const char AS_LibId[] = "CClib.library " VERSION " (" DATE ")\r\n";

struct InitTable;
extern struct InitTable InitTab;
extern APTR __ASM__ __SAVE_DS__ LibExpunge(__REG__(a6, struct Library *base));

struct Resident RomTag = {
	RTC_MATCHWORD,
	&RomTag,
	LibExpunge,
	RTF_AUTOINIT,
	VERNUM,
	NT_LIBRARY,
	0,
	(APTR)AS_LibName,
	(APTR)AS_LibId,
	(APTR)&InitTab
};

#ifdef __SASC
void __regargs __chkabort(void) { }
void __regargs _CXBRK(void)     { }

/*
 * nostartup shared libs must not pull LIB:sc.lib.  Provide the CRT exit
 * hook SLINK looks for (same symbol Lattice/SAS/C c.o expects).
 */
void __stdargs __XCEXIT(long code) { (void)code; }
void __stdargs _exit(long code)    { (void)code; }
void __stdargs exit(long code)     { (void)code; }
#endif
