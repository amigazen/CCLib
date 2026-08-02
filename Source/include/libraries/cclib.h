#ifndef LIBRARIES_CCLIB_H
#define LIBRARIES_CCLIB_H

/*
 * libraries/cclib.h — public ABI structures for cclib.library
 *
 * Shared by the standard C headers in this SDK (stdio.h, iolib.h, …).
 * Prefer these over the SAS/C clib when building for the CClib target.
 * Open the library with OpenLibrary(CCLIBNAME, 4) and include
 * the CClib SDK headers. Clients either include <proto/cclib.h>
 * (define struct Library *CCLibBase in one .c) or link the auto/stub library.
 */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef EXEC_TASKS_H
#include <exec/tasks.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif
#ifndef WORKBENCH_STARTUP_H
#include <workbench/startup.h>
#endif

#define CCLIBNAME "CClib.library"

#ifndef CCLIB_STDLIST_TYPES
#define CCLIB_STDLIST_TYPES 1
typedef struct _node
{
	struct _node *prev, *next;
} _node;

typedef struct
{
	_node *first, *last;
	long length;
} _list;
#endif

#ifndef CCLIB_HEAPMEM_TYPES
#define CCLIB_HEAPMEM_TYPES 1
typedef long ALIGN;

union header
{
	struct
	{
		union header *ptr;
		unsigned long size;
	} s;
	ALIGN x;
};

typedef union header HEADER;

typedef struct
{
	void *ptr;
	unsigned long size;
} LastFree;
#endif

#ifndef CCLIB_FILE_TYPE
#define CCLIB_FILE_TYPE 1
typedef struct
{
	long _unit;
	char *_bp;
	char *_bend;
	char *_buff;
	char _flags;
	char _bytbuf;
	short _buflen;
	char *_tmpname;
} FILE;
#endif

#ifndef __TIME_T
#define __TIME_T 1
typedef long time_t;
#endif

#ifndef __CLOCK_T
#define __CLOCK_T 1
typedef long clock_t;
#endif

#ifndef CCLIB_TM_TYPE
#define CCLIB_TM_TYPE 1
struct tm
{
	short tm_sec;
	short tm_min;
	short tm_hour;
	short tm_mday;
	short tm_mon;
	short tm_year;
	short tm_wday;
	short tm_yday;
	short tm_isdst;
	short tm_hsec;
};
#endif

#ifndef CCLIB_STAT_TYPE
#define CCLIB_STAT_TYPE 1
struct stat
{
	unsigned short st_attr;
	time_t st_mtime;
	long st_size;
	unsigned short st_mode;
	short st_nlink;
	time_t st_atime;
	time_t st_ctime;
};
#endif

typedef void (*ABORT_FUNC)(long);

typedef struct
{
	BPTR fh;
	short mode;
} FileDesc;

typedef struct
{
	_node n;
	long size;
} MemBlk;

typedef struct
{
	struct WBStartup startup;
	struct WBArg arg;
	struct Process *pid;
} Child;

typedef struct
{
	void *old_ud;
	struct Task *parent;
	_list RAM;
	HEADER base, *allocp;
	long *blocksize;
	LastFree lastfree;
	_list OpenFiles;
	_list StreamFiles;
	FILE **stdout, **stdin, **stderr;
	void *scnfp;
	long *errno;
	ABORT_FUNC abort_func;
	void *wbmsg;
	short _argc, _arg_len;
	char **_argv, *_arg_lin;
	char *scanpoint;
	char *tempname;
	short scnlast;
	struct tm tm;
	clock_t start;
	char buffer[80];
	void *scdir_mem;
	void *getenv_mem;
	struct MsgPort *exeport;
	_list Children;
} task_UserData;

#endif /* LIBRARIES_CCLIB_H */
