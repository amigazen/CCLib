/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

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
#define CC_EINTR   11
#define CC_EBUSY   12
#define CC_EAGAIN  13
#define CC_ESRCH   14
#define CC_EDEADLK 15
#define CC_ETIMEDOUT 16
#define CC_ENOSYS  17
#define CC_EPERM   18
#define CC_EOVERFLOW 19

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
/* Lazy FILE buffer (getbuff).  Public BUFSIZ stays 1024 for setbuf(). */
#define CC_STDIO_BUFSIZE 8192L
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

/*
 * Historic FILE / FileDesc field prefixes match libraries/cclib.h.
 * Private magic lives after those fields (not part of the public typedef).
 * Capability encoding today is still the object pointer; resolve only via
 * cc_*_from_list (membership + magic) before touching OS resources.
 */
#define CC_FP_MAGIC   0x43434650UL /* 'CCFP' */
#define CC_FD_MAGIC   0x43434644UL /* 'CCFD' */
#define CC_FP_POISON  0xDEADF11EUL
#define CC_FD_POISON  0xDEADF00DUL

/* Soft cap on OpenFiles length; open() sets EMFILE when reached. */
#define CC_MAX_OPEN_FILES 256L

/* Memory class tags (documentation / future grant-ready seams). */
#define CC_MEM_PRIVATE_HEAP   1
#define CC_MEM_STDIO_BUF      2
#define CC_MEM_CTRL_BLOCK     3
#define CC_MEM_IPC            4
#define CC_MEM_SCRATCH        5

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
	ULONG magic;
} cc_FILE;

typedef struct
{
	BPTR fh;
	short mode;
	/* Live FILE* / fd owners sharing this BPTR (stdio when Error==Output). */
	UWORD refs;
	ULONG magic;
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

typedef void __stdargs (*cc_ABORT_FUNC)(LONG);

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
	int *errno_p;
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
	/* NDK 3.2R4 session heap pool (CreatePool); past public task_UserData ABI. */
	APTR heap_pool;
	/*
	 * POSIX locale session (Wave 0).  locale == NULL means "C".
	 * Opaque APTR avoids forcing locale.library includes on every module;
	 * cc_locale.c treats it as struct Locale *.
	 */
	APTR locale;
	ULONG lc_mask;
	char locale_name[32];
	/* POSIX rand/srand seed (default 1). */
	ULONG rand_seed;
	/* Head of open DIR list (dirent.c); cleared in ClearSTDIO. */
	APTR open_dirs;
	/*
	 * Opaque pthread / TLS context for static ccpthread.lib.
	 * thr_teardown(thr_ctx) runs from ClearSTDIO when both are set.
	 * Use __stdargs so the library call matches the static-lib callee.
	 */
	APTR thr_ctx;
	void __stdargs (*thr_teardown)(APTR);
	/* Per-session ClearSTDIO re-entry latch (not library-global). */
	LONG clearing;
	/* CC_PANIC_ABORT (default) or CC_PANIC_HOOK — private/cc_panic.h.
	 * Kept at end so earlier session fields (thr_ctx, …) stay stable.
	 */
	short panic_mode;
	short panic_pad;
	/* CRT backtrace: set by cc_trace_init.  cc_panic calls this
	 * before abort_func so a returning test hook still prints.
	 */
	void (*trace_dump)(void);
} cc_task_UserData;

/* div()/ldiv() result (ISO C layout; filled via LVO result pointer). */
typedef struct
{
	int quot;
	int rem;
} cc_div_t;

/* Opaque DIR payload (also exposed as DIR in <dirent.h>). */
#define CC_DIR_MAGIC  0x43434449UL /* 'CCDI' */
#define CC_DIR_POISON 0xDEADD112UL
#define CC_PATH_MAX   1024L

typedef struct cc_DIR
{
	struct cc_DIR *next;
	ULONG magic;
	BPTR lock;
	struct FileInfoBlock *fib;
	LONG at_entry; /* 0 = need Examine; 1 = ExNext loop */
	char d_name[108];
} cc_DIR;

/* LC_* bit masks for ud->lc_mask (match <locale.h> category values). */
#define CC_LC_COLLATE  0x0001UL
#define CC_LC_CTYPE    0x0002UL
#define CC_LC_MONETARY 0x0004UL
#define CC_LC_NUMERIC  0x0008UL
#define CC_LC_TIME     0x0010UL
#define CC_LC_MESSAGES 0x0020UL
#define CC_LC_ALL      0x003FUL

/* Default heap quantum: 256 * 8 = 2048 bytes when *blocksize == 0 */
#define CC_DEFAULT_BLOCKSIZE 256L

/* Per-session malloc pool puddle / threshold (exec CreatePool). */
#define CC_HEAP_PUDDLE  8192UL
#define CC_HEAP_THRESH  4096UL

extern struct Library *CCLibBase;
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct Library *UtilityBase;
extern APTR SegList;

/* Shared ctype table exported via SetupSTDIO's type pointer ("C" locale). */
extern UBYTE cc_ctype_tab[257];

/* Console Break / AmigaDOS Break command (Ctrl-C..F). */
#define CC_BREAK_MASK \
	(SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_E | SIGBREAKF_CTRL_F)

