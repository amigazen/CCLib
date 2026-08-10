/*
 * support.c - stdio setup, argv parsing, and strerror for cclib.library
 *
 * SetSTDIO attaches cc_task_UserData to the current task.  SetupSTDIO
 * wires application globals, parses CLI/WB args, and opens stdin/out/err.
 */

#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* icon.library is opened only inside wb_parse for WINDOW= tooltypes. */
#include <proto/icon.h>

/* proto/icon.h LVOs reference this global for the duration of wb_parse. */
struct Library *IconBase;

static void cc_list_init(cc_list *l)
{
	l->first = (cc_node *)NULL;
	l->last = (cc_node *)NULL;
	l->length = 0;
}

/*
 * Free every node on a list that was allocated by add_head/add_tail
 * (layout [ULONG total][cc_node][data]).  Does not FreeMem the list
 * header - OpenFiles/StreamFiles live inside cc_task_UserData.
 */
static void cc_list_free_nodes(cc_list *l)
{
	cc_node *n;
	cc_node *next;
	ULONG *hp;

	if (l == (cc_list *)NULL)
		return;
	n = l->first;
	while (n != (cc_node *)NULL) {
		next = n->next;
		hp = ((ULONG *)n) - 1;
		FreeMem(hp, *hp);
		n = next;
	}
	l->first = (cc_node *)NULL;
	l->last = (cc_node *)NULL;
	l->length = 0;
}

/* True if c is a CLI whitespace separator. */
static int cc_is_space(char c)
{
	if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
		return 1;
	return 0;
}

/*
 * Build FileDesc + FILE for a dos BPTR via add_tail (same allocator as
 * open/fopen).  mode should include CC_O_STDIO for CLI Input/Output.
 * If share_fd is non-NULL, reuse that FileDesc (shared BPTR, Close once).
 */
static cc_FILE *cc_make_std_stream(
	cc_task_UserData *ud,
	BPTR fh,
	short mode,
	cc_FileDesc *share_fd)
{
	cc_FileDesc *fd;
	cc_FILE *fp;

	if (fh == (BPTR)NULL)
		return (cc_FILE *)NULL;

	fd = share_fd;
	if (fd == (cc_FileDesc *)NULL) {
		fd = (cc_FileDesc *)add_tail((APTR)&ud->OpenFiles, (LONG)sizeof(cc_FileDesc));
		if (fd == (cc_FileDesc *)NULL)
			return (cc_FILE *)NULL;
		fd->fh = fh;
		fd->mode = mode;
		fd->pad = 0;
		fd->magic = CC_FD_MAGIC;
	}

	fp = (cc_FILE *)add_tail((APTR)&ud->StreamFiles, (LONG)sizeof(cc_FILE));
	if (fp == (cc_FILE *)NULL)
		return (cc_FILE *)NULL;

	fp->_unit = (LONG)fd;
	fp->_bp = (char *)NULL;
	fp->_bend = (char *)NULL;
	fp->_buff = (char *)NULL;
	fp->_flags = 0;
	fp->_bytbuf = 0;
	fp->_buflen = 0;
	fp->_tmpname = (char *)NULL;
	fp->magic = CC_FP_MAGIC;
	return fp;
}

