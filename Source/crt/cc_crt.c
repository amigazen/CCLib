/*
 * crt/cc_crt.c - Lattice/SAS/C _main body (historic lcmain.c) + Aztec symbols
 *
 * Port of archive Source/lcmain.c for SAS/C 6 (Lattice lineage), plus the
 * Aztec globals from _main.c so one CRT covers both historic surfaces.
 *
 * Lattice flow (CClib.Doc section IX):
 *   - Refuse stock c.o if WBenchMsg already set (would double-reply WB)
 *   - Take WB message ourselves; call __fpinit after that
 *   - Open CClib + IEEE math; SetupSTDIO; main(argc,argv)
 *   - Tear down via atexit handlers, ClearSTDIO; dos Exit
 *
 * With NOSTARTUP, crt/cc_start.asm replaces sc_crt/c.a (__startup, A4,
 * BSS clear) and this file replaces lcmain / sc_crt/_main.c.
 *
 * Termination policy (stronger than ISO _exit): every CRT path that leaves
 * the process - exit, _exit, abort, Ctrl-C - runs atexit handlers once and
 * then ClearSTDIO so heap, files, children, and scdir/getenv scratch cannot
 * leak when the library is closed.  dos Exit(code) supplies the Shell RC;
 * IoErr / SetIoErr supplies Result2.  Do not SetVar("RC")/"Result2" here -
 * that deadlocks under RunCommand.
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>

#include "/private/cc_build.h"
#include "/private/cc_internal.h"
#include "/private/cc_break.h"

extern short _math;
extern long blocksize;

extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct Library *UtilityBase;

struct Library *CCLibBase = NULL;
struct Library *MathIeeeDoubBasBase = NULL;
struct WBStartup *WBenchMsg = NULL;
void (*exit_fcn)(void) = NULL;

FILE *stdin = NULL;
FILE *stdout = NULL;
FILE *stderr = NULL;
int errno = 0;
char *type = NULL;

/* Aztec C stack / trap globals (archive _main.c) */
LONG _savsp = 0;
LONG _stkbase = 0;
void *_oldtrap = NULL;
void **_trapaddr = NULL;

static LONG cc_exiting;
static short cc_math_opened; /* Lattice _mathopen: we opened IEEE, must Close */

/* ISO atexit() - CRT-owned (not a library LVO). */
#define CC_ATEXIT_MAX 32
static void (*cc_atexit_fn[CC_ATEXIT_MAX])(void);
static LONG cc_atexit_n;
static LONG cc_atexit_ran;

extern int main(int argc, char **argv);
void __stdargs __saveds _exit(LONG code);
void __stdargs __saveds exit(LONG code);
void __stdargs __saveds abort(void);
int atexit(void (*func)(void));

/* Raw DOS write for optional CRT breadcrumbs (DEFINE=CC_CRT_DEBUG). */
#ifdef CC_CRT_DEBUG
static void cc_crt_msg(STRPTR msg)
{
	BPTR out;
	LONG n;

	if (DOSBase == (struct DosLibrary *)NULL || msg == (STRPTR)NULL)
		return;
	n = 0;
	while (msg[n] != '\0')
		n++;
	out = Output();
	if (out != (BPTR)NULL)
		Write(out, msg, n);
}
#define CC_CRT_DBG(s) cc_crt_msg((STRPTR)(s))
#else
#define CC_CRT_DBG(s) ((void)0)
#endif

static void cc_run_atexit(void)
{
	void (*f)(void);

	if (cc_atexit_ran)
		return;
	cc_atexit_ran = 1;
	while (cc_atexit_n > 0)
	{
		cc_atexit_n--;
		f = cc_atexit_fn[cc_atexit_n];
		cc_atexit_fn[cc_atexit_n] = (void (*)(void))NULL;
		if (f != (void (*)(void))NULL)
			(*f)();
	}
}

