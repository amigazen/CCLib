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
	char buf[8];

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

	/* fflush(NULL) is a historic no-op — must not panic. */
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
}