LONG __ASM__ __SAVE_DS__ SetSTDIO(void)
{
	struct Task *task;
	cc_task_UserData *ud;
	APTR old;
	cc_check_break();

	task = FindTask((STRPTR)NULL);
	if (task == (struct Task *)NULL)
		return 0;

	ud = (cc_task_UserData *)AllocMem(sizeof(cc_task_UserData), MEMF_CLEAR | MEMF_PUBLIC);
	if (ud == (cc_task_UserData *)NULL)
		return 0;

	/* Per-session malloc pool (NDK 3.2R4 CreatePool). */
	ud->heap_pool = CreatePool(MEMF_ANY, CC_HEAP_PUDDLE, CC_HEAP_THRESH);
	if (ud->heap_pool == (APTR)NULL)
	{
		FreeMem(ud, sizeof(cc_task_UserData));
		return 0;
	}
	cc_locale_init(ud);

	old = task->tc_UserData;
	ud->old_ud = old;
	ud->parent = task;

	cc_list_init(&ud->RAM);
	cc_list_init(&ud->OpenFiles);
	cc_list_init(&ud->StreamFiles);
	cc_list_init(&ud->Children);

	ud->base.s.ptr = &ud->base;
	ud->base.s.size = 0;
	ud->allocp = &ud->base;
	ud->lastfree.ptr = (APTR)NULL;
	ud->lastfree.size = 0;

	ud->blocksize = (LONG *)NULL;
	ud->errno_p = (int *)NULL;
	ud->abort_func = (cc_ABORT_FUNC)NULL;
	ud->wbmsg = (APTR)NULL;
	ud->_argc = 0;
	ud->_arg_len = 0;
	ud->_argv = (char **)NULL;
	ud->_arg_lin = (char *)NULL;
	ud->scanpoint = (char *)NULL;
	ud->tempname = (char *)NULL;
	ud->scnlast = 0;
	ud->start = 0;
	ud->scdir_mem = (APTR)NULL;
	ud->getenv_mem = (APTR)NULL;
	ud->exeport = (struct MsgPort *)NULL;
	ud->mathbase = (APTR)NULL;
	ud->stdin_p = (cc_FILE **)NULL;
	ud->stdout_p = (cc_FILE **)NULL;
	ud->stderr_p = (cc_FILE **)NULL;
	ud->scnfp = (APTR)NULL;
	ud->rand_seed = 1UL;
	ud->open_dirs = (APTR)NULL;
	ud->thr_ctx = (APTR)NULL;
	ud->thr_teardown = (void __stdargs (*)(APTR))NULL;

	task->tc_UserData = (APTR)ud;
	return 1;
}

APTR __ASM__ __SAVE_DS__ GetSTDIO(void)
{
	cc_check_break();
	return (APTR)cc_get_ud();
}

void __ASM__ __SAVE_DS__ SetAbortFunc(
	__REG__(a0, APTR abortfunc))
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return;
	ud->abort_func = (cc_ABORT_FUNC)abortfunc;
}

void __ASM__ __SAVE_DS__ ClearAbortFunc(void)
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return;
	ud->abort_func = (cc_ABORT_FUNC)NULL;
}

/*
 * Close every tracked stream and low-level file.  Flush dirty buffers and
 * delete tmpfile paths before releasing list nodes.  CLI Input/Output
 * handles (CC_O_STDIO) are not passed to dos Close().  List node
 * memory is released with the add_head/add_tail FreeMem layout.
 */
void __ASM__ __SAVE_DS__ closeall(void)
{
	cc_task_UserData *ud;
	cc_node *n;
	cc_FILE *fp;
	cc_FileDesc *fd;
	BPTR fh;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return;

	/* Flush and reap stream-owned resources while nodes are still linked. */
	n = ud->StreamFiles.first;
	while (n != (cc_node *)NULL) {
		fp = (cc_FILE *)(n + 1);
		if (fp->magic == CC_FP_MAGIC) {
			if ((fp->_flags & (char)CC_DIRTY) != 0)
				(void)fflush((APTR)fp);
			if (fp->_tmpname != (char *)NULL) {
				if ((fp->_flags & (char)CC_TEMP) != 0)
					DeleteFile(fp->_tmpname);
				FreeMem(fp->_tmpname, 32UL);
				fp->_tmpname = (char *)NULL;
			}
			if (fp->_buff != (char *)NULL
			    && fp->_buff != &fp->_bytbuf
			    && (fp->_flags & (char)CC_ALLBUF) != 0) {
				FreeMem(fp->_buff, (ULONG)fp->_buflen);
				fp->_buff = (char *)NULL;
			}
			fp->_bp = (char *)NULL;
			fp->_bend = (char *)NULL;
			fp->_buflen = 0;
			fp->magic = CC_FP_POISON;
		}
		n = n->next;
	}

	n = ud->OpenFiles.first;
	while (n != (cc_node *)NULL) {
		fd = (cc_FileDesc *)(n + 1);
		fh = fd->fh;
		if (fh != (BPTR)NULL
		    && (fd->mode & (short)CC_O_STDIO) == 0) {
			Close(fh);
		}
		fd->fh = (BPTR)NULL;
		fd->magic = CC_FD_POISON;
		n = n->next;
	}

	if (ud->stdin_p != (cc_FILE **)NULL)
		*ud->stdin_p = (cc_FILE *)NULL;
	if (ud->stdout_p != (cc_FILE **)NULL)
		*ud->stdout_p = (cc_FILE *)NULL;
	if (ud->stderr_p != (cc_FILE **)NULL)
		*ud->stderr_p = (cc_FILE *)NULL;

	cc_list_free_nodes(&ud->StreamFiles);
	cc_list_free_nodes(&ud->OpenFiles);
}