int
atexit(void (*func)(void))
{
	if (func == (void (*)(void))NULL)
		return -1;
	if (cc_atexit_n >= CC_ATEXIT_MAX)
		return -1;
	cc_atexit_fn[cc_atexit_n] = func;
	cc_atexit_n++;
	return 0;
}

/*------------------------------------------------------------------------*/
/* SAS/C / Lattice compiler hooks (sc.lib / c.o when using stock CRT)     */
/*------------------------------------------------------------------------*/

#ifdef __SASC
/*
 * SAS/C autoinitialization / autotermination (User's Guide Ch. 10):
 *   _STI... -> linker builds __ctors[] (NULL-terminated)
 *   _STD... -> linker builds __dtors[]
 * With no such functions, slink substitutes absolute 0 for references to
 * __ctors / __dtors - you MUST NOT index them (SAS UG: check not NULL first).
 *
 * Matcha (matcha_cxx, ma_autolib _STI_4000_Matcha, ...) supplies real
 * tables, so walking is required.  Pure C apps (CCTest, fileutils) often
 * have none: address 0.  Do not take the address via `extern T arr[]` in
 * C - that form can be assumed non-NULL.  Use asm helpers instead.
 *
 * Called from _main after WB message, before OpenLibrary(cclib).
 * STI return values are ignored (SAS: void; SAS/C++ may leave 0 in d0).
 */
typedef LONG __stdargs (*cc_sti_fn)(void);
typedef void __stdargs (*cc_std_fn)(void);

/* cc_start.asm: move.l #__ctors/#__dtors -> d0 (same link model as __main) */
extern APTR __stdargs __cc_ctors_addr(void);
extern APTR __stdargs __cc_dtors_addr(void);

void __stdargs __fpinit(void)
{
	cc_sti_fn *table;
	cc_sti_fn fn;

	CC_CRT_DBG("cc_crt: __fpinit begin\n");
	table = (cc_sti_fn *)__cc_ctors_addr();
	if (table != (cc_sti_fn *)NULL && (ULONG)table >= 0x1000UL)
	{
		while (*table != (cc_sti_fn)NULL)
		{
			fn = *table;
			(void)(*fn)();
			table++;
		}
	}
	CC_CRT_DBG("cc_crt: __fpinit done\n");
}

void __stdargs __fpterm(void)
{
	cc_std_fn *table;
	cc_std_fn fn;

	table = (cc_std_fn *)__cc_dtors_addr();
	if (table != (cc_std_fn *)NULL && (ULONG)table >= 0x1000UL)
	{
		while (*table != (cc_std_fn)NULL)
		{
			fn = *table;
			(*fn)();
			table++;
		}
	}
}

void __stdargs __fpclear(void)
{
	__fpterm();
}

/*
 * SAS/C C++ / reaction.lib expect ___construct / ___destruct (sc.lib).
 * CClib startup already runs __ctors via __fpinit before main; these
 * symbols exist so .cxx / reaction objects link without sc.lib. They do
 * not re-walk __ctors (would double-init). Full C++ ctor-table parity
 * beyond __ctors remains TODO if a .cxx needs more than _STI-style init.
 */
void __stdargs ___construct(void)
{
}

void __stdargs ___destruct(void)
{
}

void __stdargs __construct(void)
{
}

void __stdargs __destruct(void)
{
}

void __regargs __chkabort(void)
{
	ULONG pending;
	cc_task_UserData *ud;

	/*
	 * Fast path: no break bits.  Must not call CheckBreak unconditionally —
	 * SAS inserts __chkabort on every function entry, including __fpclear
	 * after cc_terminate has CloseLibrary'd and nulled CCLibBase.
	 */
	if ((SetSignal(0L, 0L) & CC_BREAK_MASK) == 0UL)
		return;

	/* Library already closed: cannot libcall; leave bits for the shell. */
	if (CCLibBase == (struct Library *)NULL)
		return;

	pending = CheckBreak();
	if (pending == 0UL)
		return;

	/*
	 * CheckBreak returns with bits pending when abort_func is missing
	 * (or returned).  Mirror historic CRT: exit if no hook.
	 */
	ud = (cc_task_UserData *)GetSTDIO();
	if (ud == (cc_task_UserData *)NULL
	    || ud->abort_func == (cc_ABORT_FUNC)NULL)
		exit(20);
}

