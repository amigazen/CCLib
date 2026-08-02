/*
 * StartUp.c - FuncTab and LibInit/Open/Close/Expunge for cclib.library
 *
 * FuncTab[] order MUST match SDK/SFD/cclib_lib.sfd.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <dos/dos.h>

#include <proto/exec.h>

#include "private/cc_build.h"
#include "private/cc_internal.h"
#include "cclib_funcs.h"

extern const char AS_LibName[];
extern const char AS_LibId[];

struct Library *CCLibBase;
struct ExecBase *SysBase;
struct DosLibrary *DOSBase;
struct Library *UtilityBase;
APTR SegList;

struct InitTable {
	ULONG it_LibSize;
	APTR *it_FuncTable;
	APTR it_DataTable;
	APTR it_InitFunc;
};

static VOID FreeLib(struct Library *lib);

APTR FuncTab[];

struct InitTable InitTab = {
	(ULONG)sizeof(struct Library),
	(APTR *)FuncTab,
	(APTR)NULL,
	(APTR)LibInit
};

APTR FuncTab[] = {
	(APTR)LibOpen,
	(APTR)LibClose,
	(APTR)LibExpunge,
	(APTR)LibReserved,
	(APTR)SetSTDIO,
	(APTR)GetSTDIO,
	(APTR)ClearSTDIO,
	(APTR)closeall,
	(APTR)SetAbortFunc,
	(APTR)ClearAbortFunc,
	(APTR)SetupSTDIO,
	(APTR)strerror,
	(APTR)open,
	(APTR)close,
	(APTR)remove,
	(APTR)stat,
	(APTR)rename,
	(APTR)read,
	(APTR)write,
	(APTR)creat,
	(APTR)lseek,
	(APTR)isatty,
	(APTR)access,
	(APTR)fopen,
	(APTR)freopen,
	(APTR)fclose,
	(APTR)fdopen,
	(APTR)getbuff,
	(APTR)setvbuf,
	(APTR)mktemp,
	(APTR)tmpnam,
	(APTR)tmpfile,
	(APTR)putc,
	(APTR)aputc,
	(APTR)getc,
	(APTR)agetc,
	(APTR)getchar,
	(APTR)ungetc,
	(APTR)putw,
	(APTR)puts,
	(APTR)puterr,
	(APTR)fwrite,
	(APTR)perror,
	(APTR)fseek,
	(APTR)ftell,
	(APTR)fflush,
	(APTR)getw,
	(APTR)fread,
	(APTR)gets,
	(APTR)fgets,
	(APTR)fputs,
	(APTR)scanf,
	(APTR)printf,
	(APTR)fscanf,
	(APTR)fprintf,
	(APTR)sscanf,
	(APTR)sprintf,
	(APTR)_alloc,
	(APTR)_fre,
	(APTR)_freall,
	(APTR)sbrk,
	(APTR)heap_alloc,
	(APTR)heap_free,
	(APTR)malloc,
	(APTR)calloc,
	(APTR)realloc,
	(APTR)free,
	(APTR)be_free,
	(APTR)malloc_size,
	(APTR)freeall,
	(APTR)inst_node,
	(APTR)put_head,
	(APTR)add_head,
	(APTR)put_tail,
	(APTR)add_tail,
	(APTR)remv_head,
	(APTR)remv_tail,
	(APTR)remv_list,
	(APTR)r_node,
	(APTR)remv_node,
	(APTR)atoi,
	(APTR)atol,
	(APTR)atof,
	(APTR)ltoa,
	(APTR)stoa,
	(APTR)bcmp,
	(APTR)bcopy,
	(APTR)bzero,
	(APTR)memccpy,
	(APTR)memchr,
	(APTR)memcmp,
	(APTR)memcpy,
	(APTR)memset,
	(APTR)memswap,
	(APTR)index,
	(APTR)rindex,
	(APTR)strupr,
	(APTR)stpcrlf,
	(APTR)strcat,
	(APTR)strchr,
	(APTR)strcmp,
	(APTR)strcpy,
	(APTR)strcspn,
	(APTR)strlen,
	(APTR)strncat,
	(APTR)strcasecmp,
	(APTR)strncmp,
	(APTR)strncpy,
	(APTR)strpbrk,
	(APTR)strrchr,
	(APTR)strspn,
	(APTR)strstr,
	(APTR)strtok,
	(APTR)strrv,
	(APTR)strnrv,
	(APTR)bcpl_strcpy,
	(APTR)vquicksort,
	(APTR)toupper,
	(APTR)quicksort,
	(APTR)qsort,
	(APTR)cli_parse,
	(APTR)wb_parse,
	(APTR)vprintf,
	(APTR)vfprintf,
	(APTR)vsprintf,
	(APTR)fgetpos,
	(APTR)fsetpos,
	(APTR)time,
	(APTR)clock,
	(APTR)gmtime,
	(APTR)asctime,
	(APTR)ctime,
	(APTR)localtime,
	(APTR)mktime,
	(APTR)difftime,
	(APTR)strftime,
	(APTR)scdir,
	(APTR)scdir_clean,
	(APTR)tolower,
	(APTR)setenv,
	(APTR)getenv,
	(APTR)stat_,
	(APTR)wb_exec,
	(APTR)wbexec,
	(APTR)wbAexec,
	(APTR)wait_child,
	(APTR)wait_children,
	(APTR)signal_child,
	(APTR)kill,
	(APTR)kill_children,
	(APTR)cliexec,
	(APTR)exec,
	(APTR)memmove,
	(APTR)((LONG)-1)
};

LONG
__ASM__ LibReserved(void)
{
	return 0;
}

struct Library *
__ASM__ __SAVE_DS__ LibInit(
	__REG__(a6, struct ExecBase *sysbase),
	__REG__(a0, APTR seglist),
	__REG__(d0, struct Library *lib))
{
	SysBase = sysbase;

	/* AmigaOS 3.2+ (utility V47 provides Strncpy/Strncat/VSNPrintf). */
	DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
	UtilityBase = OpenLibrary("utility.library", 47);

	if (DOSBase != NULL && UtilityBase != NULL)
	{
		lib->lib_Node.ln_Type = NT_LIBRARY;
		lib->lib_Node.ln_Pri = 0;
		lib->lib_Node.ln_Name = (STRPTR)AS_LibName;
		lib->lib_Flags = LIBF_CHANGED | LIBF_SUMUSED;
		lib->lib_Version = CC_LIB_VERSION;
		lib->lib_Revision = CC_LIB_REVISION;
		lib->lib_IdString = (STRPTR)AS_LibId;
		SegList = seglist;
		CCLibBase = lib;
	}
	else
	{
		FreeLib(lib);
		lib = NULL;
	}

	return lib;
}

