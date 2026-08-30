/*
 * dirent.c - POSIX opendir / readdir / closedir for cclib.library
 *
 * Built on dos Examine / ExNext (same stack as historic scdir, but returns
 * a reusable DIR handle instead of a single task-global AnchorPath).
 * DIR* is a session capability: membership on ud->open_dirs + magic.
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

static void
cc_dir_unlink(cc_task_UserData *ud, cc_DIR *dir)
{
	cc_DIR *p;
	cc_DIR *prev;

	if (ud == (cc_task_UserData *)NULL || dir == (cc_DIR *)NULL)
		return;

	prev = (cc_DIR *)NULL;
	p = (cc_DIR *)ud->open_dirs;
	while (p != (cc_DIR *)NULL)
	{
		if (p == dir)
		{
			if (prev == (cc_DIR *)NULL)
				ud->open_dirs = (APTR)p->next;
			else
				prev->next = p->next;
			return;
		}
		prev = p;
		p = p->next;
	}
}

APTR __ASM__ __SAVE_DS__ opendir(
	__REG__(a0, STRPTR name))
{
	cc_task_UserData *ud;
	cc_DIR *dir;
	BPTR lock;
	struct FileInfoBlock *fib;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL || name == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (APTR)NULL;
	}

	lock = Lock(name, ACCESS_READ);
	if (lock == NULL)
	{
		cc_set_errno(CC_ENOENT);
		return (APTR)NULL;
	}

	fib = (struct FileInfoBlock *)AllocMem(
		sizeof(struct FileInfoBlock), MEMF_CLEAR);
	if (fib == (struct FileInfoBlock *)NULL)
	{
		UnLock(lock);
		cc_set_errno(CC_ENOMEM);
		return (APTR)NULL;
	}

	if (!Examine(lock, fib) || fib->fib_DirEntryType < 0)
	{
		/* Files have negative fib_DirEntryType; directories are positive. */
		FreeMem(fib, sizeof(struct FileInfoBlock));
		UnLock(lock);
		cc_set_errno(CC_EINVAL);
		return (APTR)NULL;
	}

	dir = (cc_DIR *)AllocMem(sizeof(cc_DIR), MEMF_CLEAR);
	if (dir == (cc_DIR *)NULL)
	{
		FreeMem(fib, sizeof(struct FileInfoBlock));
		UnLock(lock);
		cc_set_errno(CC_ENOMEM);
		return (APTR)NULL;
	}

	dir->magic = CC_DIR_MAGIC;
	dir->lock = lock;
	dir->fib = fib;
	dir->at_entry = 0;
	dir->next = (cc_DIR *)ud->open_dirs;
	ud->open_dirs = (APTR)dir;
	return (APTR)dir;
}

APTR __ASM__ __SAVE_DS__ readdir(
	__REG__(a0, APTR dirp))
{
	cc_task_UserData *ud;
	cc_DIR *dir;
	LONG ok;
	LONG i;
	cc_check_break();

	ud = cc_get_ud();
	dir = cc_dir_from_list(ud, dirp);
	if (dir == (cc_DIR *)NULL)
		return (APTR)NULL;
	if (dir->fib == (struct FileInfoBlock *)NULL)
	{
		cc_set_errno(CC_EBADF);
		cc_panic(20L);
		return (APTR)NULL;
	}

	if (dir->at_entry == 0)
	{
		/*
		 * After Examine of the directory itself, ExNext yields the
		 * first child.  Do not return "." for the directory FIB.
		 */
		ok = ExNext(dir->lock, dir->fib);
		dir->at_entry = 1;
	}
	else
	{
		ok = ExNext(dir->lock, dir->fib);
	}

	if (!ok)
		return (APTR)NULL;

	i = 0;
	while (i < 107 && dir->fib->fib_FileName[i] != '\0')
	{
		dir->d_name[i] = dir->fib->fib_FileName[i];
		i++;
	}
	dir->d_name[i] = '\0';
	/* Return address of d_name; public struct dirent starts with d_name. */
	return (APTR)&dir->d_name[0];
}

LONG __ASM__ __SAVE_DS__ closedir(
	__REG__(a0, APTR dirp))
{
	cc_task_UserData *ud;
	cc_DIR *dir;
	cc_check_break();

	ud = cc_get_ud();
	dir = cc_dir_from_list(ud, dirp);
	if (dir == (cc_DIR *)NULL)
		return -1L;

	cc_dir_unlink(ud, dir);

	if (dir->fib != (struct FileInfoBlock *)NULL)
		FreeMem(dir->fib, sizeof(struct FileInfoBlock));
	if (dir->lock != NULL)
		UnLock(dir->lock);

	dir->magic = CC_DIR_POISON;
	FreeMem(dir, sizeof(cc_DIR));
	return 0L;
}