void __ASM__ __SAVE_DS__ ClearSTDIO(void)
{
	struct Task *task;
	cc_task_UserData *ud;
	APTR old;
	static LONG clearing;

	/*
	 * Fetch ud without cc_get_ud() - that polls Ctrl-C.  Disarm
	 * abort_func first so teardown cannot re-enter exit via break.
	 */
	task = FindTask((STRPTR)NULL);
	if (task == (struct Task *)NULL)
		return;
	ud = (cc_task_UserData *)task->tc_UserData;
	if (ud == (cc_task_UserData *)NULL)
		return;

	ud->abort_func = (cc_ABORT_FUNC)NULL;

	if (clearing)
		return;
	clearing = 1;

	/*
	 * Static ccpthread session hook.  Require both slots: a lone non-NULL
	 * teardown after a partial library rebuild would jump into garbage.
	 */
	if (ud->thr_ctx != (APTR)NULL &&
	    ud->thr_teardown != (void __stdargs (*)(APTR))NULL)
	{
		(*ud->thr_teardown)(ud->thr_ctx);
		ud->thr_teardown = (void __stdargs (*)(APTR))NULL;
		ud->thr_ctx = (APTR)NULL;
	}

	if (ud->parent != (struct Task *)NULL)
		task = ud->parent;

	/*
	 * Tear down in a leak-safe order: stop directory scans, reap
	 * children, free getenv scratch, then streams/fds and the heap.
	 * atexit handlers are CRT-owned and must already have run.
	 */
	scdir_clean();
	/* Close any DIR left open by the app before streams/heap go away. */
	while (ud->open_dirs != (APTR)NULL)
		(void)closedir(ud->open_dirs);
	(void)kill_children();
	if (ud->getenv_mem != (APTR)NULL)
	{
		FreeMem(ud->getenv_mem, CC_GETENV_BUFLEN);
		ud->getenv_mem = (APTR)NULL;
	}
	if (ud->exeport != (struct MsgPort *)NULL)
	{
		DeleteMsgPort(ud->exeport);
		ud->exeport = (struct MsgPort *)NULL;
	}
	closeall();
	freeall();

	/* Drop locale before FreeMem(ud); no-op while locale stays "C". */
	cc_locale_clear(ud);

	if (ud->heap_pool != (APTR)NULL)
	{
		DeletePool(ud->heap_pool);
		ud->heap_pool = (APTR)NULL;
	}

	old = ud->old_ud;
	if (task != (struct Task *)NULL)
		task->tc_UserData = old;

	FreeMem(ud, sizeof(cc_task_UserData));
	clearing = 0;
}

