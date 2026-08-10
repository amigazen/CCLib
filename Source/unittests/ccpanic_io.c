/*
 * ccpanic_io.c - sentinel fds (EBADF, no panic) and forged/stale fds
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include <stat.h>
#include "cctest_util.h"

#define CT_O_RDWR   2
#define CT_O_CREAT  (1 << 8)
#define CT_O_TRUNC  (1 << 9)

void
ct_suite_panic_io(void)
{
	LONG fd;
	LONG n;
	char buf[8];
	char path[40];
	struct stat sb;

	ct_suite_begin((STRPTR)"panic_io",
		(STRPTR)"fd validation / abort_func");

	strcpy(path, (STRPTR)"T:ccpanic_io");
	remove(path);

	/*
	 * Sentinels (-1 / 0): soft EBADF, no abort_func.  These are not
	 * forged capabilities - they never entered OpenFiles.
	 */
	errno = 0;
	ct_install_panic_hook();
	n = close(-1L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"close_neg1",
		(STRPTR)"close(-1)",
		(STRPTR)"close(-1) must fail with -1",
		-1L, n);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"close_neg1_no_panic",
		(STRPTR)"abort_func after close(-1)",
		(STRPTR)"sentinel close(-1) must not call abort_func",
		ct_panic_hits == 0L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"close_neg1_errno",
		(STRPTR)"errno after close(-1)",
		(STRPTR)"close(-1) must set errno to EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	n = close(0L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"close_zero",
		(STRPTR)"close(0)",
		(STRPTR)"close(0) must fail with -1",
		-1L, n);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"close_zero_no_panic",
		(STRPTR)"abort_func after close(0)",
		(STRPTR)"sentinel close(0) must not call abort_func",
		ct_panic_hits == 0L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"close_zero_errno",
		(STRPTR)"errno after close(0)",
		(STRPTR)"close(0) must set errno to EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	n = read(-1L, (APTR)buf, 1L);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"read_neg1",
		(STRPTR)"read(-1)",
		(STRPTR)"read(-1) must fail without panic",
		n == -1L && ct_panic_hits == 0L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"read_neg1_errno",
		(STRPTR)"errno after read(-1)",
		(STRPTR)"read(-1) must set errno to EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	n = write(0L, (APTR)"x", 1L);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"write_zero",
		(STRPTR)"write(0)",
		(STRPTR)"write(0) must fail without panic",
		n == -1L && ct_panic_hits == 0L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"write_zero_errno",
		(STRPTR)"errno after write(0)",
		(STRPTR)"write(0) must set errno to EBADF",
		(LONG)EBADF, errno);
	ct_restore_panic_hook();

	/* Forged fd pointer: panic before any dos call. */
	errno = 0;
	ct_install_panic_hook();
	n = read(0x12345678L, (APTR)buf, 1L);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"read_forged_fd",
		(STRPTR)"read(forged fd)",
		(STRPTR)"forged fd must panic and fail without calling dos",
		n == -1L && ct_panic_hits == 1L);
	ct_check_long((STRPTR)"panic_io", (STRPTR)"read_forged_errno",
		(STRPTR)"errno after read(forged)",
		(STRPTR)"forged read must set errno to EBADF",
		(LONG)EBADF, errno);

	errno = 0;
	n = write(0x12345678L, (APTR)"x", 1L);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"write_forged_fd",
		(STRPTR)"write(forged fd)",
		(STRPTR)"forged write must panic and fail",
		n == -1L && ct_panic_hits == 2L);

	errno = 0;
	n = close(0x12345678L);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"close_forged_fd",
		(STRPTR)"close(forged fd)",
		(STRPTR)"forged close must panic and fail",
		n == -1L && ct_panic_hits == 3L);
	ct_restore_panic_hook();

	/* Double close + use-after-close on a real fd. */
	fd = open(path, (ULONG)(CT_O_RDWR | CT_O_CREAT | CT_O_TRUNC), 0);
	ct_check_true((STRPTR)"panic_io", (STRPTR)"open_for_stale",
		(STRPTR)"open for stale-fd tests",
		(STRPTR)"open must succeed so close can leave a stale capability",
		fd >= 0);
	if (fd >= 0)
	{
		n = close(fd);
		ct_check_long((STRPTR)"panic_io", (STRPTR)"close_once",
			(STRPTR)"close(fd)",
			(STRPTR)"first close must return 0",
			0L, n);

		errno = 0;
		ct_install_panic_hook();
		n = close(fd);
		ct_check_true((STRPTR)"panic_io", (STRPTR)"close_twice",
			(STRPTR)"close(fd) again",
			(STRPTR)"second close must panic and fail",
			n == -1L && ct_panic_hits == 1L);
		ct_check_long((STRPTR)"panic_io", (STRPTR)"close_twice_errno",
			(STRPTR)"errno after double close",
			(STRPTR)"double close must set errno to EBADF",
			(LONG)EBADF, errno);

		ct_panic_hits = 0;
		errno = 0;
		n = write(fd, (APTR)"x", 1L);
		ct_check_true((STRPTR)"panic_io", (STRPTR)"write_after_close",
			(STRPTR)"write after close",
			(STRPTR)"write on a closed fd must panic and fail",
			n == -1L && ct_panic_hits == 1L);

		ct_panic_hits = 0;
		n = read(fd, (APTR)buf, 1L);
		ct_check_true((STRPTR)"panic_io", (STRPTR)"read_after_close",
			(STRPTR)"read after close",
			(STRPTR)"read on a closed fd must panic and fail",
			n == -1L && ct_panic_hits == 1L);

		ct_panic_hits = 0;
		n = lseek(fd, 0L, 0);
		ct_check_true((STRPTR)"panic_io", (STRPTR)"lseek_after_close",
			(STRPTR)"lseek after close",
			(STRPTR)"lseek on a closed fd must panic and fail",
			n == -1L && ct_panic_hits == 1L);

		ct_panic_hits = 0;
		errno = 0;
		memset((APTR)&sb, 0, (LONG)sizeof(sb));
		n = fstat(fd, &sb);
		ct_check_true((STRPTR)"panic_io", (STRPTR)"fstat_after_close",
			(STRPTR)"fstat after close",
			(STRPTR)"fstat on a closed fd must panic and fail",
			n == -1L && ct_panic_hits == 1L);
		ct_check_long((STRPTR)"panic_io", (STRPTR)"fstat_after_close_errno",
			(STRPTR)"errno after fstat on closed fd",
			(STRPTR)"stale fstat must set errno to EBADF",
			(LONG)EBADF, errno);
		ct_restore_panic_hook();

		remove(path);
	}
}
