/*
 * env.c - environment variables and directory scanning for cclib.library
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosasl.h>
#include <dos/var.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* Path limits for scdir / getenv scratch (also in cc_internal.h). */
/* Convert shell-style * / ? wildcards into AmigaDOS #? / ? patterns */
static void cc_shell_to_amiga_pattern(STRPTR src, STRPTR dst, LONG dstlen)
{
	LONG i;
	LONG j;

	i = 0L;
	j = 0L;
	while (src[i] != '\0' && j < dstlen - 3L)
	{
		if (src[i] == '*')
		{
			dst[j++] = '#';
			dst[j++] = '?';
			i++;
		}
		else
		{
			dst[j++] = src[i++];
		}
	}
	dst[j] = '\0';
}

STRPTR __ASM__ __SAVE_DS__ scdir(
	__REG__(a0, STRPTR pattern))
{
	cc_task_UserData *ud;
	struct AnchorPath *ap;
	LONG err;
	char amiga_pat[128];
	ULONG apsize;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == NULL)
		return NULL;

	apsize = sizeof(struct AnchorPath) + CC_SCDIR_PATHLEN;

	if (ud->scdir_mem == NULL)
	{
		if (pattern == NULL)
			return NULL;

		ap = (struct AnchorPath *)AllocMem(apsize, MEMF_CLEAR);
		if (ap == NULL)
		{
			cc_set_errno(CC_ENOMEM);
			return NULL;
		}

		ap->ap_Strlen = (UBYTE)CC_SCDIR_PATHLEN;
		ap->ap_BreakBits = SIGBREAKF_CTRL_C;
		ap->ap_Flags = 0;

		cc_shell_to_amiga_pattern(pattern, amiga_pat, (LONG)sizeof(amiga_pat));
		err = MatchFirst(amiga_pat, ap);
		if (err != 0L)
		{
			MatchEnd(ap);
			FreeMem(ap, apsize);
			return NULL;
		}

		ud->scdir_mem = (APTR)ap;
		return ap->ap_Info.fib_FileName;
	}

	/* Continue an existing scan - pattern ignored after first call */
	ap = (struct AnchorPath *)ud->scdir_mem;
	err = MatchNext(ap);
	if (err != 0L)
	{
		MatchEnd(ap);
		FreeMem(ap, apsize);
		ud->scdir_mem = NULL;
		return NULL;
	}
	return ap->ap_Info.fib_FileName;
}

void __ASM__ __SAVE_DS__ scdir_clean(void)
{
	cc_task_UserData *ud;
	struct AnchorPath *ap;
	ULONG apsize;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == NULL || ud->scdir_mem == NULL)
		return;

	ap = (struct AnchorPath *)ud->scdir_mem;
	apsize = sizeof(struct AnchorPath) + CC_SCDIR_PATHLEN;
	MatchEnd(ap);
	FreeMem(ap, apsize);
	ud->scdir_mem = NULL;
}

LONG __ASM__ __SAVE_DS__ setenv(
	__REG__(a0, STRPTR name),
	__REG__(a1, STRPTR value))
{
	LONG ok;

	/*

	 * Wrap dos SetVar / DeleteVar (see dos.library autodocs).
	 * Prefer GVF_LOCAL_ONLY - SetVar's default is local-to-process.
	 */
	cc_check_break();
	if (name == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	/* NULL or empty value deletes (Amiga SETENV / POSIX unset-like). */
	if (value == NULL || value[0] == '\0')
	{
		ok = DeleteVar(name, GVF_LOCAL_ONLY);
		if (!ok)
			ok = DeleteVar(name, GVF_GLOBAL_ONLY);
		if (!ok)
		{
			cc_set_errno(CC_ENOENT);
			return -1L;
		}
		return 0L;
	}

	ok = SetVar(name, value, -1L, GVF_LOCAL_ONLY);
	if (!ok)
	{
		cc_set_errno(CC_ENOMEM);
		return -1L;
	}
	return 0L;
}

LONG __ASM__ __SAVE_DS__ unsetenv(
	__REG__(a0, STRPTR name))
{
	/*

	 * POSIX unsetenv via DeleteVar.  Drop the local var first; also
	 * clear a global ENV: copy so getenv's default lookup cannot see it.
	 */
	cc_check_break();
	if (name == NULL || name[0] == '\0')
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	(void)DeleteVar(name, GVF_LOCAL_ONLY);
	(void)DeleteVar(name, GVF_GLOBAL_ONLY);
	return 0L;
}

STRPTR __ASM__ __SAVE_DS__ getenv(
	__REG__(a0, STRPTR name))
{
	cc_task_UserData *ud;
	STRPTR buf;
	LONG len;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == NULL || name == NULL)
		return NULL;

	/* Free the previous getenv buffer */
	if (ud->getenv_mem != NULL)
	{
		FreeMem(ud->getenv_mem, CC_GETENV_BUFLEN);
		ud->getenv_mem = NULL;
	}

	buf = (STRPTR)AllocMem(CC_GETENV_BUFLEN, MEMF_CLEAR);
	if (buf == NULL)
	{
		cc_set_errno(CC_ENOMEM);
		return NULL;
	}

	/*
	 * Wrap GetVar with flags 0: local process var first, then global
	 * ENV: (dos.library GetVar default).
	 */
	len = GetVar(name, buf, CC_GETENV_BUFLEN - 1L, 0UL);
	if (len < 0L)
	{
		FreeMem(buf, CC_GETENV_BUFLEN);
		return NULL;
	}

	buf[len] = '\0';
	ud->getenv_mem = (APTR)buf;
	return buf;
}