void __ASM__ __SAVE_DS__ cli_parse(
	__REG__(a0, APTR ud),
	__REG__(d0, LONG alen),
	__REG__(a1, STRPTR aptr))
{
	cc_task_UserData *u;
	struct Process *pr;
	struct CommandLineInterface *cli;
	UBYTE *bname;
	LONG namelen;
	LONG i;
	LONG argc;
	LONG in_token;
	LONG need;
	char *store;
	char **argv;
	char *p;
	char *lim;
	char *dst;
	LONG argi;
	cc_check_break();

	u = (cc_task_UserData *)ud;
	if (u == (cc_task_UserData *)NULL)
		return;

	u->_argc = 0;
	u->_argv = (char **)NULL;
	u->_arg_lin = (char *)NULL;
	u->_arg_len = (short)alen;

	/* Count tokens in the CLI argument string. */
	argc = 1; /* argv[0] = command name */
	in_token = 0;
	i = 0;
	if (aptr != (STRPTR)NULL) {
		while (i < alen) {
			if (cc_is_space((char)aptr[i])) {
				in_token = 0;
			} else if (!in_token) {
				in_token = 1;
				argc++;
			}
			i++;
		}
	}

	/* Resolve argv[0] from the CLI command name BSTR when possible. */
	namelen = 0;
	bname = (UBYTE *)NULL;
	pr = (struct Process *)FindTask((STRPTR)NULL);
	if (pr != (struct Process *)NULL && pr->pr_CLI != (BPTR)NULL) {
		cli = (struct CommandLineInterface *)BADDR(pr->pr_CLI);
		if (cli != (struct CommandLineInterface *)NULL
		    && cli->cli_CommandName != (BPTR)NULL) {
			bname = (UBYTE *)BADDR(cli->cli_CommandName);
			namelen = (LONG)bname[0];
		}
	}
	if (namelen <= 0) {
		namelen = 0;
	}

	need = (LONG)((ULONG)(argc + 1) * sizeof(char *))
	    + namelen + 1
	    + ((aptr != (STRPTR)NULL && alen > 0) ? (alen + 1) : 1);

	store = (char *)_alloc(u, need, (LONG)(MEMF_CLEAR | MEMF_PUBLIC));
	if (store == (char *)NULL)
		return;

	argv = (char **)store;
	dst = store + (ULONG)(argc + 1) * sizeof(char *);

	/* argv[0] */
	argv[0] = dst;
	if (bname != (UBYTE *)NULL && namelen > 0) {
		i = 0;
		while (i < namelen) {
			dst[i] = (char)bname[i + 1];
			i++;
		}
		dst[namelen] = '\0';
		dst += namelen + 1;
	} else {
		dst[0] = '\0';
		dst++;
	}

	argi = 1;
	u->_arg_lin = dst;
	if (aptr != (STRPTR)NULL && alen > 0) {
		i = 0;
		while (i < alen) {
			dst[i] = (char)aptr[i];
			i++;
		}
		dst[alen] = '\0';

		p = u->_arg_lin;
		lim = u->_arg_lin + alen;
		while (p < lim && argi < argc) {
			while (p < lim && cc_is_space(*p))
				p++;
			if (p >= lim)
				break;
			argv[argi] = p;
			argi++;
			while (p < lim && !cc_is_space(*p))
				p++;
			if (p < lim) {
				*p = '\0';
				p++;
			}
		}
	}

	argv[argi] = (char *)NULL;
	u->_argc = (short)argi;
	u->_argv = argv;
}

