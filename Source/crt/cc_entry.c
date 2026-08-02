/*
 * crt/cc_entry.c - SAS/C / Lattice startup (historic cc.o role)
 *
 * SAS/C is Lattice C 6.x.  Historic CClib linked:
 *   cc.o + lcmain.o + app ... lcclib.lib, lcmieee.lib, lc.lib, amiga.lib
 * where cc.o was a patched Lattice c.o (WB reply and argv[0] prepend removed).
 *
 * This module is the NOSTARTUP replacement for that cc.o: install SysBase /
 * DOSBase, collect the CLI arg string (without prepending argv[0] — CClib
 * cli_parse does that), do NOT reply to the Workbench message (lcmain /
 * SetupSTDIO own that), then call _main(aptr) as Lattice did.
 *
 * Must be first on the SLINK line, or use ENTRY=_start.  Do not also link
 * sc:lib/c.o — stock c.o double-replies WB and duplicates argv[0].
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

/* Lattice / SAS/C: _main receives a NUL-terminated argument string. */
extern void _main(STRPTR aptr);

void
_start(void)
{
	struct Process *pr;
	STRPTR aptr;
	struct Library *dos;

	SysBase = *((struct ExecBase **)4);
	DOSBase = (struct DosLibrary *)NULL;
	aptr = (STRPTR)NULL;

	dos = OpenLibrary((STRPTR)"dos.library", 37);
	if (dos == (struct Library *)NULL)
		dos = OpenLibrary((STRPTR)"dos.library", 0);
	if (dos == (struct Library *)NULL)
		return;
	DOSBase = (struct DosLibrary *)dos;

	pr = (struct Process *)FindTask(NULL);
	/*
	 * GetArgStr() is the AmigaDOS 2+ form of the Lattice c.o argument
	 * string: arguments only, no command name (CClib adds argv[0]).
	 */
	if (pr->pr_CLI != (BPTR)NULL
	    && ((struct Library *)DOSBase)->lib_Version >= 37)
		aptr = GetArgStr();

	_main(aptr);
}

void
__main(void)
{
	_start();
}
