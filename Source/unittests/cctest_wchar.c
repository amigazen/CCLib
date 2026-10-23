/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_wchar.c - multibyte / wchar / wctype LVOs (MB_CUR_MAX==1)
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include <limits.h>
#include <wchar.h>
#include <wctype.h>
#include "cctest_util.h"

void
ct_suite_wchar(void)
{
	wchar_t wc;
	wchar_t wbuf[8];
	char mbuf[8];
	LONG n;
	ULONG un;

	ct_suite_begin((STRPTR)"wchar",
		(STRPTR)"mblen/mbtowc/wctomb/mbstowcs/wcstombs + isw*/tow*");

	ct_check_long((STRPTR)"wchar", (STRPTR)"MB_CUR_MAX",
		(STRPTR)"MB_CUR_MAX",
		(STRPTR)"CClib single-byte foundation keeps MB_CUR_MAX at 1",
		1L, (LONG)MB_CUR_MAX);

	ct_check_long((STRPTR)"wchar", (STRPTR)"mblen_null",
		(STRPTR)"mblen(NULL, 0)",
		(STRPTR)"Null s reports non-state-dependent encoding (0)",
		0L, (LONG)mblen((CONST char *)NULL, 0UL));

	ct_check_long((STRPTR)"wchar", (STRPTR)"mblen_empty_n0",
		(STRPTR)"mblen(\"\", 0)",
		(STRPTR)"n==0 with non-NULL s must return -1",
		-1L, (LONG)mblen((CONST char *)"", 0UL));

	ct_check_long((STRPTR)"wchar", (STRPTR)"mblen_nul",
		(STRPTR)"mblen(\"\", 1)",
		(STRPTR)"NUL multibyte character length is 0",
		0L, (LONG)mblen((CONST char *)"", 1UL));

	ct_check_long((STRPTR)"wchar", (STRPTR)"mblen_a",
		(STRPTR)"mblen(\"a\", 1)",
		(STRPTR)"Single-byte letter length is 1",
		1L, (LONG)mblen((CONST char *)"a", 1UL));

	wc = (wchar_t)0;
	n = (LONG)mbtowc(&wc, (CONST char *)"Z", 1UL);
	ct_check_long((STRPTR)"wchar", (STRPTR)"mbtowc_len",
		(STRPTR)"mbtowc(&wc, \"Z\", 1)",
		(STRPTR)"mbtowc must consume one byte",
		1L, n);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbtowc_val",
		(STRPTR)"wc after mbtowc(\"Z\")",
		(STRPTR)"Identity conversion stores the byte value",
		(ULONG)'Z', (ULONG)wc);

	n = (LONG)mbtowc(&wc, (CONST char *)"", 1UL);
	ct_check_long((STRPTR)"wchar", (STRPTR)"mbtowc_nul_len",
		(STRPTR)"mbtowc(&wc, \"\\0\", 1)",
		(STRPTR)"NUL conversion returns 0",
		0L, n);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbtowc_nul_val",
		(STRPTR)"wc after mbtowc NUL",
		(STRPTR)"NUL wide character is 0",
		0UL, (ULONG)wc);

	mbuf[0] = 'x';
	n = (LONG)wctomb(mbuf, (wchar_t)'Q');
	ct_check_long((STRPTR)"wchar", (STRPTR)"wctomb_len",
		(STRPTR)"wctomb(buf, L'Q')",
		(STRPTR)"wctomb must write one byte",
		1L, n);
	ct_check_long((STRPTR)"wchar", (STRPTR)"wctomb_byte",
		(STRPTR)"buf[0] after wctomb",
		(STRPTR)"Identity conversion writes the low byte",
		(LONG)'Q', (LONG)(unsigned char)mbuf[0]);

	errno = 0;
	n = (LONG)wctomb(mbuf, (wchar_t)0x100);
	ct_check_long((STRPTR)"wchar", (STRPTR)"wctomb_overflow",
		(STRPTR)"wctomb(buf, 0x100)",
		(STRPTR)"Values outside 0..255 are encoding errors",
		-1L, n);
	ct_check_long((STRPTR)"wchar", (STRPTR)"wctomb_EILSEQ",
		(STRPTR)"errno after wctomb(0x100)",
		(STRPTR)"Failed wctomb must set EILSEQ",
		(LONG)EILSEQ, (LONG)errno);

	ct_check_long((STRPTR)"wchar", (STRPTR)"wctomb_null",
		(STRPTR)"wctomb(NULL, 0)",
		(STRPTR)"Null s reports non-state-dependent encoding (0)",
		0L, (LONG)wctomb((char *)NULL, (wchar_t)0));

	un = (ULONG)mbstowcs((wchar_t *)NULL, (CONST char *)"ab", 0UL);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbstowcs_count",
		(STRPTR)"mbstowcs(NULL, \"ab\", 0)",
		(STRPTR)"Null destination counts wide characters",
		2UL, un);

	wbuf[0] = wbuf[1] = wbuf[2] = (wchar_t)0xEE;
	un = (ULONG)mbstowcs(wbuf, (CONST char *)"ab", 8UL);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbstowcs_n",
		(STRPTR)"mbstowcs(wbuf, \"ab\", 8)",
		(STRPTR)"mbstowcs returns converted count excluding NUL",
		2UL, un);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbstowcs_0",
		(STRPTR)"wbuf[0]",
		(STRPTR)"First wide char is 'a'",
		(ULONG)'a', (ULONG)wbuf[0]);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbstowcs_1",
		(STRPTR)"wbuf[1]",
		(STRPTR)"Second wide char is 'b'",
		(ULONG)'b', (ULONG)wbuf[1]);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"mbstowcs_nul",
		(STRPTR)"wbuf[2]",
		(STRPTR)"Terminator wide NUL must be written",
		0UL, (ULONG)wbuf[2]);

	wbuf[0] = (wchar_t)'x';
	wbuf[1] = (wchar_t)'y';
	wbuf[2] = (wchar_t)0;
	mbuf[0] = mbuf[1] = mbuf[2] = 'Z';
	un = (ULONG)wcstombs(mbuf, wbuf, 8UL);
	ct_check_ulong((STRPTR)"wchar", (STRPTR)"wcstombs_n",
		(STRPTR)"wcstombs(mbuf, L\"xy\", 8)",
		(STRPTR)"wcstombs returns converted count excluding NUL",
		2UL, un);
	ct_check_string((STRPTR)"wchar", (STRPTR)"wcstombs_str",
		(STRPTR)"mbuf after wcstombs",
		(STRPTR)"Identity reverse conversion yields \"xy\"",
		(STRPTR)"xy", (STRPTR)mbuf);

	ct_check_true((STRPTR)"wchar", (STRPTR)"iswalpha_a",
		(STRPTR)"iswalpha(L'a')",
		(STRPTR)"iswalpha must be true for L'a' in \"C\"",
		iswalpha((wint_t)'a') != 0);
	ct_check_true((STRPTR)"wchar", (STRPTR)"iswdigit_5",
		(STRPTR)"iswdigit(L'5')",
		(STRPTR)"iswdigit must be true for L'5'",
		iswdigit((wint_t)'5') != 0);
	ct_check_true((STRPTR)"wchar", (STRPTR)"iswdigit_a_false",
		(STRPTR)"!iswdigit(L'a')",
		(STRPTR)"iswdigit must be false for letters",
		iswdigit((wint_t)'a') == 0);
	ct_check_true((STRPTR)"wchar", (STRPTR)"iswspace_space",
		(STRPTR)"iswspace(L' ')",
		(STRPTR)"iswspace must be true for space",
		iswspace((wint_t)' ') != 0);
	ct_check_true((STRPTR)"wchar", (STRPTR)"iswblank_tab",
		(STRPTR)"iswblank(L'\\t')",
		(STRPTR)"iswblank must be true for tab",
		iswblank((wint_t)'\t') != 0);
	ct_check_true((STRPTR)"wchar", (STRPTR)"iswascii_A",
		(STRPTR)"iswascii(L'A')",
		(STRPTR)"iswascii must be true for 7-bit L'A'",
		iswascii((wint_t)'A') != 0);
	ct_check_true((STRPTR)"wchar", (STRPTR)"iswalpha_WEOF",
		(STRPTR)"!iswalpha(WEOF)",
		(STRPTR)"isw* must treat WEOF as false",
		iswalpha(WEOF) == 0);

	ct_check_long((STRPTR)"wchar", (STRPTR)"towupper_a",
		(STRPTR)"towupper(L'a')",
		(STRPTR)"towupper(L'a') must return L'A'",
		(LONG)'A', (LONG)towupper((wint_t)'a'));
	ct_check_long((STRPTR)"wchar", (STRPTR)"towlower_A",
		(STRPTR)"towlower(L'A')",
		(STRPTR)"towlower(L'A') must return L'a'",
		(LONG)'a', (LONG)towlower((wint_t)'A'));
	ct_check_long((STRPTR)"wchar", (STRPTR)"towlower_WEOF",
		(STRPTR)"towlower(WEOF)",
		(STRPTR)"tow* must return WEOF unchanged",
		(LONG)WEOF, (LONG)towlower(WEOF));
}
