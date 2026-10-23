/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * crt/cc_entry.c - SAS/C __main (sc_crt/_main.c role) for CClib clients
 *
 * Called from crt/cc_start.asm after A4/BSS setup.  Opens dos + utility,
 * then Lattice _main(aptr).  __stdargs matches sc_crt and asm -u xref __main.
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct Library *UtilityBase;

/*
 * Minimum stack in bytes.  cc_start.asm grows only when the task's real
 * bounds (tc_SPUpper-tc_SPLower) are smaller.  A $STACK: cookie or c:stack
 * that is already large enough is left alone — unlike sc_crt/c.a, which
 * StackSwap's every V36+ start down to cli_DefaultStack.
 */
unsigned long __stack = 8192;

extern void __stdargs _main(STRPTR aptr);

static void
cc_dos_msg(STRPTR s)
{
	BPTR out;
	LONG n;

	if (DOSBase == (struct DosLibrary *)NULL || s == (STRPTR)NULL)
		return;
	n = 0;
	while (s[n] != '\0')
		n++;
	out = Output();
	if (out != (BPTR)NULL)
		Write(out, s, n);
}

void __stdargs
__main(char *line)
{
	struct Process *pr;
	STRPTR aptr;
	struct Library *dos;
	struct Library *util;

	(void)line;

	SysBase = *((struct ExecBase **)4);
	DOSBase = (struct DosLibrary *)NULL;
	UtilityBase = (struct Library *)NULL;
	aptr = (STRPTR)NULL;

	dos = OpenLibrary((STRPTR)"dos.library", 37);
	if (dos == (struct Library *)NULL)
		dos = OpenLibrary((STRPTR)"dos.library", 0);
	if (dos == (struct Library *)NULL)
		return;
	DOSBase = (struct DosLibrary *)dos;

	util = OpenLibrary((STRPTR)"utility.library", 37);
	if (util == (struct Library *)NULL)
		util = OpenLibrary((STRPTR)"utility.library", 0);
	if (util == (struct Library *)NULL)
	{
		cc_dos_msg((STRPTR)"CCTest: cannot open utility.library\n");
		CloseLibrary(dos);
		DOSBase = (struct DosLibrary *)NULL;
		return;
	}
	UtilityBase = util;

	pr = (struct Process *)FindTask(NULL);
	if (pr->pr_CLI != (BPTR)NULL
	    && ((struct Library *)DOSBase)->lib_Version >= 37)
		aptr = GetArgStr();

	_main(aptr);
}
