/*
 * ccpanic_dir.c - forged / NULL DIR* validation
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include <dirent.h>
#include "cctest_util.h"

void
ct_suite_panic_dir(void)
{
	DIR *dir;
	struct dirent *de;
	LONG n;

	ct_suite_begin((STRPTR)"panic_dir",
		(STRPTR)"DIR* membership / abort_func");

	/* NULL: soft EBADF, no panic. */
	errno = 0;
	ct_install_panic_hook();
	de = readdir((DIR *)NULL);
	ct_check_true((STRPTR)"panic_dir", (STRPTR)"readdir_null",
		(STRPTR)"readdir(NULL)",
		(STRPTR)"readdir(NULL) must fail without panic",
		de == (struct dirent *)NULL && ct_panic_hits == 0L);
	ct_restore_panic_hook();

	errno = 0;
	ct_install_panic_hook();
	n = closedir((DIR *)NULL);
	ct_check_true((STRPTR)"panic_dir", (STRPTR)"closedir_null",
		(STRPTR)"closedir(NULL)",
		(STRPTR)"closedir(NULL) must fail without panic",
		n != 0L && ct_panic_hits == 0L);
	ct_restore_panic_hook();

	/* Forged cookie: panic + EBADF. */
	errno = 0;
	ct_install_panic_hook();
	de = readdir((DIR *)0x12345678L);
	ct_check_true((STRPTR)"panic_dir", (STRPTR)"readdir_forged",
		(STRPTR)"readdir(forged)",
		(STRPTR)"readdir of a forged DIR* must panic",
		de == (struct dirent *)NULL && ct_panic_hits == 1L);
	ct_check_long((STRPTR)"panic_dir", (STRPTR)"readdir_forged_errno",
		(STRPTR)"errno after readdir(forged)",
		(STRPTR)"forged readdir must set errno to EBADF",
		(LONG)EBADF, (LONG)errno);
	ct_restore_panic_hook();

	errno = 0;
	ct_install_panic_hook();
	n = closedir((DIR *)0x12345678L);
	ct_check_true((STRPTR)"panic_dir", (STRPTR)"closedir_forged",
		(STRPTR)"closedir(forged)",
		(STRPTR)"closedir of a forged DIR* must panic",
		n != 0L && ct_panic_hits == 1L);
	ct_restore_panic_hook();

	/* Double closedir after a real open. */
	dir = opendir((STRPTR)"LIBS:");
	ct_check_ptr_nonnull((STRPTR)"panic_dir", (STRPTR)"opendir_for_twice",
		(STRPTR)"opendir(LIBS:)",
		(STRPTR)"need a live DIR* for double-closedir",
		(APTR)dir);
	if (dir != (DIR *)NULL)
	{
		n = closedir(dir);
		ct_check_long((STRPTR)"panic_dir", (STRPTR)"closedir_once",
			(STRPTR)"closedir(LIBS:)",
			(STRPTR)"first closedir must succeed",
			0L, n);

		errno = 0;
		ct_install_panic_hook();
		n = closedir(dir);
		ct_check_true((STRPTR)"panic_dir", (STRPTR)"closedir_twice",
			(STRPTR)"closedir again",
			(STRPTR)"second closedir must panic and fail",
			n != 0L && ct_panic_hits == 1L);
		ct_restore_panic_hook();
	}
}