cc_task_UserData *cc_get_ud(void);
void cc_set_errno(LONG err);
LONG cc_get_errno(void);

/*
 * Break poll (SAS __chkabort model for shared LVOs).  Watches
 * CC_BREAK_MASK (Ctrl-C..F).  Clears pending bits and calls
 * ud->abort_func(20) when armed.  When abort_func is NULL, leaves bits
 * set (no session / ClearSTDIO disarmed).  Leaf LVOs that never call
 * cc_get_ud() should call cc_check_break() at entry; cc_get_ud() polls
 * too.  CheckBreak LVO is the companion-callable form of cc_poll_break.
 *
 * After Wait(...|CC_BREAK_MASK), bits are already cleared - call
 * cc_raise_ctrl_c() instead of cc_check_break().
 */
ULONG cc_poll_break(void);
void cc_check_break(void);
void cc_raise_ctrl_c(void);

/*
 * Panic: invalid capability / programming error.  Writes a console
 * message, sets IoErr (Shell Result2 / WHY), then:
 *   CC_PANIC_ABORT (default): ud->abort_func if armed, else ClearSTDIO+Exit
 *   CC_PANIC_HOOK: abort_func if armed; never Exit from the library
 * Forged-handle detection runs either way.  If the hook returns, the
 * caller must still fail the LVO (errno already set).
 *
 * Primary RC is the Exit() code from abort_func/CRT - never SetVar("RC")
 * or SetVar("Result2") from a still-running RunCommand (CLI deadlock).
 *
 * The library-global panic latch is cleared before abort_func/Exit so a
 * shared cclib.library image is not left poisoned for the next client.
 */
void cc_panic(LONG code);

/* Capability resolve: membership on session registries (+ magic). */
cc_FileDesc *cc_fd_from_list(cc_task_UserData *ud, LONG fd);
cc_FILE *cc_file_from_list(cc_task_UserData *ud, APTR stream);
cc_DIR *cc_dir_from_list(cc_task_UserData *ud, APTR dirp);
cc_DIR *cc_dir_from_list(cc_task_UserData *ud, APTR dirp);
cc_MemBlk *cc_blk_from_malloc_ptr(cc_task_UserData *ud, APTR ptr);

/*
 * C string / buffer validation (same shape as printf %s).
 * NULL is a sentinel: the LVO sets EINVAL and does not call these.
 * Unmapped (TypeOfMem 0) or no NUL in CC_STR_PROBE -> EINVAL + panic, 0.
 */
#define CC_STR_PROBE 1024L
LONG cc_str_mapped(STRPTR s);
LONG cc_str_arg(STRPTR s);
LONG cc_buf_mapped(APTR p, ULONG n);
LONG cc_buf_arg(APTR p, ULONG n);

/*
 * Locale / character helpers (Wave 0).  Today behave as "C" + utility;
 * when ud->locale is set (Wave 1+), case/collate route through locale.library.
 */
void cc_locale_init(cc_task_UserData *ud);
void cc_locale_clear(cc_task_UserData *ud);
LONG cc_toupper_ch(cc_task_UserData *ud, LONG c);
LONG cc_tolower_ch(cc_task_UserData *ud, LONG c);
LONG cc_str_casecmp(cc_task_UserData *ud, STRPTR s1, STRPTR s2);
LONG cc_str_ncasecmp(cc_task_UserData *ud, STRPTR s1, STRPTR s2, LONG n);
LONG cc_strcoll(cc_task_UserData *ud, STRPTR s1, STRPTR s2);

/* ctype classification: "C" table or locale.library Is* when LC_CTYPE. */
LONG cc_isalnum_ch(cc_task_UserData *ud, LONG c);
LONG cc_isalpha_ch(cc_task_UserData *ud, LONG c);
LONG cc_iscntrl_ch(cc_task_UserData *ud, LONG c);
LONG cc_isdigit_ch(cc_task_UserData *ud, LONG c);
LONG cc_isgraph_ch(cc_task_UserData *ud, LONG c);
LONG cc_islower_ch(cc_task_UserData *ud, LONG c);
LONG cc_isprint_ch(cc_task_UserData *ud, LONG c);
LONG cc_ispunct_ch(cc_task_UserData *ud, LONG c);
LONG cc_isspace_ch(cc_task_UserData *ud, LONG c);
LONG cc_isupper_ch(cc_task_UserData *ud, LONG c);
LONG cc_isxdigit_ch(cc_task_UserData *ud, LONG c);
LONG cc_isblank_ch(cc_task_UserData *ud, LONG c);
LONG cc_isascii_ch(LONG c);

/*
 * Locale-aware strftime via locale.library/FormatDate when LC_TIME is
 * active.  Returns 1 if the locale path was used (*out_len is the result
 * length, or 0 on truncation).  Returns 0 to fall back to Pure C.
 */
LONG cc_strftime_locale(cc_task_UserData *ud, STRPTR s, ULONG maxsize,
	STRPTR format, APTR tm, ULONG *out_len);

#endif /* CCLIB_PRIVATE_CC_INTERNAL_H */
