/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_stdio.c - stream I/O via tmpfile, fopen, fprintf/fscanf
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include "cctest_util.h"

void
ct_suite_stdio(void)
{
	APTR fp;
	APTR nfp;
	APTR ofp;
	char buf[128];
	char vbuf[64];
	char template[40];
	char tpath[48];
	LONG n;
	LONG c;
	LONG pos;
	LONG pos2;
	LONG iv;
	LONG wv;
	LONG rv;
	STRPTR t;
	LONG fd;

	ct_suite_begin((STRPTR)"stdio", (STRPTR)"tmpfile / fopen / fprintf / fscanf");

	fp = tmpfile();
	ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"tmpfile_create",
		(STRPTR)"tmpfile()",
		(STRPTR)"tmpfile() must open a temporary binary update stream",
		fp);
	if (fp == (APTR)NULL)
		return;

	n = fputs((STRPTR)"hello", fp);
	ct_check_true((STRPTR)"stdio", (STRPTR)"fputs_hello",
		(STRPTR)"fputs(\"hello\") to a tmpfile",
		(STRPTR)"fputs(\"hello\") to a tmpfile must succeed (non-negative)",
		n >= 0);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fputs_len",
		(STRPTR)"fputs(\"hello\")",
		(STRPTR)"fputs(\"hello\") must return the string length (5)",
		5, n);

	n = fflush(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fflush_ok",
		(STRPTR)"fflush of the tmpfile",
		(STRPTR)"fflush of the tmpfile must return 0 on success",
		0, n);

	n = fseek(fp, 0, 0);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fseek_set",
		(STRPTR)"fseek to beginning (SEEK_SET)",
		(STRPTR)"fseek to beginning (SEEK_SET) must return 0",
		0, n);

	pos = ftell(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"ftell_start",
		(STRPTR)"ftell after seek-to-start",
		(STRPTR)"ftell after seek-to-start must be 0",
		0, pos);

	buf[0] = '\0';
	t = fgets(buf, (LONG)sizeof(buf), fp);
	ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"fgets_ptr",
		(STRPTR)"fgets",
		(STRPTR)"fgets must return the buffer pointer on success",
		(APTR)t);
	ct_check_string((STRPTR)"stdio",
		(STRPTR)"fgets_content",
		(STRPTR)"fgets",
		(STRPTR)"fgets must read back the five characters written by fputs",
		(STRPTR)"hello",
		buf);

	n = fseek(fp, 0, 0);
	c = aputc((LONG)'Z', fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"aputc_Z",
		(STRPTR)"aputc('Z')",
		(STRPTR)"aputc('Z') must return the character written",
		(LONG)'Z', c);
	fflush(fp);
	fseek(fp, 0, 0);
	c = agetc(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"agetc_Z",
		(STRPTR)"agetc after writing Z at start",
		(STRPTR)"agetc after writing Z at start must read 'Z'",
		(LONG)'Z', c);

	fseek(fp, 0, 0);
	c = putc((LONG)'Y', fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"putc_Y",
		(STRPTR)"putc('Y')",
		(STRPTR)"putc('Y') must return the character written",
		(LONG)'Y', c);
	fflush(fp);
	fseek(fp, 0, 0);
	c = getc(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"getc_Y",
		(STRPTR)"getc after writing Y at start",
		(STRPTR)"getc after writing Y at start must read 'Y'",
		(LONG)'Y', c);

	fseek(fp, 0, 0);
	n = ungetc((LONG)'Q', fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"ungetc_Q",
		(STRPTR)"ungetc",
		(STRPTR)"ungetc must return the pushed-back character",
		(LONG)'Q', n);
	c = agetc(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"agetc_after_ungetc",
		(STRPTR)"Next agetc",
		(STRPTR)"Next agetc must return the character pushed by ungetc",
		(LONG)'Q', c);

	fseek(fp, 0, 0);
	n = (LONG)fwrite((APTR)"ABCD", 1, 4, fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fwrite_4",
		(STRPTR)"fwrite of 4 bytes",
		(STRPTR)"fwrite of 4 bytes must report 4 items written",
		4, n);
	fflush(fp);
	fseek(fp, 0, 0);
	memset(buf, 0, sizeof(buf));
	n = (LONG)fread(buf, 1, 4, fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fread_4",
		(STRPTR)"fread of 4 bytes",
		(STRPTR)"fread of 4 bytes must report 4 items read",
		4, n);
	buf[4] = '\0';
	ct_check_string((STRPTR)"stdio",
		(STRPTR)"fread_content",
		(STRPTR)"fread",
		(STRPTR)"fread must recover the four bytes written by fwrite",
		(STRPTR)"ABCD",
		buf);

	fseek(fp, 0, 2);
	pos = ftell(fp);
	ct_check_true((STRPTR)"stdio", (STRPTR)"ftell_end",
		(STRPTR)"ftell at SEEK_END",
		(STRPTR)"ftell at SEEK_END must be >= 4 after writing ABCD",
		pos >= 4);

	fseek(fp, 2, 0);
	pos = ftell(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fseek_cur_pos",
		(STRPTR)"fseek to offset 2 from SEEK_SET",
		(STRPTR)"fseek to offset 2 from SEEK_SET must leave ftell at 2",
		2, pos);
	fseek(fp, 1, 1);
	pos = ftell(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fseek_cur_rel",
		(STRPTR)"fseek +1 SEEK_CUR from 2",
		(STRPTR)"fseek +1 SEEK_CUR from 2 must leave ftell at 3",
		3, pos);

	n = fgetpos(fp, (APTR)&pos2);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fgetpos_ok",
		(STRPTR)"fgetpos",
		(STRPTR)"fgetpos must return 0 on success",
		0, n);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fgetpos_value",
		(STRPTR)"fgetpos",
		(STRPTR)"fgetpos must record the current stream position",
		pos, pos2);

	fseek(fp, 0, 0);
	n = fsetpos(fp, (APTR)&pos2);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fsetpos_ok",
		(STRPTR)"fsetpos",
		(STRPTR)"fsetpos must return 0 on success",
		0, n);
	pos = ftell(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fsetpos_restore",
		(STRPTR)"fsetpos",
		(STRPTR)"fsetpos must restore the position saved by fgetpos",
		pos2, pos);

	fseek(fp, 0, 0);
	n = putw((ULONG)0x1234, fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"putw_ok",
		(STRPTR)"putw(0x1234)",
		(STRPTR)"putw must return the word written (0x1234) on success",
		(LONG)0x1234, n);
	fflush(fp);
	fseek(fp, 0, 0);
	wv = getw(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"getw_value",
		(STRPTR)"getw",
		(STRPTR)"getw must read back the word written by putw (0x1234)",
		(LONG)0x1234, wv);

	n = fclose(fp);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fclose_tmp",
		(STRPTR)"fclose of the tmpfile",
		(STRPTR)"fclose of the tmpfile must return 0",
		0, n);

	strcpy(template, (STRPTR)"T:CCXXXXXX");
	t = mktemp(template);
	ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"mktemp_ptr",
		(STRPTR)"mktemp",
		(STRPTR)"mktemp must return a non-NULL unique name template result",
		(APTR)t);
	ct_check_true((STRPTR)"stdio", (STRPTR)"mktemp_changed",
		(STRPTR)"mktemp",
		(STRPTR)"mktemp must replace X characters in the template",
		strcmp(template, (STRPTR)"T:CCXXXXXX") != 0);

	strcpy(template, (STRPTR)"T:MSXXXXXX");
	fd = mkstemp(template);
	ct_check_true((STRPTR)"stdio", (STRPTR)"mkstemp_fd",
		(STRPTR)"mkstemp",
		(STRPTR)"mkstemp must return a valid file descriptor",
		fd != -1L);
	if (fd != -1L)
	{
		ct_check_true((STRPTR)"stdio", (STRPTR)"mkstemp_name",
			(STRPTR)"mkstemp template",
			(STRPTR)"mkstemp must rewrite X characters in the template",
			strcmp(template, (STRPTR)"T:MSXXXXXX") != 0);
		close(fd);
		remove(template);
	}

	buf[0] = '\0';
	t = tmpnam(buf);
	ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"tmpnam_buf",
		(STRPTR)"tmpnam with a caller buffer",
		(STRPTR)"tmpnam with a caller buffer must return that buffer",
		(APTR)t);
	ct_check_true((STRPTR)"stdio", (STRPTR)"tmpnam_nonempty",
		(STRPTR)"tmpnam",
		(STRPTR)"tmpnam must write a non-empty name into the buffer",
		buf[0] != '\0');

	strcpy(tpath, (STRPTR)"T:cctest_stdio");
	remove(tpath);
	ofp = fopen(tpath, (STRPTR)"w+");
	ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"fopen_create",
		(STRPTR)"fopen(T:cctest_stdio,\"w+\")",
		(STRPTR)"fopen(T:cctest_stdio,\"w+\") must return a non-NULL stream",
		ofp);
	if (ofp != (APTR)NULL)
	{
		n = setvbuf(ofp, (STRPTR)NULL, 3, 0);
		ct_check_long((STRPTR)"stdio", (STRPTR)"setvbuf_ionbf",
		(STRPTR)"setvbuf(_IONBF) on a T: file",
			(STRPTR)"setvbuf(_IONBF) on a T: file must return 0",
			0, n);

		n = fprintf(ofp, (STRPTR)"n=%ld k=%ld\n", 42L, 7L);
		ct_check_true((STRPTR)"stdio", (STRPTR)"fprintf_ok",
		(STRPTR)"fprintf to T: file",
			(STRPTR)"fprintf to T: file must return a positive character count",
			n > 0);
		ct_check_true((STRPTR)"stdio", (STRPTR)"fprintf_multi_len",
		(STRPTR)"fprintf with two %%ld fields",
			(STRPTR)"fprintf(\"n=%%ld k=%%ld\\n\",42,7) must write 9 characters",
			n >= 9);
		fflush(ofp);
		fseek(ofp, 0, 0);

		iv = 0;
		rv = 0;
		n = fscanf(ofp, (STRPTR)"n=%ld k=%ld", &iv, &rv);
		ct_check_long((STRPTR)"stdio", (STRPTR)"fscanf_count",
		(STRPTR)"fscanf",
			(STRPTR)"fscanf must assign two fields from \"n=42 k=7\"",
			2, n);
		ct_check_long((STRPTR)"stdio", (STRPTR)"fscanf_value",
		(STRPTR)"fscanf",
			(STRPTR)"fscanf must store 42 into the first destination long",
			42, iv);
		ct_check_long((STRPTR)"stdio", (STRPTR)"fscanf_value2",
		(STRPTR)"fscanf",
			(STRPTR)"fscanf must store 7 into the second destination long",
			7, rv);

		fseek(ofp, 0, 0);
		fputs((STRPTR)"reopen", ofp);
		fflush(ofp);

		nfp = freopen(tpath, (STRPTR)"r+", ofp);
		ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"freopen_ptr",
		(STRPTR)"freopen on the same T: path",
			(STRPTR)"freopen on the same T: path must return a stream",
			nfp);
		ct_check_ptr_eq((STRPTR)"stdio", (STRPTR)"freopen_same",
		(STRPTR)"freopen",
			(STRPTR)"freopen must reuse the original stream pointer",
			ofp, nfp);

		fseek(nfp, 0, 0);
		memset(vbuf, 0, sizeof(vbuf));
		t = fgets(vbuf, (LONG)sizeof(vbuf), nfp);
		ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"freopen_fgets",
		(STRPTR)"fgets after freopen",
			(STRPTR)"fgets after freopen must return the buffer pointer",
			(APTR)t);
		ct_check_true((STRPTR)"stdio", (STRPTR)"freopen_content",
		(STRPTR)"Content after freopen",
			(STRPTR)"Content after freopen must begin with \"reopen\"",
			strncmp(vbuf, (STRPTR)"reopen", 6) == 0);

		n = fclose(nfp);
		ct_check_long((STRPTR)"stdio", (STRPTR)"fclose_named",
		(STRPTR)"fclose of the named T: stream",
			(STRPTR)"fclose of the named T: stream must return 0",
			0, n);
	}

	remove(tpath);

	fp = tmpfile();
	if (fp != (APTR)NULL)
	{
		fputs((STRPTR)"line1\nline2", fp);
		fflush(fp);
		fseek(fp, 0, 0);
		memset(buf, 0, sizeof(buf));
		t = fgets(buf, (LONG)sizeof(buf), fp);
		ct_check_string((STRPTR)"stdio",
		(STRPTR)"fgets_line1",
		(STRPTR)"First fgets",
		(STRPTR)"First fgets must read \"line1\" including newline",
		(STRPTR)"line1\n",
		buf);
		t = fgets(buf, (LONG)sizeof(buf), fp);
		ct_check_string((STRPTR)"stdio",
		(STRPTR)"fgets_line2",
		(STRPTR)"Second fgets",
		(STRPTR)"Second fgets must read \"line2\"",
		(STRPTR)"line2",
		buf);
		fseek(fp, 0, 2);
		pos = ftell(fp);
		ct_check_true((STRPTR)"stdio", (STRPTR)"ftell_end_lines",
		(STRPTR)"ftell at SEEK_END after two lines",
			(STRPTR)"ftell at SEEK_END after two lines must be positive",
			pos > 0);
		fclose(fp);
	}

	fp = tmpfile();
	if (fp != (APTR)NULL)
	{
		n = fputs((STRPTR)"data", fp);
		ct_check_long((STRPTR)"stdio", (STRPTR)"fputs_data_len",
		(STRPTR)"fputs(\"data\")",
			(STRPTR)"fputs(\"data\") must return length 4",
			4, n);
		fflush(fp);
		fseek(fp, 0, 0);
		memset(buf, 0, sizeof(buf));
		n = (LONG)fread(buf, 1, 2, fp);
		ct_check_long((STRPTR)"stdio", (STRPTR)"fread_partial",
		(STRPTR)"fread of 2 bytes from \"data\"",
			(STRPTR)"fread of 2 bytes from \"data\" must return 2",
			2, n);
		buf[2] = '\0';
		ct_check_string((STRPTR)"stdio",
		(STRPTR)"fread_partial_content",
		(STRPTR)"Partial fread",
		(STRPTR)"Partial fread must yield \"da\"",
		(STRPTR)"da",
		buf);
		fclose(fp);
	}

	fp = tmpfile();
	if (fp != (APTR)NULL)
	{
		fwrite((APTR)"XYZ", 1, 3, fp);
		fflush(fp);
		fseek(fp, 1, 0);
		c = agetc(fp);
		ct_check_long((STRPTR)"stdio", (STRPTR)"agetc_mid",
		(STRPTR)"agetc at offset 1",
			(STRPTR)"agetc at offset 1 must return 'Y'",
			(LONG)'Y', c);
		ungetc((LONG)'!', fp);
		c = getc(fp);
		ct_check_long((STRPTR)"stdio", (STRPTR)"ungetc_mid",
		(STRPTR)"ungetc in the middle of a stream",
			(STRPTR)"ungetc in the middle of a stream must supply '!'",
			(LONG)'!', c);
		fclose(fp);
	}

	{
		char *line;
		ULONG linesz;
		LONG glen;
		APTR gfp;

		line = (char *)NULL;
		linesz = 0UL;
		gfp = tmpfile();
		ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"getline_tmp",
			(STRPTR)"tmpfile for getline",
			(STRPTR)"tmpfile must open for getline test",
			gfp);
		if (gfp != (APTR)NULL)
		{
			fputs((STRPTR)"hello line\nrest", gfp);
			fflush(gfp);
			fseek(gfp, 0, 0);
			glen = getline((APTR)&line, (APTR)&linesz, gfp);
			ct_check_true((STRPTR)"stdio", (STRPTR)"getline_len",
				(STRPTR)"getline",
				(STRPTR)"getline must return length including newline",
				glen == 11);
			ct_check_string((STRPTR)"stdio",
				(STRPTR)"getline_text",
				(STRPTR)"getline buffer",
				(STRPTR)"getline must store the first line with newline",
				(STRPTR)"hello line\n",
				line);
			fclose(gfp);
			if (line != (char *)NULL)
				free(line);
		}
	}

	{
		FILE *sfp;
		char *big;
		char *got;
		char apath[40];
		LONG i;
		LONG fd;
		APTR afp;
		APTR rfp;

		sfp = (FILE *)tmpfile();
		ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"lazy_buf_tmp",
			(STRPTR)"tmpfile for lazy buffer size",
			(STRPTR)"tmpfile must open so getbuff can allocate the default buffer",
			(APTR)sfp);
		if (sfp != (FILE *)NULL)
		{
			c = putc((LONG)'A', (APTR)sfp);
			ct_check_long((STRPTR)"stdio", (STRPTR)"lazy_buf_putc",
				(STRPTR)"putc to force getbuff",
				(STRPTR)"first putc must succeed so the lazy buffer exists",
				(LONG)'A', c);
			ct_check_long((STRPTR)"stdio", (STRPTR)"lazy_buf_size",
				(STRPTR)"FILE->_buflen after first putc",
				(STRPTR)"getbuff must allocate the 8192-byte lazy buffer, not public BUFSIZ",
				8192L, (LONG)sfp->_buflen);
			fclose((APTR)sfp);
		}

		fp = tmpfile();
		ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"fflush_null_tmp",
			(STRPTR)"tmpfile for fflush(NULL)",
			(STRPTR)"tmpfile must open so a dirty buffer can be flushed by fflush(NULL)",
			fp);
		if (fp != (APTR)NULL)
		{
			n = (LONG)fwrite((APTR)"FLUSHNULL", 1UL, 9UL, fp);
			ct_check_long((STRPTR)"stdio", (STRPTR)"fflush_null_fwrite",
				(STRPTR)"fwrite(\"FLUSHNULL\")",
				(STRPTR)"nine bytes must sit in the FILE buffer (smaller than 8192)",
				9, n);
			n = fflush((APTR)NULL);
			ct_check_long((STRPTR)"stdio", (STRPTR)"fflush_null_ok",
				(STRPTR)"fflush(NULL) after a dirty fwrite",
				(STRPTR)"fflush(NULL) must flush dirty session streams and return 0",
				0, n);
			fd = ((FILE *)fp)->_unit;
			pos = lseek(fd, 0L, 0);
			ct_check_long((STRPTR)"stdio", (STRPTR)"fflush_null_seek",
				(STRPTR)"lseek(fileno, 0) after fflush(NULL)",
				(STRPTR)"underlying fd must be readable at 0 after ISO fflush(NULL)",
				0, pos);
			memset(buf, 0, sizeof(buf));
			n = read(fd, (APTR)buf, 9L);
			ct_check_long((STRPTR)"stdio", (STRPTR)"fflush_null_read",
				(STRPTR)"read of 9 from fileno after fflush(NULL)",
				(STRPTR)"fflush(NULL) must have pushed the dirty buffer to AmigaDOS",
				9, n);
			buf[9] = '\0';
			ct_check_string((STRPTR)"stdio",
				(STRPTR)"fflush_null_content",
				(STRPTR)"contents after fflush(NULL)",
				(STRPTR)"disk bytes must be FLUSHNULL without an explicit fflush(fp)",
				(STRPTR)"FLUSHNULL",
				buf);
			fclose(fp);
		}

		big = (char *)malloc(10000UL);
		got = (char *)malloc(10000UL);
		ct_check_true((STRPTR)"stdio", (STRPTR)"bulk_alloc",
			(STRPTR)"malloc(10000) x2 for fread/fwrite bypass",
			(STRPTR)"heap buffers must be available for the 9000-byte bulk I/O test",
			big != (char *)NULL && got != (char *)NULL);
		if (big != (char *)NULL && got != (char *)NULL)
		{
			for (i = 0L; i < 9000L; i++)
				big[i] = (char)('A' + (i % 26L));
			fp = tmpfile();
			ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"bulk_tmp",
				(STRPTR)"tmpfile for 9000-byte fwrite/fread",
				(STRPTR)"tmpfile must open for the bulk transfer test",
				fp);
			if (fp != (APTR)NULL)
			{
				n = (LONG)fwrite((APTR)big, 1UL, 9000UL, fp);
				ct_check_long((STRPTR)"stdio", (STRPTR)"fwrite_9000",
					(STRPTR)"fwrite of 9000 bytes",
					(STRPTR)"fwrite larger than the FILE buffer must transfer 9000 items",
					9000L, n);
				fflush(fp);
				fseek(fp, 0, 0);
				memset(got, 0, 9000);
				n = (LONG)fread((APTR)got, 1UL, 9000UL, fp);
				ct_check_long((STRPTR)"stdio", (STRPTR)"fread_9000",
					(STRPTR)"fread of 9000 bytes",
					(STRPTR)"fread larger than the FILE buffer must transfer 9000 items",
					9000L, n);
				ct_check_true((STRPTR)"stdio", (STRPTR)"fread_9000_match",
					(STRPTR)"memcmp after bulk fread",
					(STRPTR)"bulk fread must recover the 9000 bytes written by fwrite",
					memcmp((APTR)big, (APTR)got, 9000) == 0);
				fclose(fp);
			}
		}
		if (big != (char *)NULL)
			free(big);
		if (got != (char *)NULL)
			free(got);

		strcpy(apath, (STRPTR)"T:cctest_fopen_a");
		remove(apath);
		afp = fopen(apath, (STRPTR)"w");
		ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"fopen_w_append",
			(STRPTR)"fopen(T:cctest_fopen_a,\"w\")",
			(STRPTR)"setup file for fopen-append must be created",
			afp);
		if (afp != (APTR)NULL)
		{
			fputs((STRPTR)"AB", afp);
			fclose(afp);
			afp = fopen(apath, (STRPTR)"a");
			ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"fopen_a",
				(STRPTR)"fopen(...,\"a\")",
				(STRPTR)"fopen append mode must reopen the existing file",
				afp);
			if (afp != (APTR)NULL)
			{
				n = fputs((STRPTR)"CD", afp);
				ct_check_long((STRPTR)"stdio", (STRPTR)"fputs_append",
					(STRPTR)"fputs(\"CD\") in append mode",
					(STRPTR)"append fputs must report 2 characters",
					2, n);
				fclose(afp);
			}
			rfp = fopen(apath, (STRPTR)"r");
			ct_check_ptr_nonnull((STRPTR)"stdio", (STRPTR)"fopen_a_read",
				(STRPTR)"fopen of the append result for read",
				(STRPTR)"the append result file must open for reading",
				rfp);
			if (rfp != (APTR)NULL)
			{
				memset(buf, 0, sizeof(buf));
				t = fgets(buf, (LONG)sizeof(buf), rfp);
				ct_check_ptr_nonnull((STRPTR)"stdio",
					(STRPTR)"fgets_append",
					(STRPTR)"fgets after fopen \"a\"",
					(STRPTR)"fgets must return the buffer after append writes",
					(APTR)t);
				ct_check_string((STRPTR)"stdio",
					(STRPTR)"fgets_append_content",
					(STRPTR)"append file contents",
					(STRPTR)"fopen \"w\" then \"a\" must yield ABCD",
					(STRPTR)"ABCD",
					buf);
				fclose(rfp);
			}
		}
		remove(apath);
	}

	errno = 0;
	n = (LONG)fread((APTR)NULL, 1UL, 4UL, stdout);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fread_null_ptr",
		(STRPTR)"fread(NULL, 1, 4, stdout)",
		(STRPTR)"NULL buffer is EINVAL and transfers 0, no panic",
		0, n);
	ct_check_long((STRPTR)"stdio", (STRPTR)"fread_null_ptr_errno",
		(STRPTR)"errno after fread(NULL, ...)",
		(STRPTR)"NULL fread dest must set EINVAL",
		(LONG)EINVAL, errno);
}