LONG __ASM__ __SAVE_DS__ wb_parse(
	__REG__(a0, APTR ud),
	__REG__(a1, APTR wbm))
{
	cc_task_UserData *u;
	struct WBStartup *wb;
	struct WBArg *arg;
	struct DiskObject *dobj;
	STRPTR window;
	BPTR olddir;
	BPTR fh;
	LONG ok;
	cc_check_break();

	u = (cc_task_UserData *)ud;
	wb = (struct WBStartup *)wbm;
	ok = 0;

	if (u == (cc_task_UserData *)NULL || wb == (struct WBStartup *)NULL)
		return 0;
	if (wb->sm_NumArgs < 1 || wb->sm_ArgList == (struct WBArg *)NULL)
		return 0;

	arg = &wb->sm_ArgList[0];
	u->wbmsg = (APTR)wb;

	/* Build argv from WB arg list (argv[0] = tool name). */
	{
		LONG argc;
		LONG i;
		LONG need;
		char *store;
		char **argv;
		char *dst;
		LONG nlen;
		char *nm;

		argc = (LONG)wb->sm_NumArgs;
		if (argc < 1)
			argc = 1;

		need = (LONG)((ULONG)(argc + 1) * sizeof(char *));
		i = 0;
		while (i < argc) {
			nm = wb->sm_ArgList[i].wa_Name;
			nlen = 0;
			if (nm != (char *)NULL) {
				while (nm[nlen] != '\0')
					nlen++;
			}
			need += nlen + 1;
			i++;
		}

		store = (char *)_alloc(u, need, (LONG)(MEMF_CLEAR | MEMF_PUBLIC));
		if (store != (char *)NULL) {
			argv = (char **)store;
			dst = store + (ULONG)(argc + 1) * sizeof(char *);
			i = 0;
			while (i < argc) {
				nm = wb->sm_ArgList[i].wa_Name;
				argv[i] = dst;
				if (nm != (char *)NULL) {
					nlen = 0;
					while (nm[nlen] != '\0') {
						dst[nlen] = nm[nlen];
						nlen++;
					}
					dst[nlen] = '\0';
					dst += nlen + 1;
				} else {
					*dst = '\0';
					dst++;
				}
				i++;
			}
			argv[argc] = (char *)NULL;
			u->_argc = (short)argc;
			u->_argv = argv;
		}
	}

	IconBase = OpenLibrary((STRPTR)"icon.library", 0L);
	if (IconBase == (struct Library *)NULL)
		return 0;

	olddir = (BPTR)NULL;
	if (arg->wa_Lock != (BPTR)NULL)
		olddir = CurrentDir(arg->wa_Lock);

	dobj = GetDiskObject(arg->wa_Name);

	if (olddir != (BPTR)NULL)
		CurrentDir(olddir);

	if (dobj == (struct DiskObject *)NULL) {
		CloseLibrary(IconBase);
		IconBase = (struct Library *)NULL;
		return 0;
	}

	window = FindToolType(dobj->do_ToolTypes, (STRPTR)"WINDOW");
	if (window != (STRPTR)NULL) {
		fh = Open(window, MODE_NEWFILE);
		if (fh != (BPTR)NULL) {
			SelectInput(fh);
			SelectOutput(fh);
			ok = 1;
		}
	}

	FreeDiskObject(dobj);
	CloseLibrary(IconBase);
	IconBase = (struct Library *)NULL;
	return ok;
}

