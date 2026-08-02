/*
 * io.c - low-level file I/O for cclib.library
 *
 * open()/close() cheat by returning a FileDesc pointer in a LONG.
 * Descriptors are tracked on ud->OpenFiles via add_head/remv_node.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* Convert Amiga DateStamp to seconds since 1978-01-01 */
static LONG cc_datestamp_secs(struct DateStamp *ds)
{
	LONG secs;

	secs = ds->ds_Days * 86400L;
	secs += ds->ds_Minute * 60L;
	secs += ds->ds_Tick / 50L;
	return secs;
}

/* Map CC_O_* flags to a dos.library Open() mode */
static LONG cc_open_dosmode(ULONG flags)
{
	LONG dosmode;

	if (flags & CC_O_TRUNC)
	{
		dosmode = MODE_NEWFILE;
	}
	else if (flags & CC_O_CREAT)
	{
		/* Create if missing; keep existing contents unless O_TRUNC */
		dosmode = MODE_READWRITE;
	}
	else if (flags & CC_O_RDWR)
	{
		dosmode = MODE_READWRITE;
	}
	else
	{
		/* O_RDONLY or O_WRONLY on an existing file */
		dosmode = MODE_OLDFILE;
	}
	return dosmode;
}

LONG __ASM__ __SAVE_DS__ open(
	__REG__(a0, STRPTR name),
	__REG__(d0, ULONG mode),
	__REG__(d1, ULONG perms))
{
	cc_task_UserData *ud;
	cc_FileDesc *fd;
	BPTR fh;
	BPTR lock;
	LONG dosmode;
	ULONG flags;
	cc_check_break();

	(void)perms;

	ud = cc_get_ud();
	if (ud == NULL || name == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	flags = mode;

	/* O_EXCL with O_CREAT: fail if the object already exists */
	if ((flags & CC_O_CREAT) && (flags & CC_O_EXCL))
	{
		lock = Lock(name, ACCESS_READ);
		if (lock != NULL)
		{
			UnLock(lock);
			cc_set_errno(CC_EEXIST);
			return -1L;
		}
	}

	dosmode = cc_open_dosmode(flags);
	fh = Open(name, dosmode);
	if (fh == NULL)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}

	if (flags & CC_O_APPEND)
	{
		Seek(fh, 0L, OFFSET_END);
	}

	/* Soft open-file cap before registering (EMFILE). */
	if (ud->OpenFiles.length >= CC_MAX_OPEN_FILES)
	{
		Close(fh);
		cc_set_errno(CC_EMFILE);
		return -1L;
	}

	fd = (cc_FileDesc *)add_head((APTR)&ud->OpenFiles, (LONG)sizeof(cc_FileDesc));
	if (fd == NULL)
	{
		Close(fh);
		cc_set_errno(CC_ENOMEM);
		return -1L;
	}

	fd->fh = fh;
	fd->mode = (short)flags;
	fd->pad = 0;
	fd->magic = CC_FD_MAGIC;
	return (LONG)fd;
}