struct Library *
__ASM__ __SAVE_DS__ LibOpen(__REG__(a6, struct Library *lib))
{
	++lib->lib_OpenCnt;
	lib->lib_Flags &= ~LIBF_DELEXP;
	return lib;
}

APTR
__ASM__ __SAVE_DS__ LibClose(__REG__(a6, struct Library *lib))
{
	if (lib->lib_OpenCnt && --lib->lib_OpenCnt)
	{
		return NULL;
	}

	if (lib->lib_Flags & LIBF_DELEXP)
	{
		return LibExpunge(lib);
	}

	return NULL;
}

APTR
__ASM__ __SAVE_DS__ LibExpunge(__REG__(a6, struct Library *lib))
{
	if (lib->lib_OpenCnt)
	{
		lib->lib_Flags |= LIBF_DELEXP;
		return NULL;
	}

	Remove(&lib->lib_Node);
	FreeLib(lib);

	return SegList;
}

static VOID
FreeLib(struct Library *lib)
{
	if (UtilityBase != NULL)
	{
		CloseLibrary(UtilityBase);
		UtilityBase = NULL;
	}
	if (DOSBase != NULL)
	{
		CloseLibrary((struct Library *)DOSBase);
		DOSBase = NULL;
	}

	FreeMem((UBYTE *)lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize);
	CCLibBase = NULL;
}