LONG __ASM__ __SAVE_DS__ SetupSTDIO(
	__REG__(a0, APTR stdin_p),
	__REG__(a1, APTR stdout_p),
	__REG__(a2, APTR stderr_p),
	__REG__(a3, APTR errno_p),
	__REG__(a4, APTR blocksize_p),
	__REG__(a5, APTR type_p),
	__REG__(d0, APTR mathbase),
	__REG__(d1, LONG alen),
	__REG__(d2, STRPTR aptr),
	__REG__(d3, APTR wbm),
	__REG__(d4, APTR exitfunc))
{
	cc_task_UserData *ud;
	BPTR in_fh;
	BPTR out_fh;
	BPTR err_fh;
	cc_FILE *fp_in;
	cc_FILE *fp_out;
	cc_FILE *fp_err;
	cc_FileDesc *fd_in;
	cc_FileDesc *fd_out;
	short stdio_bit;
	LONG have_io;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL) {
		if (!SetSTDIO())
			return 0;
		ud = cc_get_ud();
		if (ud == (cc_task_UserData *)NULL)
			return 0;
	}

	ud->stdin_p = (cc_FILE **)stdin_p;
	ud->stdout_p = (cc_FILE **)stdout_p;
	ud->stderr_p = (cc_FILE **)stderr_p;
	ud->errno_p = (int *)errno_p;
	ud->blocksize = (LONG *)blocksize_p;
	ud->mathbase = mathbase;
	ud->abort_func = (cc_ABORT_FUNC)exitfunc;
	ud->wbmsg = wbm;

	if (type_p != (APTR)NULL)
		*((char **)type_p) = (char *)cc_ctype_tab;

	if (ud->errno_p != (int *)NULL)
		*ud->errno_p = 0;

	have_io = 1;
	if (wbm != (APTR)NULL) {
		if (!wb_parse(ud, wbm))
			have_io = 0;
	} else {
		cli_parse(ud, alen, aptr);
	}

	if (stdin_p != (APTR)NULL)
		*((cc_FILE **)stdin_p) = (cc_FILE *)NULL;
	if (stdout_p != (APTR)NULL)
		*((cc_FILE **)stdout_p) = (cc_FILE *)NULL;
	if (stderr_p != (APTR)NULL)
		*((cc_FILE **)stderr_p) = (cc_FILE *)NULL;

	if (!have_io)
		return 1;

	in_fh = Input();
	out_fh = Output();
	err_fh = out_fh;
	/*
	 * ErrorOutput() exists from dos.library V37.  When unavailable,
	 * stderr shares Output().
	 */
	if (DOSBase != (struct DosLibrary *)NULL
	    && ((struct Library *)DOSBase)->lib_Version >= 37) {
		err_fh = ErrorOutput();
		if (err_fh == (BPTR)NULL)
			err_fh = out_fh;
	}

	/*
	 * CLI Input/Output must not be Closed.  A WINDOW opened by
	 * wb_parse was SelectInput/Output'd and should be Closed later.
	 */
	if (wbm != (APTR)NULL)
		stdio_bit = 0;
	else
		stdio_bit = (short)CC_O_STDIO;

	fp_in = (cc_FILE *)NULL;
	fp_out = (cc_FILE *)NULL;
	fp_err = (cc_FILE *)NULL;
	fd_in = (cc_FileDesc *)NULL;
	fd_out = (cc_FileDesc *)NULL;

	if (in_fh != (BPTR)NULL) {
		fp_in = cc_make_std_stream(ud, in_fh, (short)(CC_O_RDONLY | stdio_bit), (cc_FileDesc *)NULL);
		if (fp_in != (cc_FILE *)NULL) {
			fd_in = (cc_FileDesc *)fp_in->_unit;
			if (stdin_p != (APTR)NULL)
				*((cc_FILE **)stdin_p) = fp_in;
		}
	}

	if (out_fh != (BPTR)NULL) {
		if (out_fh == in_fh && fd_in != (cc_FileDesc *)NULL) {
			fp_out = cc_make_std_stream(ud, out_fh, (short)(CC_O_WRONLY | stdio_bit), fd_in);
		} else {
			fp_out = cc_make_std_stream(ud, out_fh, (short)(CC_O_WRONLY | stdio_bit), (cc_FileDesc *)NULL);
		}
		if (fp_out != (cc_FILE *)NULL) {
			fd_out = (cc_FileDesc *)fp_out->_unit;
			if (stdout_p != (APTR)NULL)
				*((cc_FILE **)stdout_p) = fp_out;
		}
	}

	if (err_fh != (BPTR)NULL) {
		if (err_fh == out_fh && fp_out != (cc_FILE *)NULL) {
			fp_err = fp_out;
		} else if (err_fh == in_fh && fd_in != (cc_FileDesc *)NULL) {
			fp_err = cc_make_std_stream(ud, err_fh, (short)(CC_O_WRONLY | stdio_bit), fd_in);
		} else {
			fp_err = cc_make_std_stream(ud, err_fh, (short)(CC_O_WRONLY | stdio_bit), (cc_FileDesc *)NULL);
		}
		if (fp_err != (cc_FILE *)NULL && stderr_p != (APTR)NULL)
			*((cc_FILE **)stderr_p) = fp_err;
	}

	(void)fd_out;
	return 1;
}

STRPTR __ASM__ __SAVE_DS__ strerror(
	__REG__(d0, LONG err))
{
	/* Index 0 = "No Error"; 1..11 match CC_E* in cc_internal.h. */
	static char *msgs[12] = {
		"No Error",
		"File Not Found",
		"Bad File Handle",
		"Not Enough Memory",
		"File Exists",
		"Invalid Function Number",
		"Too Many Files Opened",
		"Not A Console Device",
		"Invalid Access Code",
		"Result Too Large",
		"Domain Error",
		"Interrupted System Call"
	};
	static char unknown[] = "Unknown Error";

	cc_check_break();

	if (err < 0 || err > 11)
		return (STRPTR)unknown;
	return (STRPTR)msgs[err];
}
