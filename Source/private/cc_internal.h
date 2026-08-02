/*
 * cc_internal.h - private ABI structures for cclib.library
 *
 * Layout matches historic V3 iolib.h / stdio.h / stdlist.h / heapmem.h so
 * CRT and apps that touch tc_UserData remain compatible. Library TUs must
 * not include SAS/C libc headers.
 */

#ifndef CCLIB_PRIVATE_CC_INTERNAL_H
#define CCLIB_PRIVATE_CC_INTERNAL_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>

/* errno values (historic CClib) */
#define CC_ENOENT  1
#define CC_EBADF   2
#define CC_ENOMEM  3
#define CC_EEXIST  4
#define CC_EINVAL  5
#define CC_EMFILE  6
#define CC_ENOTTY  7
#define CC_EACCES  8
#define CC_ERANGE  9
#define CC_EDOM    10

/* fcntl open flags */
#define CC_O_RDONLY  0
#define CC_O_WRONLY  1
#define CC_O_RDWR    (1<<1)
#define CC_O_CREAT   (1<<8)
#define CC_O_TRUNC   (1<<9)
#define CC_O_EXCL    (1<<10)
#define CC_O_APPEND  (1<<11)
#define CC_O_CONRAW  (1<<14)
#define CC_O_STDIO   (1<<15)

#define CC_BUFSIZ    1024L
#define CC_EOF       (-1)
#define CC_SEEK_SET  0L
#define CC_SEEK_CUR  1L
#define CC_SEEK_END  2L

#define CC_BUSY      (1<<0)
#define CC_ALLBUF    (1<<1)
#define CC_DIRTY     (1<<2)
#define CC_EOFBIT    (1<<3)
#define CC_IOERR     (1<<4)
#define CC_TEMP      (1<<5)

#define CC_IOFBF     1L
#define CC_IOLBF     2L
#define CC_IONBF     3L

#define CC_ST_DELETE   (1L<<0)
#define CC_ST_EXECUTE  (1L<<1)
#define CC_ST_WRITE    (1L<<2)
#define CC_ST_READ     (1L<<3)
#define CC_ST_ARCHIVE  (1L<<4)

#define CC_S_IFMT   0x16
#define CC_S_IFDIR  0x04
#define CC_S_IFCHR  0x02
#define CC_S_IFBLK  0x06
#define CC_S_IFREG  0x10

typedef struct cc_node
{
	struct cc_node *prev;
	struct cc_node *next;
} cc_node;

typedef struct
{
	cc_node *first;
	cc_node *last;
	LONG length;
} cc_list;

typedef long cc_ALIGN;

typedef union cc_header
{
	struct
	{
		union cc_header *ptr;
		ULONG size;
	} s;
	cc_ALIGN x;
} cc_HEADER;

typedef struct
{
	APTR ptr;
	ULONG size;
} cc_LastFree;

typedef struct
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
} cc_tm;

typedef struct
{
	UWORD st_attr;
	LONG st_mtime;
	LONG st_size;
	UWORD st_mode;
	short st_nlink;
	LONG st_atime;
	LONG st_ctime;
} cc_stat;

typedef struct
{
	LONG _unit;
	char *_bp;
	char *_bend;
	char *_buff;
	char _flags;
	char _bytbuf;
	short _buflen;
	char *_tmpname;
} cc_FILE;

typedef struct
{
	BPTR fh;
	short mode;
} cc_FileDesc;

typedef struct
{
	cc_node n;
	LONG size;
} cc_MemBlk;

typedef struct
{
	struct WBStartup startup;
	struct WBArg arg;
	struct Process *pid;
} cc_Child;

typedef void (*cc_ABORT_FUNC)(LONG);

#define CC_GETENV_BUFLEN 256L
#define CC_SCDIR_PATHLEN 256L

typedef struct
{
	APTR old_ud;
	struct Task *parent;
	cc_list RAM;
	cc_HEADER base;
	cc_HEADER *allocp;
	LONG *blocksize;
	cc_LastFree lastfree;
	cc_list OpenFiles;
	cc_list StreamFiles;
	cc_FILE **stdout_p;
	cc_FILE **stdin_p;
	cc_FILE **stderr_p;
	APTR scnfp;
	LONG *errno_p;
	cc_ABORT_FUNC abort_func;
	APTR wbmsg;
	short _argc;
	short _arg_len;
	char **_argv;
	char *_arg_lin;
	char *scanpoint;
	char *tempname;
	short scnlast;
	cc_tm tm;
	LONG start;
	char buffer[80];
	APTR scdir_mem;
	APTR getenv_mem;
	struct MsgPort *exeport;
	cc_list Children;
	APTR mathbase;
} cc_task_UserData;

/* Default heap quantum: 256 * 8 = 2048 bytes when *blocksize == 0 */
#define CC_DEFAULT_BLOCKSIZE 256L

extern struct Library *CCLibBase;
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct Library *UtilityBase;
extern APTR SegList;

/* Shared ctype table exported via SetupSTDIO's type pointer */
extern UBYTE cc_ctype_tab[257];

cc_task_UserData *cc_get_ud(void);
void cc_set_errno(LONG err);
LONG cc_get_errno(void);

#endif /* CCLIB_PRIVATE_CC_INTERNAL_H */