LONG __ASM__ __SAVE_DS__ close(
	__REG__(d0, LONG fd))
{
	cc_task_UserData *ud;
	cc_FileDesc *desc;
	cc_node *node;
	ULONG *hp;
	cc_check_break();

	ud = cc_get_ud();
	/* Validate membership before any dos Close (forged fd must not hit OS). */
	desc = cc_fd_from_list(ud, fd);
	if (desc == (cc_FileDesc *)NULL)
		return -1L;

	/* Stdio handles that wrap Input()/Output() must not be Closed */
	if (!(desc->mode & CC_O_STDIO))
	{
		if (desc->fh != NULL)
			Close(desc->fh);
	}
	desc->fh = NULL;
	desc->magic = CC_FD_POISON;

	/* remv_node unlinks only — free [ULONG][cc_node][data] block */
	node = (cc_node *)remv_node((APTR)&ud->OpenFiles, (APTR)desc);
	if (node != NULL)
	{
		hp = ((ULONG *)node) - 1;
		FreeMem(hp, *hp);
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ remove(
	__REG__(a0, STRPTR name))
{
	LONG ok;
	cc_check_break();

	if (name == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	ok = DeleteFile(name);
	if (!ok)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ rename(
	__REG__(a0, STRPTR oldname),
	__REG__(a1, STRPTR newname))
{
	LONG ok;
	cc_check_break();

	if (oldname == NULL || newname == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	ok = Rename(oldname, newname);
	if (!ok)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ read(
	__REG__(d0, LONG fd),
	__REG__(a0, APTR buf),
	__REG__(d1, LONG len))
{
	cc_task_UserData *ud;
	cc_FileDesc *desc;
	LONG n;
	cc_check_break();

	if (buf == NULL || len < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	ud = cc_get_ud();
	desc = cc_fd_from_list(ud, fd);
	if (desc == (cc_FileDesc *)NULL)
		return -1L;
	if (desc->fh == NULL)
	{
		cc_set_errno(CC_EBADF);
		return -1L;
	}

	n = Read(desc->fh, buf, len);
	if (n < 0L)
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}
	return n;
}

LONG __ASM__ __SAVE_DS__ write(
	__REG__(d0, LONG fd),
	__REG__(a0, APTR buf),
	__REG__(d1, LONG len))
{
	cc_task_UserData *ud;
	cc_FileDesc *desc;
	LONG n;
	cc_check_break();

	if (buf == NULL || len < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	ud = cc_get_ud();
	desc = cc_fd_from_list(ud, fd);
	if (desc == (cc_FileDesc *)NULL)
		return -1L;
	if (desc->fh == NULL)
	{
		cc_set_errno(CC_EBADF);
		return -1L;
	}

	n = Write(desc->fh, buf, len);
	if (n < 0L)
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}
	return n;
}

LONG __ASM__ __SAVE_DS__ creat(
	__REG__(a0, STRPTR name),
	__REG__(d0, ULONG perms))
{
	cc_check_break();
	return open(name, (ULONG)(CC_O_WRONLY | CC_O_CREAT | CC_O_TRUNC), perms);
}

LONG __ASM__ __SAVE_DS__ lseek(
	__REG__(d0, LONG fd),
	__REG__(d1, LONG offset),
	__REG__(d2, ULONG whence))
{
	cc_task_UserData *ud;
	cc_FileDesc *desc;
	LONG mode;
	LONG oldpos;
	LONG newpos;
	cc_check_break();

	ud = cc_get_ud();
	desc = cc_fd_from_list(ud, fd);
	if (desc == (cc_FileDesc *)NULL)
		return -1L;
	if (desc->fh == NULL)
	{
		cc_set_errno(CC_EBADF);
		return -1L;
	}

	if (whence == (ULONG)CC_SEEK_SET)
		mode = OFFSET_BEGINNING;
	else if (whence == (ULONG)CC_SEEK_CUR)
		mode = OFFSET_CURRENT;
	else if (whence == (ULONG)CC_SEEK_END)
		mode = OFFSET_END;
	else
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	/* Seek returns the previous position; ask again for the new one */
	oldpos = Seek(desc->fh, offset, mode);
	if (oldpos < 0L && IoErr() != 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	newpos = Seek(desc->fh, 0L, OFFSET_CURRENT);
	if (newpos < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	return newpos;
}

LONG __ASM__ __SAVE_DS__ isatty(
	__REG__(d0, LONG fd))
{
	cc_task_UserData *ud;
	cc_FileDesc *desc;
	cc_check_break();

	ud = cc_get_ud();
	desc = cc_fd_from_list(ud, fd);
	if (desc == (cc_FileDesc *)NULL)
		return 0L;
	if (desc->fh == NULL)
	{
		cc_set_errno(CC_EBADF);
		return 0L;
	}

	if (IsInteractive(desc->fh))
		return 1L;

	cc_set_errno(CC_ENOTTY);
	return 0L;
}

LONG __ASM__ __SAVE_DS__ access(
	__REG__(a0, STRPTR name),
	__REG__(d0, LONG mode))
{
	BPTR lock;
	struct FileInfoBlock *fib;
	LONG ok;
	ULONG prot;
	cc_check_break();

	if (name == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	lock = Lock(name, ACCESS_READ);
	if (lock == NULL)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}

	/* mode 0 = existence only */
	if (mode == 0L)
	{
		UnLock(lock);
		return 0L;
	}

	fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);
	if (fib == NULL)
	{
		UnLock(lock);
		cc_set_errno(CC_ENOMEM);
		return -1L;
	}

	ok = Examine(lock, fib);
	prot = (ULONG)fib->fib_Protection;
	FreeMem(fib, sizeof(struct FileInfoBlock));
	UnLock(lock);

	if (!ok)
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}

	/* Classic R_OK=4 W_OK=2 X_OK=1 — Amiga protection bits are inverted */
	if ((mode & 4L) && (prot & FIBF_READ))
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}
	if ((mode & 2L) && (prot & FIBF_WRITE))
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}
	if ((mode & 1L) && (prot & FIBF_EXECUTE))
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}
	return 0L;
}

/* Fill cc_stat from a locked name; full=TRUE also fills extended fields */
static LONG cc_fill_stat(STRPTR name, cc_stat *st, LONG full)
{
	BPTR lock;
	struct FileInfoBlock *fib;
	LONG ok;

	lock = Lock(name, ACCESS_READ);
	if (lock == NULL)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}

	fib = (struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock), MEMF_CLEAR);
	if (fib == NULL)
	{
		UnLock(lock);
		cc_set_errno(CC_ENOMEM);
		return -1L;
	}

	ok = Examine(lock, fib);
	UnLock(lock);
	if (!ok)
	{
		FreeMem(fib, sizeof(struct FileInfoBlock));
		cc_set_errno(CC_EACCES);
		return -1L;
	}

	st->st_attr = (UWORD)(fib->fib_Protection &
		(FIBF_DELETE | FIBF_EXECUTE | FIBF_WRITE | FIBF_READ | FIBF_ARCHIVE));
	st->st_mtime = cc_datestamp_secs(&fib->fib_Date);
	st->st_size = fib->fib_Size;

	if (full)
	{
		if (fib->fib_DirEntryType > 0)
			st->st_mode = (UWORD)CC_S_IFREG;
		else if (fib->fib_DirEntryType < 0)
			st->st_mode = (UWORD)CC_S_IFDIR;
		else
			st->st_mode = (UWORD)CC_S_IFREG;

		st->st_nlink = 1;
		st->st_atime = st->st_mtime;
		st->st_ctime = st->st_mtime;
	}
	else
	{
		st->st_mode = 0;
		st->st_nlink = 0;
		st->st_atime = 0;
		st->st_ctime = 0;
	}

	FreeMem(fib, sizeof(struct FileInfoBlock));
	return 0L;
}

LONG __ASM__ __SAVE_DS__ stat(
	__REG__(a0, STRPTR name),
	__REG__(a1, APTR buf))
{
	cc_check_break();
	if (name == NULL || buf == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	return cc_fill_stat(name, (cc_stat *)buf, 0L);
}

LONG __ASM__ __SAVE_DS__ stat_(
	__REG__(a0, STRPTR name),
	__REG__(a1, APTR buf))
{
	cc_check_break();
	if (name == NULL || buf == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	return cc_fill_stat(name, (cc_stat *)buf, 1L);
}

STRPTR __ASM__ __SAVE_DS__ getcwd(
	__REG__(a0, STRPTR buf),
	__REG__(d0, LONG size))
{
	struct Process *pr;
	BPTR lock;
	STRPTR out;
	LONG alloced;
	cc_check_break();

	if (size < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}

	alloced = 0;
	out = buf;
	if (out == (STRPTR)NULL)
	{
		if (size == 0L)
			size = CC_PATH_MAX;
		out = (STRPTR)malloc((ULONG)size);
		if (out == (STRPTR)NULL)
		{
			cc_set_errno(CC_ENOMEM);
			return (STRPTR)NULL;
		}
		alloced = 1;
	}
	if (size == 0L)
	{
		cc_set_errno(CC_EINVAL);
		if (alloced)
			free(out);
		return (STRPTR)NULL;
	}

	pr = (struct Process *)FindTask((STRPTR)NULL);
	lock = pr->pr_CurrentDir;
	if (!NameFromLock(lock, out, size))
	{
		cc_set_errno(CC_ERANGE);
		if (alloced)
			free(out);
		return (STRPTR)NULL;
	}
	return out;
}

LONG __ASM__ __SAVE_DS__ chdir(
	__REG__(a0, STRPTR path))
{
	BPTR lock;
	BPTR old;
	cc_check_break();

	if (path == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	lock = Lock(path, ACCESS_READ);
	if (lock == NULL)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}

	old = CurrentDir(lock);
	if (old != NULL)
		UnLock(old);
	return 0L;
}

STRPTR __ASM__ __SAVE_DS__ realpath(
	__REG__(a0, STRPTR path),
	__REG__(a1, STRPTR resolved))
{
	BPTR lock;
	STRPTR out;
	LONG alloced;
	cc_check_break();

	if (path == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}

	alloced = 0;
	out = resolved;
	if (out == (STRPTR)NULL)
	{
		out = (STRPTR)malloc((ULONG)CC_PATH_MAX);
		if (out == (STRPTR)NULL)
		{
			cc_set_errno(CC_ENOMEM);
			return (STRPTR)NULL;
		}
		alloced = 1;
	}

	lock = Lock(path, ACCESS_READ);
	if (lock == NULL)
	{
		cc_set_errno(CC_ENOENT);
		if (alloced)
			free(out);
		return (STRPTR)NULL;
	}

	if (!NameFromLock(lock, out, CC_PATH_MAX))
	{
		UnLock(lock);
		cc_set_errno(CC_ERANGE);
		if (alloced)
			free(out);
		return (STRPTR)NULL;
	}
	UnLock(lock);
	return out;
}

LONG __ASM__ __SAVE_DS__ mkdir(
	__REG__(a0, STRPTR path),
	__REG__(d0, LONG mode))
{
	BPTR lock;
	cc_check_break();

	(void)mode; /* Amiga CreateDir has no POSIX mode bits */

	if (path == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	lock = CreateDir(path);
	if (lock == NULL)
	{
		cc_set_errno(CC_EEXIST);
		return -1L;
	}
	UnLock(lock);
	return 0L;
}

LONG __ASM__ __SAVE_DS__ rmdir(
	__REG__(a0, STRPTR path))
{
	BPTR lock;
	struct FileInfoBlock *fib;
	LONG ok;
	cc_check_break();

	if (path == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	lock = Lock(path, ACCESS_READ);
	if (lock == NULL)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}

	fib = (struct FileInfoBlock *)AllocMem(
		sizeof(struct FileInfoBlock), MEMF_CLEAR);
	if (fib == (struct FileInfoBlock *)NULL)
	{
		UnLock(lock);
		cc_set_errno(CC_ENOMEM);
		return -1L;
	}

	ok = Examine(lock, fib);
	UnLock(lock);
	if (!ok || fib->fib_DirEntryType < 0)
	{
		FreeMem(fib, sizeof(struct FileInfoBlock));
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	FreeMem(fib, sizeof(struct FileInfoBlock));

	if (!DeleteFile(path))
	{
		cc_set_errno(CC_EACCES);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ chmod(
	__REG__(a0, STRPTR path),
	__REG__(d0, LONG mode))
{
	ULONG prot;
	cc_check_break();

	if (path == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	/*
	 * Map low 9 Unix permission bits onto inverted Amiga FIBF_* flags.
	 * Owner rwx → absence of FIBF_READ/WRITE/EXECUTE.
	 */
	prot = 0UL;
	if ((mode & 0400L) == 0L)
		prot |= (ULONG)FIBF_READ;
	if ((mode & 0200L) == 0L)
		prot |= (ULONG)FIBF_WRITE;
	if ((mode & 0100L) == 0L)
		prot |= (ULONG)FIBF_EXECUTE;

	if (!SetProtection(path, (LONG)prot))
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ utime(
	__REG__(a0, STRPTR path),
	__REG__(a1, APTR times))
{
	struct DateStamp ds;
	LONG *tp;
	LONG secs;
	cc_check_break();

	if (path == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	/* struct utimbuf { long actime; long modtime; } — Amiga epoch seconds. */
	if (times == (APTR)NULL)
	{
		DateStamp(&ds);
	}
	else
	{
		tp = (LONG *)times;
		secs = tp[1]; /* modtime */
		if (secs < 0L)
			secs = 0L;
		ds.ds_Days = secs / 86400L;
		secs = secs % 86400L;
		ds.ds_Minute = secs / 60L;
		ds.ds_Tick = (secs % 60L) * 50L;
	}

	if (!SetFileDate(path, &ds))
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ symlink(
	__REG__(a0, STRPTR target),
	__REG__(a1, STRPTR linkpath))
{
	cc_check_break();
	if (target == (STRPTR)NULL || linkpath == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	/* soft != 0: dest is a pointer to the link text (dos MakeLink). */
	if (!MakeLink(linkpath, (LONG)target, 1L))
	{
		cc_set_errno(CC_EEXIST);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ readlink(
	__REG__(a0, STRPTR path),
	__REG__(a1, STRPTR buf),
	__REG__(d0, LONG bufsiz))
{
	struct DevProc *dp;
	STRPTR namepart;
	LONG ok;
	LONG len;
	cc_check_break();

	if (path == (STRPTR)NULL || buf == (STRPTR)NULL || bufsiz <= 0L)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	dp = GetDeviceProc(path, (struct DevProc *)NULL);
	if (dp == (struct DevProc *)NULL)
	{
		cc_set_errno(CC_ENOENT);
		return -1L;
	}

	namepart = FilePart(path);
	ok = ReadLink(dp->dvp_Port, dp->dvp_Lock, namepart, buf, (ULONG)bufsiz);
	FreeDeviceProc(dp);
	if (!ok)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	len = 0L;
	while (len < bufsiz && buf[len] != '\0')
		len++;
	return len;
}
