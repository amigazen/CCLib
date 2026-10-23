/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * ccpanic_stdio.c - forged / NULL / double-close FILE* validation
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include "cctest_util.h"

void
ct_suite_panic_stdio(void)
{
	APTR fp;
	LONG c;
	LONG n;
	LONG hits;
	char buf[8];
	char out[16];
	char zsrc[1100];

	ct_suite_begin((STRPTR)"panic_stdio",
		(STRPTR)"FILE* validation / abort_func");

	/* NULL stream: EINVAL, no panic (not a forged capability). */
	errno = 0;
	ct_install_panic_hook();
	c = getc((APTR)NULL);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"getc_null",
		(STRPTR)"getc(NULL)",
		(STRPTR)"getc(NULL) must return EOF without panic",
		c == (LONG)(-1) && ct_panic_hits == 0L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"getc_null_errno",
		(STRPTR)"errno after getc(NULL)",
		(STRPTR)"getc(NULL) must set errno to EINVAL",
		(LONG)EINVAL, errno);

	errno = 0;
	n = fclose((APTR)NULL);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fclose_null",
		(STRPTR)"fclose(NULL)",
		(STRPTR)"fclose(NULL) must fail without panic",
		n == -1L && ct_panic_hits == 0L);
	ct_restore_panic_hook();

	/* Forged FILE*: panic + fail. */
	errno = 0;
	ct_install_panic_hook();
	c = getc((APTR)0x12345678L);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"getc_forged",
		(STRPTR)"getc(forged)",
		(STRPTR)"getc of a forged FILE* must panic and return EOF",
		c == (LONG)(-1) && ct_panic_hits == 1L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"getc_forged_errno",
		(STRPTR)"errno after getc(forged)",
		(STRPTR)"forged getc must set errno to EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	n = fclose((APTR)0x12345678L);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fclose_forged",
		(STRPTR)"fclose(forged)",
		(STRPTR)"fclose of a forged FILE* must panic and fail",
		n == -1L && ct_panic_hits == 2L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"fclose_forged_errno",
		(STRPTR)"errno after fclose(forged)",
		(STRPTR)"forged fclose must set errno to EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	n = (LONG)fread((APTR)buf, 1UL, 4UL, (APTR)0x12345678L);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fread_forged",
		(STRPTR)"fread(forged)",
		(STRPTR)"fread of a forged FILE* must panic and transfer 0",
		n == 0L && ct_panic_hits == 3L);

	errno = 0;
	n = fflush((APTR)0x12345678L);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fflush_forged",
		(STRPTR)"fflush(forged)",
		(STRPTR)"fflush of a forged FILE* must panic and fail",
		n == -1L && ct_panic_hits == 4L);
	ct_restore_panic_hook();

	/* fflush(NULL) is a historic no-op - must not panic. */
	ct_install_panic_hook();
	n = fflush((APTR)NULL);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fflush_null",
		(STRPTR)"fflush(NULL)",
		(STRPTR)"fflush(NULL) must complete without panic",
		ct_panic_hits == 0L);
	ct_restore_panic_hook();

	/* Double fclose after a real close. */
	fp = tmpfile();
	ct_check_ptr_nonnull((STRPTR)"panic_stdio", (STRPTR)"tmpfile_for_twice",
		(STRPTR)"tmpfile()",
		(STRPTR)"need a live FILE* for double-fclose",
		fp);
	if (fp != (APTR)NULL)
	{
		n = fclose(fp);
		ct_check_long((STRPTR)"panic_stdio", (STRPTR)"fclose_once",
			(STRPTR)"fclose(tmpfile)",
			(STRPTR)"first fclose must succeed",
			0L, n);

		errno = 0;
		ct_install_panic_hook();
		n = fclose(fp);
		ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fclose_twice",
			(STRPTR)"fclose again",
			(STRPTR)"second fclose must panic and fail",
			n == -1L && ct_panic_hits == 1L);
		ct_check_long((STRPTR)"panic_stdio", (STRPTR)"fclose_twice_errno",
			(STRPTR)"errno after double fclose",
			(STRPTR)"double fclose must set errno to EBADF",
			(LONG)EBADF, errno);

		c = getc(fp);
		ct_check_true((STRPTR)"panic_stdio", (STRPTR)"getc_after_fclose",
			(STRPTR)"getc after fclose",
			(STRPTR)"getc on a closed FILE* must panic and return EOF",
			c == (LONG)(-1) && ct_panic_hits == 2L);
		ct_restore_panic_hook();
	}

	/*
	 * %s is a string parameter, not a dump.  Unmapped pointer and a
	 * block with no NUL are programming errors: EINVAL + panic.
	 */
	out[0] = 'X';
	out[1] = '\0';
	errno = 0;
	ct_install_panic_hook();
	hits = ct_panic_hits;
	n = sprintf(out, (STRPTR)"%s", (STRPTR)1L);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"sprintf_s_unmapped",
		(STRPTR)"sprintf %%s of address 1",
		(STRPTR)"%%s of a TypeOfMem-rejected pointer must panic and fail",
		n == -1L && ct_panic_hits == hits + 1L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"sprintf_s_unmapped_errno",
		(STRPTR)"errno after %%s of address 1",
		(STRPTR)"unmapped %%s must set errno to EINVAL",
		(LONG)EINVAL, errno);

	memset(zsrc, 'Z', 1100UL);
	out[0] = 'X';
	out[1] = '\0';
	errno = 0;
	hits = ct_panic_hits;
	n = sprintf(out, (STRPTR)"%s", (STRPTR)zsrc);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"sprintf_s_nonstring",
		(STRPTR)"sprintf %%s of 1100 unterminated bytes",
		(STRPTR)"unbounded %%s without a NUL in the probe must panic and fail",
		n == -1L && ct_panic_hits == hits + 1L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"sprintf_s_nonstring_errno",
		(STRPTR)"errno after unterminated %%s",
		(STRPTR)"unterminated %%s must set errno to EINVAL",
		(LONG)EINVAL, errno);

	n = sprintf(out, (STRPTR)"alive");
	ct_check_string((STRPTR)"panic_stdio",
		(STRPTR)"sprintf_s_after_bad",
		(STRPTR)"sprintf after a rejected %%s",
		(STRPTR)"formatter state must survive a rejected %%s",
		(STRPTR)"alive",
		out);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"sprintf_s_after_bad_len",
		(STRPTR)"sprintf(\"alive\") after rejected %%s",
		(STRPTR)"later sprintf must return 5",
		5L, n);

	errno = 0;
	hits = ct_panic_hits;
	n = fscanf((APTR)1L, (STRPTR)"%ld", &n);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fscanf_forged",
		(STRPTR)"fscanf(forged, \"%%ld\")",
		(STRPTR)"forged FILE* must panic and return EOF",
		n == (LONG)(-1) && ct_panic_hits == hits + 1L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"fscanf_forged_errno",
		(STRPTR)"errno after fscanf(forged)",
		(STRPTR)"forged fscanf must set EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	hits = ct_panic_hits;
	n = fputs((STRPTR)1L, stdout);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"fputs_unmapped",
		(STRPTR)"fputs of address 1",
		(STRPTR)"unmapped fputs source must panic and fail",
		n == (LONG)(-1) && ct_panic_hits == hits + 1L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"fputs_unmapped_errno",
		(STRPTR)"errno after fputs(unmapped)",
		(STRPTR)"unmapped fputs must set EINVAL",
		(LONG)EINVAL, errno);

	errno = 0;
	hits = ct_panic_hits;
	(void)strcpy(out, (STRPTR)1L);
	ct_check_true((STRPTR)"panic_stdio", (STRPTR)"strcpy_unmapped",
		(STRPTR)"strcpy(buf, address 1)",
		(STRPTR)"unmapped strcpy source must panic",
		ct_panic_hits == hits + 1L);
	ct_check_long((STRPTR)"panic_stdio", (STRPTR)"strcpy_unmapped_errno",
		(STRPTR)"errno after strcpy(unmapped)",
		(STRPTR)"unmapped strcpy must set EINVAL",
		(LONG)EINVAL, errno);

	ct_restore_panic_hook();
}
