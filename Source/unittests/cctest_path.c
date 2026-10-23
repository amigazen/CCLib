/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_path.c - getcwd / chdir / realpath
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_path(void)
{
	char buf[256];
	char resolved[256];
	char home[256];
	STRPTR p;
	LONG rc;

	ct_suite_begin((STRPTR)"path", (STRPTR)"getcwd chdir realpath");

	p = getcwd(buf, (LONG)sizeof(buf));
	ct_check_ptr_nonnull((STRPTR)"path", (STRPTR)"getcwd_ok",
		(STRPTR)"getcwd(buf, sizeof buf)",
		(STRPTR)"getcwd must return the buffer on success",
		(APTR)p);
	ct_check_true((STRPTR)"path", (STRPTR)"getcwd_nonempty",
		(STRPTR)"getcwd result",
		(STRPTR)"getcwd must write a non-empty path",
		buf[0] != '\0');

	strcpy(home, buf);

	rc = chdir((STRPTR)"T:");
	ct_check_long((STRPTR)"path", (STRPTR)"chdir_t",
		(STRPTR)"chdir(\"T:\")",
		(STRPTR)"chdir to T: must succeed",
		0, rc);

	p = getcwd(buf, (LONG)sizeof(buf));
	ct_check_ptr_nonnull((STRPTR)"path", (STRPTR)"getcwd_after_chdir",
		(STRPTR)"getcwd after chdir(T:)",
		(STRPTR)"getcwd after chdir must succeed",
		(APTR)p);

	/* Restore prior directory when we still have its path. */
	if (home[0] != '\0')
		(void)chdir(home);

	p = realpath((STRPTR)"T:", resolved);
	ct_check_ptr_nonnull((STRPTR)"path", (STRPTR)"realpath_t",
		(STRPTR)"realpath(\"T:\", resolved)",
		(STRPTR)"realpath of T: must succeed into the caller buffer",
		(APTR)p);
	ct_check_true((STRPTR)"path", (STRPTR)"realpath_nonempty",
		(STRPTR)"realpath result",
		(STRPTR)"realpath must write a non-empty absolute path",
		resolved[0] != '\0');

	{
		LONG rc2;
		char dname[48];

		strcpy(dname, (STRPTR)"T:cctest_mkdir_zz");
		(void)rmdir(dname);
		rc2 = mkdir(dname, 0755L);
		ct_check_long((STRPTR)"path", (STRPTR)"mkdir_ok",
			(STRPTR)"mkdir(T:cctest_mkdir_zz)",
			(STRPTR)"mkdir must create a directory on T:",
			0, rc2);
		rc2 = rmdir(dname);
		ct_check_long((STRPTR)"path", (STRPTR)"rmdir_ok",
			(STRPTR)"rmdir after mkdir",
			(STRPTR)"rmdir must remove the directory just created",
			0, rc2);
	}
}