void __regargs _chkabort(void)
{
	__chkabort();
}

void __regargs _CXBRK(void)
{
	__chkabort();
}

/* Lattice spelled this XCEXIT; SAS/C uses __XCEXIT. */
void __stdargs __XCEXIT(long code)
{
	_exit((LONG)code);
}

void __stdargs XCEXIT(long code)
{
	_exit((LONG)code);
}
#endif

/*------------------------------------------------------------------------*/
/* _main - Lattice signature: NUL-terminated arg string only              */
/* __stdargs: stack args, no parm=r '@' name (matches historic lcmain).   */
/*------------------------------------------------------------------------*/

void __stdargs
_main(STRPTR aptr)
{
	cc_task_UserData *ud;
	struct Process *ThisProcess;
	LONG alen;
	int rv;

	cc_exiting = 0;
	cc_math_opened = 0;
	cc_atexit_n = 0;
	cc_atexit_ran = 0;
	MathIeeeDoubBasBase = (struct Library *)NULL;
	CCLibBase = (struct Library *)NULL;
	WBenchMsg = (struct WBStartup *)NULL;
	exit_fcn = (void (*)(void))NULL;
	stdin = (FILE *)NULL;
	stdout = (FILE *)NULL;
	stderr = (FILE *)NULL;
	errno = 0;
	type = (char *)NULL;
	_oldtrap = (void *)NULL;
	_trapaddr = (void **)NULL;
	_stkbase = 0;
	rv = 20;
	alen = 0;

	/*
	 * Lattice lcmain: if WBenchMsg is already set, stock c.o ran and
	 * will reply to WB again after CClib - crash.  Abort hard.
	 */
	if (WBenchMsg != (struct WBStartup *)NULL)
	{
		if (DOSBase != (struct DosLibrary *)NULL)
			Exit(100);
		return;
	}

	/* Aztec MANX stack cookie when asm prologue filled _savsp */
	if (_savsp != 0)
	{
		_stkbase = _savsp - *((LONG *)_savsp + 1) + 8;
		*(LONG *)_stkbase = 0x4D414E58L;
	}

	ThisProcess = (struct Process *)FindTask(NULL);

	if (ThisProcess->pr_CLI == (BPTR)NULL)
	{
		WaitPort(&ThisProcess->pr_MsgPort);
		WBenchMsg = (struct WBStartup *)GetMsg(&ThisProcess->pr_MsgPort);
	}

#ifdef __SASC
	__fpinit();
#endif

	CCLibBase = OpenLibrary((STRPTR)CCLIBNAME, 4);
	if (CCLibBase == (struct Library *)NULL)
	{
		if (DOSBase != (struct DosLibrary *)NULL)
		{
			BPTR out;
			STRPTR msg;

			msg = (STRPTR)"CCTest: cannot open cclib.library v4\n";
			out = Output();
			if (out != (BPTR)NULL)
			{
				alen = 0;
				while (msg[alen] != '\0')
					alen++;
				Write(out, msg, alen);
			}
		}
		goto fail;
	}

	if (aptr != (STRPTR)NULL)
		alen = strlen(aptr);
	else
		alen = 0;

	if (_math)
	{
		if (MathIeeeDoubBasBase == (struct Library *)NULL)
		{
			MathIeeeDoubBasBase = OpenLibrary(
				(STRPTR)"mathieeedoubbas.library", 0);
			if (MathIeeeDoubBasBase == (struct Library *)NULL)
			{
				if (DOSBase != (struct DosLibrary *)NULL)
				{
					BPTR out;
					STRPTR msg;

					msg = (STRPTR)"CCTest: cannot open mathieeedoubbas.library\n";
					out = Output();
					if (out != (BPTR)NULL)
					{
						alen = 0;
						while (msg[alen] != '\0')
							alen++;
						Write(out, msg, alen);
					}
				}
				goto fail;
			}
			cc_math_opened = 1;
		}
	}

	/*
	 * Install exit() as the break/abort hook so Ctrl-C runs atexit
	 * handlers and ClearSTDIO rather than a bare dos Exit().
	 */
	if (!SetupSTDIO(
		(APTR)&stdin, (APTR)&stdout, (APTR)&stderr,
		(APTR)&errno, (APTR)&blocksize, (APTR)&type,
		(APTR)MathIeeeDoubBasBase,
		alen, aptr, (APTR)WBenchMsg, (APTR)exit))
	{
		if (DOSBase != (struct DosLibrary *)NULL)
		{
			BPTR out;
			STRPTR msg;

			msg = (STRPTR)"CCTest: SetupSTDIO failed\n";
			out = Output();
			if (out != (BPTR)NULL)
			{
				alen = 0;
				while (msg[alen] != '\0')
					alen++;
				Write(out, msg, alen);
			}
		}
		goto fail;
	}

	ud = (cc_task_UserData *)GetSTDIO();
	if (ud != (cc_task_UserData *)NULL)
	{
		CC_CRT_DBG("cc_crt: calling main()\n");
		rv = main((int)ud->_argc, ud->_argv);
		CC_CRT_DBG("cc_crt: main() returned\n");
	}
	else
	{
		CC_CRT_DBG("cc_crt: GetSTDIO NULL — skip main\n");
		rv = 20;
	}

fail:
	_exit((LONG)rv);
}

