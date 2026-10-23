/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_io.c - low-level open/read/write/lseek/close/remove/rename/stat/access
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <stat.h>
#include "cctest_util.h"

/* Historic O_* from CClib fcntl */
#define CT_O_RDONLY  0
#define CT_O_WRONLY  1
#define CT_O_RDWR    2
#define CT_O_CREAT   (1<<8)
#define CT_O_TRUNC   (1<<9)

void
ct_suite_io(void)
{
	LONG fd;
	LONG fd2;
	LONG n;
	LONG pos;
	LONG st;
	LONG i;
	char buf[64];
	char bigbuf[256];
	char rdbuf[256];
	char path1[40];
	char path2[40];
	char path3[40];
	struct stat sb;

	ct_suite_begin((STRPTR)"io", (STRPTR)"open / read / write / lseek / stat");

	strcpy(path1, (STRPTR)"T:cctest_io_1");
	strcpy(path2, (STRPTR)"T:cctest_io_2");
	strcpy(path3, (STRPTR)"T:cctest_io_3");
	remove(path1);
	remove(path2);
	remove(path3);

	fd = open(path1, (ULONG)(CT_O_WRONLY | CT_O_CREAT | CT_O_TRUNC), 0);
	ct_check_true((STRPTR)"io", (STRPTR)"open_create_write",
		(STRPTR)"open(T:cctest_io_1, WRONLY|CREAT|TRUNC)",
		(STRPTR)"open(T:cctest_io_1, WRONLY|CREAT|TRUNC) must return a non-negative fd",
		fd >= 0);
	if (fd < 0)
		return;

	n = write(fd, (APTR)"IOTEST", 6);
	ct_check_long((STRPTR)"io", (STRPTR)"write_6",
		(STRPTR)"write",
		(STRPTR)"write must report 6 bytes written",
		6, n);

	n = close(fd);
	ct_check_long((STRPTR)"io", (STRPTR)"close_writer",
		(STRPTR)"close of the write fd",
		(STRPTR)"close of the write fd must return 0",
		0, n);

	fd = open(path1, (ULONG)CT_O_RDONLY, 0);
	ct_check_true((STRPTR)"io", (STRPTR)"open_read",
		(STRPTR)"Re-open of the same path for reading",
		(STRPTR)"Re-open of the same path for reading must succeed",
		fd >= 0);
	if (fd < 0)
		return;

	memset(buf, 0, sizeof(buf));
	n = read(fd, buf, 6);
	ct_check_long((STRPTR)"io", (STRPTR)"read_6",
		(STRPTR)"read",
		(STRPTR)"read must return 6 bytes from the file written earlier",
		6, n);
	buf[6] = '\0';
	ct_check_string((STRPTR)"io",
		(STRPTR)"read_content",
		(STRPTR)"File contents",
		(STRPTR)"File contents must be the six characters IOTEST",
		(STRPTR)"IOTEST",
		buf);

	pos = lseek(fd, 0, 0);
	ct_check_long((STRPTR)"io", (STRPTR)"lseek_set",
		(STRPTR)"lseek to offset 0 from SEEK_SET",
		(STRPTR)"lseek to offset 0 from SEEK_SET must return 0",
		0, pos);

	pos = lseek(fd, 2, 0);
	ct_check_long((STRPTR)"io", (STRPTR)"lseek_2",
		(STRPTR)"lseek to offset 2",
		(STRPTR)"lseek to offset 2 must return 2",
		2, pos);
	memset(buf, 0, sizeof(buf));
	n = read(fd, buf, 4);
	buf[4] = '\0';
	ct_check_string((STRPTR)"io",
		(STRPTR)"read_after_lseek",
		(STRPTR)"After lseek(2), next 4 bytes",
		(STRPTR)"After lseek(2), next 4 bytes must be \"TEST\"",
		(STRPTR)"TEST",
		buf);

	pos = lseek(fd, 0, 2);
	ct_check_true((STRPTR)"io", (STRPTR)"lseek_end",
		(STRPTR)"lseek SEEK_END",
		(STRPTR)"lseek SEEK_END must return a position >= 6",
		pos >= 6);

	pos = lseek(fd, -2, 1);
	ct_check_true((STRPTR)"io", (STRPTR)"lseek_cur_back",
		(STRPTR)"lseek -2 SEEK_CUR from end",
		(STRPTR)"lseek -2 SEEK_CUR from end must move back two bytes",
		pos >= 4);
	memset(buf, 0, sizeof(buf));
	n = read(fd, buf, 2);
	buf[2] = '\0';
	ct_check_string((STRPTR)"io",
		(STRPTR)"read_after_lseek_cur",
		(STRPTR)"Last two bytes of IOTEST",
		(STRPTR)"Last two bytes of IOTEST must be \"ST\"",
		(STRPTR)"ST",
		buf);

	n = isatty(fd);
	ct_check_false((STRPTR)"io", (STRPTR)"isatty_file",
		(STRPTR)"isatty on a disk file descriptor",
		(STRPTR)"isatty on a disk file descriptor must be false (0)",
		n);

	memset((APTR)&sb, 0, (LONG)sizeof(sb));
	st = fstat(fd, &sb);
	ct_check_long((STRPTR)"io", (STRPTR)"fstat_open",
		(STRPTR)"fstat on an open disk fd",
		(STRPTR)"fstat on an open disk fd must return 0",
		0, st);
	ct_check_true((STRPTR)"io", (STRPTR)"fstat_size_ge_6",
		(STRPTR)"fstat.st_size",
		(STRPTR)"fstat.st_size must be at least 6 for IOTEST",
		sb.st_size >= 6);

	close(fd);

	/* Sentinel fd: soft EBADF, no panic (stale closed fds are CCPanic). */
	st = fstat(-1, &sb);
	ct_check_true((STRPTR)"io", (STRPTR)"fstat_neg1",
		(STRPTR)"fstat(-1)",
		(STRPTR)"fstat on sentinel -1 must fail without abort_func",
		st < 0);

	st = access(path1, 0);
	ct_check_long((STRPTR)"io", (STRPTR)"access_exists",
		(STRPTR)"access(path,0)",
		(STRPTR)"access(path,0) must return 0 when the file exists",
		0, st);

	memset((APTR)&sb, 0, (LONG)sizeof(sb));
	st = stat_(path1, (APTR)&sb);
	ct_check_long((STRPTR)"io", (STRPTR)"stat_exists",
		(STRPTR)"stat_ on an existing file",
		(STRPTR)"stat_ on an existing file must return 0",
		0, st);
	ct_check_true((STRPTR)"io", (STRPTR)"stat_size_ge_6",
		(STRPTR)"stat_.st_size",
		(STRPTR)"stat_.st_size must be at least 6 for IOTEST",
		sb.st_size >= 6);

	st = rename(path1, path2);
	ct_check_long((STRPTR)"io", (STRPTR)"rename_ok",
		(STRPTR)"rename of the test file to a new T: name",
		(STRPTR)"rename of the test file to a new T: name must return 0",
		0, st);

	st = access(path1, 0);
	ct_check_true((STRPTR)"io", (STRPTR)"access_old_gone",
		(STRPTR)"Old path",
		(STRPTR)"Old path must no longer be accessible after rename",
		st != 0);

	st = access(path2, 0);
	ct_check_long((STRPTR)"io", (STRPTR)"access_new_exists",
		(STRPTR)"New path",
		(STRPTR)"New path must exist after rename",
		0, st);

	memset((APTR)&sb, 0, (LONG)sizeof(sb));
	st = stat_(path2, (APTR)&sb);
	ct_check_long((STRPTR)"io", (STRPTR)"stat_after_rename",
		(STRPTR)"stat_ on renamed file",
		(STRPTR)"stat_ on renamed file must return 0",
		0, st);
	ct_check_true((STRPTR)"io", (STRPTR)"stat_renamed_size",
		(STRPTR)"stat_.st_size after rename",
		(STRPTR)"stat_.st_size after rename must still be at least 6",
		sb.st_size >= 6);

	fd = open(path2, (ULONG)CT_O_RDWR, 0);
	ct_check_true((STRPTR)"io", (STRPTR)"open_rdwr",
		(STRPTR)"open with O_RDWR on an existing file",
		(STRPTR)"open with O_RDWR on an existing file must succeed",
		fd >= 0);
	if (fd >= 0)
	{
		pos = lseek(fd, 0, 2);
		ct_check_true((STRPTR)"io", (STRPTR)"lseek_rdwr_end",
		(STRPTR)"lseek SEEK_END on RDWR fd",
			(STRPTR)"lseek SEEK_END on RDWR fd must succeed",
			pos >= 0);
		n = write(fd, (APTR)"!", 1);
		ct_check_long((STRPTR)"io", (STRPTR)"write_rdwr_append",
		(STRPTR)"write one byte at end in RDWR mode",
			(STRPTR)"write one byte at end in RDWR mode must report 1",
			1, n);
		lseek(fd, 0, 0);
		memset(buf, 0, sizeof(buf));
		n = read(fd, buf, 7);
		buf[7] = '\0';
		ct_check_string((STRPTR)"io",
		(STRPTR)"read_rdwr_roundtrip",
		(STRPTR)"RDWR read after append",
		(STRPTR)"RDWR read after append must see \"IOTEST!\"",
		(STRPTR)"IOTEST!",
		buf);
		close(fd);
	}

	for (i = 0; i < 256; i++)
		bigbuf[i] = (char)('A' + (i % 26));
	bigbuf[255] = '\0';

	fd = open(path3, (ULONG)(CT_O_WRONLY | CT_O_CREAT | CT_O_TRUNC), 0);
	ct_check_true((STRPTR)"io", (STRPTR)"open_big_write",
		(STRPTR)"open for big-buffer write",
		(STRPTR)"open for big-buffer write must succeed",
		fd >= 0);
	if (fd >= 0)
	{
		n = write(fd, (APTR)bigbuf, 128);
		ct_check_long((STRPTR)"io", (STRPTR)"write_big_128",
		(STRPTR)"write of 128 bytes",
			(STRPTR)"write of 128 bytes must report 128",
			128, n);
		close(fd);

		fd = open(path3, (ULONG)CT_O_RDONLY, 0);
		ct_check_true((STRPTR)"io", (STRPTR)"open_big_read",
		(STRPTR)"open for big-buffer read",
			(STRPTR)"open for big-buffer read must succeed",
			fd >= 0);
		if (fd >= 0)
		{
			memset(rdbuf, 0, sizeof(rdbuf));
			n = read(fd, rdbuf, 64);
			ct_check_long((STRPTR)"io", (STRPTR)"read_big_partial",
		(STRPTR)"first read of 64 bytes",
				(STRPTR)"first read of 64 bytes must return 64",
				64, n);
			n = read(fd, rdbuf + 64, 64);
			ct_check_long((STRPTR)"io", (STRPTR)"read_big_second",
		(STRPTR)"second read of 64 bytes",
				(STRPTR)"second read of 64 bytes must return 64",
				64, n);
			ct_check_true((STRPTR)"io", (STRPTR)"read_big_match",
		(STRPTR)"128-byte read",
				(STRPTR)"128-byte read must match what was written",
				memcmp((APTR)bigbuf, (APTR)rdbuf, 128) == 0);
			pos = lseek(fd, 200, 0);
			ct_check_true((STRPTR)"io", (STRPTR)"lseek_past",
		(STRPTR)"lseek past end of 128-byte file",
				(STRPTR)"lseek past EOF may return the new offset or fail (-1) on AmigaDOS",
				pos == 200 || pos < 0);
			n = read(fd, buf, 16);
			ct_check_long((STRPTR)"io", (STRPTR)"read_past_eof",
		(STRPTR)"read after lseek past EOF",
				(STRPTR)"read after lseek past EOF must return 0 bytes",
				0, n);
			close(fd);
		}
	}

	fd = creat(path1, 0);
	ct_check_true((STRPTR)"io", (STRPTR)"creat_ok",
		(STRPTR)"creat",
		(STRPTR)"creat must create/truncate a file and return a writable fd",
		fd >= 0);
	if (fd >= 0)
	{
		n = write(fd, (APTR)"C", 1);
		ct_check_long((STRPTR)"io", (STRPTR)"creat_write_1",
		(STRPTR)"write after creat",
			(STRPTR)"write after creat must report 1 byte",
			1, n);
		close(fd);
		st = access(path1, 0);
		ct_check_long((STRPTR)"io", (STRPTR)"access_after_creat",
		(STRPTR)"access after creat",
			(STRPTR)"access after creat must return 0",
			0, st);
	}

	n = remove(path1);
	ct_check_long((STRPTR)"io", (STRPTR)"remove_1",
		(STRPTR)"remove of path1",
		(STRPTR)"remove of path1 must return 0",
		0, n);
	n = remove(path2);
	ct_check_long((STRPTR)"io", (STRPTR)"remove_2",
		(STRPTR)"remove of path2",
		(STRPTR)"remove of path2 must return 0",
		0, n);
	n = unlink(path3);
	ct_check_long((STRPTR)"io", (STRPTR)"unlink_3",
		(STRPTR)"unlink of path3",
		(STRPTR)"unlink (alias of remove) of path3 must return 0",
		0, n);

	st = access(path2, 0);
	ct_check_true((STRPTR)"io", (STRPTR)"access_after_remove",
		(STRPTR)"access after remove",
		(STRPTR)"access after remove must fail (non-zero)",
		st != 0);
	st = access(path3, 0);
	ct_check_true((STRPTR)"io", (STRPTR)"access_after_unlink",
		(STRPTR)"access after unlink",
		(STRPTR)"access after unlink must fail (non-zero)",
		st != 0);

	fd2 = open(path1, (ULONG)(CT_O_WRONLY | CT_O_CREAT | CT_O_TRUNC), 0);
	ct_check_true((STRPTR)"io", (STRPTR)"open_fail_cleanup",
		(STRPTR)"open after cleanup",
		(STRPTR)"open after cleanup must still work for a new file",
		fd2 >= 0);
	if (fd2 >= 0)
	{
		close(fd2);
		remove(path1);
	}
}