/*
 * Shared teardown for exit / _exit / abort.  Always runs atexit (Amiga
 * extension vs ISO _exit) so abnormal paths cannot skip user cleanup,
 * then ClearSTDIO for library-owned resources.
 */
static void
cc_terminate(LONG code)
{
	LONG rv;

	if (cc_exiting)
	{
		if (DOSBase != (struct DosLibrary *)NULL)
			Exit(code);
		return;
	}
	cc_exiting = 1;
	rv = code;

	/* LIFO atexit handlers, then legacy single exit_fcn hook. */
	cc_run_atexit();
	if (exit_fcn != (void (*)(void))NULL)
		(*exit_fcn)();

	if (_trapaddr != (void **)NULL)
		*_trapaddr = _oldtrap;

	if (CCLibBase != (struct Library *)NULL)
	{
		ClearSTDIO();
		CloseLibrary(CCLibBase);
		CCLibBase = (struct Library *)NULL;
	}

	if (cc_math_opened && MathIeeeDoubBasBase != (struct Library *)NULL)
	{
		CloseLibrary(MathIeeeDoubBasBase);
		MathIeeeDoubBasBase = (struct Library *)NULL;
		cc_math_opened = 0;
	}

	if (UtilityBase != (struct Library *)NULL)
	{
		CloseLibrary(UtilityBase);
		UtilityBase = (struct Library *)NULL;
	}

	if (WBenchMsg != (struct WBStartup *)NULL)
	{
		Forbid();
		ReplyMsg((struct Message *)WBenchMsg);
		WBenchMsg = (struct WBStartup *)NULL;
		Permit();
	}

#ifdef __SASC
	__fpclear();
#endif

	if (DOSBase != (struct DosLibrary *)NULL)
		Exit(rv);
}

void __stdargs __saveds
_exit(LONG code)
{
	cc_terminate(code);
}

void __stdargs __saveds
exit(LONG code)
{
	cc_terminate(code);
}

void __stdargs __saveds
abort(void)
{
	cc_terminate(20L); /* EXIT_FAILURE */
}
