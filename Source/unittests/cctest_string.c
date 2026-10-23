/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_string.c - string LVOs
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include "cctest_util.h"

void
ct_suite_string(void)
{
	char a[80];
	char b[80];
	char c[80];
	STRPTR p;
	STRPTR q;
	STRPTR r;
	LONG n;
	LONG i;

	ct_suite_begin((STRPTR)"string", (STRPTR)"strlen strcpy strcat strcmp family");

	/* --- strlen --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strlen_empty",
		(STRPTR)"strlen(\"\")",
		(STRPTR)"strlen of an empty string must return 0",
		0, strlen((STRPTR)""));

	ct_check_long((STRPTR)"string", (STRPTR)"strlen_one",
		(STRPTR)"strlen(\"a\")",
		(STRPTR)"strlen must count a single character before the terminator",
		1, strlen((STRPTR)"a"));

	ct_check_long((STRPTR)"string", (STRPTR)"strlen_four",
		(STRPTR)"strlen(\"abcd\")",
		(STRPTR)"strlen must return 4 for a four-character literal",
		4, strlen((STRPTR)"abcd"));

	ct_check_long((STRPTR)"string", (STRPTR)"strlen_ten",
		(STRPTR)"strlen(\"0123456789\")",
		(STRPTR)"strlen must return 10 for a ten-digit run",
		10, strlen((STRPTR)"0123456789"));

	ct_check_long((STRPTR)"string", (STRPTR)"strlen_twenty",
		(STRPTR)"strlen(\"01234567890123456789\")",
		(STRPTR)"strlen must count twenty characters without including the NUL",
		20, strlen((STRPTR)"01234567890123456789"));

	ct_check_long((STRPTR)"string", (STRPTR)"strlen_thirtytwo",
		(STRPTR)"strlen(\"01234567890123456789012345678901\")",
		(STRPTR)"strlen must handle a 32-character string",
		32, strlen((STRPTR)"01234567890123456789012345678901"));

	errno = 0;
	n = strlen((STRPTR)NULL);
	ct_check_long((STRPTR)"string", (STRPTR)"strlen_null",
		(STRPTR)"strlen(NULL)",
		(STRPTR)"NULL is a sentinel: return 0",
		0, n);
	ct_check_long((STRPTR)"string", (STRPTR)"strlen_null_errno",
		(STRPTR)"errno after strlen(NULL)",
		(STRPTR)"NULL strlen must set EINVAL and not panic",
		(LONG)EINVAL, errno);

	/* --- strcpy --- */
	strcpy(a, (STRPTR)"hello");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcpy_basic",
		(STRPTR)"strcpy(a, \"hello\")",
		(STRPTR)"strcpy must copy the full NUL-terminated source into the destination",
		(STRPTR)"hello",
		a);

	strcpy(a, (STRPTR)"");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcpy_empty",
		(STRPTR)"strcpy(a, \"\")",
		(STRPTR)"strcpy of an empty source must leave destination as an empty string",
		(STRPTR)"",
		a);

	strcpy(a, (STRPTR)"X");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcpy_one_char",
		(STRPTR)"strcpy(a, \"X\")",
		(STRPTR)"strcpy must copy a single character plus the terminator",
		(STRPTR)"X",
		a);

	strcpy(a, (STRPTR)"old");
	strcpy(a, (STRPTR)"new");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcpy_overwrite",
		(STRPTR)"strcpy(a, \"new\") after prior contents",
		(STRPTR)"strcpy must overwrite the entire prior destination contents",
		(STRPTR)"new",
		a);

	/* --- strcat --- */
	strcpy(a, (STRPTR)"");
	strcat(a, (STRPTR)"go");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcat_onto_empty",
		(STRPTR)"strcat(a, \"go\") on empty destination",
		(STRPTR)"strcat onto an empty string must yield the appended text",
		(STRPTR)"go",
		a);

	strcpy(a, (STRPTR)"hi");
	strcat(a, (STRPTR)"");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcat_empty_src",
		(STRPTR)"strcat(a, \"\")",
		(STRPTR)"strcat with an empty source must not alter the destination",
		(STRPTR)"hi",
		a);

	strcpy(a, (STRPTR)"foo");
	strcat(a, (STRPTR)"bar");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcat_append",
		(STRPTR)"strcat(a, \"bar\") after strcpy(a,\"foo\")",
		(STRPTR)"strcat must append src onto dst including a new NUL terminator",
		(STRPTR)"foobar",
		a);

	strcpy(a, (STRPTR)"ab");
	strcat(a, (STRPTR)"cd");
	strcat(a, (STRPTR)"ef");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strcat_twice",
		(STRPTR)"two strcat calls chaining cd and ef onto ab",
		(STRPTR)"repeated strcat must build the concatenation in order",
		(STRPTR)"abcdef",
		a);

	/* --- strcmp --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strcmp_equal",
		(STRPTR)"strcmp(\"same\", \"same\")",
		(STRPTR)"strcmp of identical strings must return 0",
		0, strcmp((STRPTR)"same", (STRPTR)"same"));

	ct_check_true((STRPTR)"string", (STRPTR)"strcmp_less",
		(STRPTR)"strcmp(\"a\", \"b\") < 0",
		(STRPTR)"strcmp must return negative when the first string sorts before the second",
		strcmp((STRPTR)"a", (STRPTR)"b") < 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strcmp_greater",
		(STRPTR)"strcmp(\"b\", \"a\") > 0",
		(STRPTR)"strcmp must return positive when the first string sorts after the second",
		strcmp((STRPTR)"b", (STRPTR)"a") > 0);

	ct_check_long((STRPTR)"string", (STRPTR)"strcmp_empty_equal",
		(STRPTR)"strcmp(\"\", \"\")",
		(STRPTR)"two empty strings must compare equal",
		0, strcmp((STRPTR)"", (STRPTR)""));

	ct_check_true((STRPTR)"string", (STRPTR)"strcmp_empty_lt_nonempty",
		(STRPTR)"strcmp(\"\", \"a\") < 0",
		(STRPTR)"empty string must sort before any non-empty string",
		strcmp((STRPTR)"", (STRPTR)"a") < 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strcmp_prefix_lt",
		(STRPTR)"strcmp(\"abc\", \"abcd\") < 0",
		(STRPTR)"a proper prefix must compare less than the longer string",
		strcmp((STRPTR)"abc", (STRPTR)"abcd") < 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strcmp_prefix_gt",
		(STRPTR)"strcmp(\"abcd\", \"abc\") > 0",
		(STRPTR)"a longer string must compare greater than its proper prefix",
		strcmp((STRPTR)"abcd", (STRPTR)"abc") > 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strcmp_case_sensitive",
		(STRPTR)"strcmp(\"A\", \"a\") != 0",
		(STRPTR)"strcmp is case-sensitive and must not treat A and a as equal",
		strcmp((STRPTR)"A", (STRPTR)"a") != 0);

	/* --- strncmp --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strncmp_prefix_equal",
		(STRPTR)"strncmp(\"abcdef\", \"abcxyz\", 3)",
		(STRPTR)"strncmp with n=3 must compare only the shared prefix and return 0",
		0, strncmp((STRPTR)"abcdef", (STRPTR)"abcxyz", 3));

	ct_check_true((STRPTR)"string", (STRPTR)"strncmp_full_diff",
		(STRPTR)"strncmp(\"abcd\", \"abxd\", 4) != 0",
		(STRPTR)"strncmp must detect a difference within the compared length",
		strncmp((STRPTR)"abcd", (STRPTR)"abxd", 4) != 0);

	ct_check_long((STRPTR)"string", (STRPTR)"strncmp_n_zero",
		(STRPTR)"strncmp(\"abc\", \"xyz\", 0)",
		(STRPTR)"strncmp with n=0 must compare no characters and return 0",
		0, strncmp((STRPTR)"abc", (STRPTR)"xyz", 0));

	ct_check_long((STRPTR)"string", (STRPTR)"strncmp_n_one_equal",
		(STRPTR)"strncmp(\"abc\", \"axc\", 1)",
		(STRPTR)"strncmp with n=1 must compare only the first byte",
		0, strncmp((STRPTR)"abc", (STRPTR)"axc", 1));

	ct_check_true((STRPTR)"string", (STRPTR)"strncmp_n_one_diff",
		(STRPTR)"strncmp(\"b\", \"a\", 1) > 0",
		(STRPTR)"strncmp first-byte difference must follow strcmp ordering",
		strncmp((STRPTR)"b", (STRPTR)"a", 1) > 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strncmp_longer_n_equal",
		(STRPTR)"strncmp(\"hello\", \"hello!\", 5) == 0",
		(STRPTR)"strncmp must stop at n even when strings differ later",
		strncmp((STRPTR)"hello", (STRPTR)"hello!", 5) == 0);

	/* --- strncpy --- */
	memset(a, 0, sizeof(a));
	strncpy(a, (STRPTR)"ABCDEFGH", 4);
	a[4] = '\0';
	ct_check_string((STRPTR)"string",
		(STRPTR)"strncpy_limited",
		(STRPTR)"strncpy(a, \"ABCDEFGH\", 4) then force NUL at index 4",
		(STRPTR)"strncpy must copy exactly n bytes without adding a NUL when n < src len",
		(STRPTR)"ABCD",
		a);

	memset(a, 0, sizeof(a));
	strncpy(a, (STRPTR)"hi", 8);
	ct_check_true((STRPTR)"string", (STRPTR)"strncpy_zero_fill",
		(STRPTR)"strncpy(a, \"hi\", 8) zero-pads remainder",
		(STRPTR)"when n exceeds source length strncpy must zero-fill the remaining bytes",
		a[0] == 'h' && a[1] == 'i' && a[2] == '\0'
		&& a[3] == '\0' && a[4] == '\0' && a[5] == '\0'
		&& a[6] == '\0' && a[7] == '\0');

	memset(a, 'Z', sizeof(a));
	a[sizeof(a) - 1] = '\0';
	strncpy(a, (STRPTR)"ok", 0);
	ct_check_true((STRPTR)"string", (STRPTR)"strncpy_n_zero",
		(STRPTR)"strncpy(a, \"ok\", 0)",
		(STRPTR)"CClib strncpy with n=0 copies no source bytes but still writes a NUL at dst[0]",
		a[0] == '\0');

	strcpy(a, (STRPTR)"base");
	strncpy(a, (STRPTR)"XY", 2);
	a[2] = '\0';
	ct_check_string((STRPTR)"string",
		(STRPTR)"strncpy_exact_src_len",
		(STRPTR)"strncpy(a, \"XY\", 2)",
		(STRPTR)"strncpy with n equal to source length must copy those bytes",
		(STRPTR)"XY",
		a);

	/* --- strncat --- */
	strcpy(a, (STRPTR)"hi");
	strncat(a, (STRPTR)"THERE", 3);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strncat_limited",
		(STRPTR)"strncat(a, \"THERE\", 3) after strcpy(a,\"hi\")",
		(STRPTR)"strncat must append at most n characters from the source",
		(STRPTR)"hiTHE",
		a);

	strcpy(a, (STRPTR)"ab");
	strncat(a, (STRPTR)"CDEF", 0);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strncat_n_zero",
		(STRPTR)"strncat(a, \"CDEF\", 0)",
		(STRPTR)"strncat with n=0 must leave the destination unchanged",
		(STRPTR)"ab",
		a);

	strcpy(a, (STRPTR)"go");
	strncat(a, (STRPTR)"odbye", 5);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strncat_full",
		(STRPTR)"strncat(a, \"odbye\", 5)",
		(STRPTR)"strncat with n covering the full source must append the entire suffix",
		(STRPTR)"goodbye",
		a);

	strcpy(a, (STRPTR)"x");
	strncat(a, (STRPTR)"yz", 1);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strncat_one_char",
		(STRPTR)"strncat(a, \"yz\", 1)",
		(STRPTR)"strncat with n=1 must append exactly one character",
		(STRPTR)"xy",
		a);

	/* --- strchr / strrchr --- */
	p = strchr((STRPTR)"banana", (LONG)'n');
	ct_check_true((STRPTR)"string", (STRPTR)"strchr_found_first",
		(STRPTR)"strchr(\"banana\", 'n')",
		(STRPTR)"strchr must return a pointer to the first matching character",
		p != (STRPTR)NULL && p[0] == 'n' && p[1] == 'a');

	p = strchr((STRPTR)"banana", (LONG)'b');
	ct_check_true((STRPTR)"string", (STRPTR)"strchr_found_start",
		(STRPTR)"strchr(\"banana\", 'b')",
		(STRPTR)"strchr must find a character at the start of the string",
		p != (STRPTR)NULL && p == (STRPTR)"banana");

	p = strchr((STRPTR)"banana", (LONG)'z');
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strchr_missing",
		(STRPTR)"strchr(\"banana\", 'z')",
		(STRPTR)"strchr must return NULL when the character is absent",
		(APTR)p);

	p = strchr((STRPTR)"abc", (LONG)'c');
	ct_check_true((STRPTR)"string", (STRPTR)"strchr_found_last",
		(STRPTR)"strchr(\"abc\", 'c')",
		(STRPTR)"strchr must find the last character when it is the only occurrence",
		p != (STRPTR)NULL && p[0] == 'c' && p[1] == '\0');

	p = strrchr((STRPTR)"banana", (LONG)'a');
	ct_check_true((STRPTR)"string", (STRPTR)"strrchr_last",
		(STRPTR)"strrchr(\"banana\", 'a')",
		(STRPTR)"strrchr must point at the final occurrence of the character",
		p != (STRPTR)NULL && strcmp(p, (STRPTR)"a") == 0);

	p = strrchr((STRPTR)"banana", (LONG)'b');
	ct_check_true((STRPTR)"string", (STRPTR)"strrchr_first_only",
		(STRPTR)"strrchr(\"banana\", 'b')",
		(STRPTR)"strrchr with a single occurrence must return the same position as strchr",
		p != (STRPTR)NULL && p[0] == 'b');

	p = strrchr((STRPTR)"hello", (LONG)'z');
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strrchr_missing",
		(STRPTR)"strrchr(\"hello\", 'z')",
		(STRPTR)"strrchr must return NULL when the character is absent",
		(APTR)p);

	/* --- index / rindex (BSD names for strchr / strrchr) --- */
	p = index((STRPTR)"hello", (LONG)'e');
	ct_check_true((STRPTR)"string", (STRPTR)"index_alias",
		(STRPTR)"index(\"hello\", 'e')",
		(STRPTR)"index is the BSD alias of strchr and must find the first 'e'",
		p != (STRPTR)NULL && p[0] == 'e');

	p = index((STRPTR)"miss", (LONG)'s');
	ct_check_true((STRPTR)"string", (STRPTR)"index_second_char",
		(STRPTR)"index(\"miss\", 's')",
		(STRPTR)"index must locate a character not at the start of the string",
		p != (STRPTR)NULL && p[0] == 's' && p[1] == 's');

	p = rindex((STRPTR)"hello", (LONG)'l');
	ct_check_true((STRPTR)"string", (STRPTR)"rindex_alias",
		(STRPTR)"rindex(\"hello\", 'l')",
		(STRPTR)"rindex is the BSD alias of strrchr and must find the last 'l'",
		p != (STRPTR)NULL && p[0] == 'l' && p[1] == 'o');

	p = rindex((STRPTR)"abc", (LONG)'a');
	ct_check_true((STRPTR)"string", (STRPTR)"rindex_first_char",
		(STRPTR)"rindex(\"abc\", 'a')",
		(STRPTR)"rindex must return the start pointer when only one match exists at index 0",
		p != (STRPTR)NULL && p == (STRPTR)"abc");

	/* --- strstr --- */
	p = strstr((STRPTR)"the quick brown", (STRPTR)"quick");
	ct_check_true((STRPTR)"string", (STRPTR)"strstr_found_middle",
		(STRPTR)"strstr(\"the quick brown\", \"quick\")",
		(STRPTR)"strstr must locate a substring in the middle of the haystack",
		p != (STRPTR)NULL && strncmp(p, (STRPTR)"quick", 5) == 0);

	p = strstr((STRPTR)"foobar", (STRPTR)"foo");
	ct_check_true((STRPTR)"string", (STRPTR)"strstr_found_start",
		(STRPTR)"strstr(\"foobar\", \"foo\")",
		(STRPTR)"strstr must return the haystack pointer when the needle is a prefix",
		p != (STRPTR)NULL && p == (STRPTR)"foobar");

	p = strstr((STRPTR)"foobar", (STRPTR)"bar");
	ct_check_true((STRPTR)"string", (STRPTR)"strstr_found_end",
		(STRPTR)"strstr(\"foobar\", \"bar\")",
		(STRPTR)"strstr must locate a suffix substring",
		p != (STRPTR)NULL && strcmp(p, (STRPTR)"bar") == 0);

	p = strstr((STRPTR)"the quick brown", (STRPTR)"slow");
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strstr_missing",
		(STRPTR)"strstr(\"the quick brown\", \"slow\")",
		(STRPTR)"strstr must return NULL when the needle is absent",
		(APTR)p);

	p = strstr((STRPTR)"anything", (STRPTR)"");
	ct_check_ptr_eq((STRPTR)"string", (STRPTR)"strstr_empty_needle",
		(STRPTR)"strstr(\"anything\", \"\")",
		(STRPTR)"strstr with an empty needle must return the haystack pointer",
		(APTR)(STRPTR)"anything", (APTR)p);

	/* --- strspn / strcspn / strpbrk --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strspn_digits",
		(STRPTR)"strspn(\"123abc\", \"0123456789\")",
		(STRPTR)"strspn must return the length of the initial span in the accept set",
		3, strspn((STRPTR)"123abc", (STRPTR)"0123456789"));

	ct_check_long((STRPTR)"string", (STRPTR)"strspn_all_accept",
		(STRPTR)"strspn(\"abc\", \"abc\")",
		(STRPTR)"strspn must count the entire string when every character is accepted",
		3, strspn((STRPTR)"abc", (STRPTR)"abc"));

	ct_check_long((STRPTR)"string", (STRPTR)"strspn_none",
		(STRPTR)"strspn(\"abc\", \"xyz\")",
		(STRPTR)"strspn must return 0 when no initial character is in the accept set",
		0, strspn((STRPTR)"abc", (STRPTR)"xyz"));

	ct_check_long((STRPTR)"string", (STRPTR)"strspn_empty_accept",
		(STRPTR)"strspn(\"abc\", \"\")",
		(STRPTR)"strspn with an empty accept set must return 0",
		0, strspn((STRPTR)"abc", (STRPTR)""));

	ct_check_long((STRPTR)"string", (STRPTR)"strcspn_no_reject",
		(STRPTR)"strcspn(\"hello\", \"x\")",
		(STRPTR)"strcspn must return full length when no reject character appears",
		5, strcspn((STRPTR)"hello", (STRPTR)"x"));

	ct_check_long((STRPTR)"string", (STRPTR)"strcspn_at_start",
		(STRPTR)"strcspn(\"xhello\", \"x\")",
		(STRPTR)"strcspn must return 0 when the first character is in the reject set",
		0, strcspn((STRPTR)"xhello", (STRPTR)"x"));

	ct_check_long((STRPTR)"string", (STRPTR)"strcspn_middle",
		(STRPTR)"strcspn(\"helxo\", \"x\")",
		(STRPTR)"strcspn must return the index of the first reject character",
		3, strcspn((STRPTR)"helxo", (STRPTR)"x"));

	p = strpbrk((STRPTR)"hello", (STRPTR)"aeiou");
	ct_check_true((STRPTR)"string", (STRPTR)"strpbrk_vowel",
		(STRPTR)"strpbrk(\"hello\", \"aeiou\")",
		(STRPTR)"strpbrk must return a pointer to the first character from the accept set",
		p != (STRPTR)NULL && p[0] == 'e');

	p = strpbrk((STRPTR)"xyz", (STRPTR)"aeiou");
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strpbrk_none",
		(STRPTR)"strpbrk(\"xyz\", \"aeiou\")",
		(STRPTR)"strpbrk must return NULL when no character matches",
		(APTR)p);

	p = strpbrk((STRPTR)"abc", (STRPTR)"cba");
	ct_check_true((STRPTR)"string", (STRPTR)"strpbrk_first_match",
		(STRPTR)"strpbrk(\"abc\", \"cba\")",
		(STRPTR)"strpbrk must find the earliest haystack character present in the set",
		p != (STRPTR)NULL && p == (STRPTR)"abc");

	/* --- strupr --- */
	strcpy(a, (STRPTR)"AbC");
	strupr(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strupr_mixed",
		(STRPTR)"strupr(a) after strcpy(a,\"AbC\")",
		(STRPTR)"strupr must convert every lowercase letter to uppercase in place",
		(STRPTR)"ABC",
		a);

	strcpy(a, (STRPTR)"already");
	strupr(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strupr_already_upper",
		(STRPTR)"strupr(\"already\")",
		(STRPTR)"strupr must leave already-uppercase letters unchanged",
		(STRPTR)"ALREADY",
		a);

	strcpy(a, (STRPTR)"123!@#");
	strupr(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strupr_nonletters",
		(STRPTR)"strupr(\"123!@#\")",
		(STRPTR)"strupr must not alter digits or punctuation",
		(STRPTR)"123!@#",
		a);

	/* --- stpcrlf --- */
	strcpy(a, (STRPTR)"line\r\n");
	stpcrlf(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"stpcrlf_crlf",
		(STRPTR)"stpcrlf(\"line\\r\\n\")",
		(STRPTR)"stpcrlf must strip trailing CR/LF pairs",
		(STRPTR)"line",
		a);

	strcpy(a, (STRPTR)"text\r");
	stpcrlf(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"stpcrlf_cr_only",
		(STRPTR)"stpcrlf(\"text\\r\")",
		(STRPTR)"stpcrlf must strip a lone trailing carriage return",
		(STRPTR)"text",
		a);

	strcpy(a, (STRPTR)"text\n");
	stpcrlf(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"stpcrlf_lf_only",
		(STRPTR)"stpcrlf(\"text\\n\")",
		(STRPTR)"stpcrlf must strip a lone trailing line feed",
		(STRPTR)"text",
		a);

	strcpy(a, (STRPTR)"clean");
	stpcrlf(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"stpcrlf_no_ending",
		(STRPTR)"stpcrlf(\"clean\")",
		(STRPTR)"stpcrlf must not alter a string with no trailing line ending",
		(STRPTR)"clean",
		a);

	/* --- strrv / strnrv --- */
	strcpy(a, (STRPTR)"stressed");
	strrv(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strrv_reverse",
		(STRPTR)"strrv(\"stressed\")",
		(STRPTR)"strrv must reverse the entire string in place",
		(STRPTR)"desserts",
		a);

	strcpy(a, (STRPTR)"abcd");
	strrv(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strrv_even_len",
		(STRPTR)"strrv(\"abcd\")",
		(STRPTR)"strrv must reverse an even-length string correctly",
		(STRPTR)"dcba",
		a);

	strcpy(a, (STRPTR)"abcde");
	strrv(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strrv_odd_len",
		(STRPTR)"strrv(\"abcde\")",
		(STRPTR)"strrv must reverse an odd-length string correctly",
		(STRPTR)"edcba",
		a);

	strcpy(a, (STRPTR)"abcdef");
	strnrv(a, 3);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strnrv_partial",
		(STRPTR)"strnrv(\"abcdef\", 3)",
		(STRPTR)"strnrv must reverse only the first n characters",
		(STRPTR)"cbadef",
		a);

	strcpy(a, (STRPTR)"abcdef");
	strnrv(a, 0);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strnrv_zero",
		(STRPTR)"strnrv(\"abcdef\", 0)",
		(STRPTR)"strnrv with n=0 must leave the string unchanged",
		(STRPTR)"abcdef",
		a);

	strcpy(a, (STRPTR)"abcdef");
	strnrv(a, 1);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strnrv_one",
		(STRPTR)"strnrv(\"abcdef\", 1)",
		(STRPTR)"strnrv with n=1 must not change a single leading character",
		(STRPTR)"abcdef",
		a);

	/* --- strcasecmp (case-insensitive) --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strcasecmp_casefold_equal",
		(STRPTR)"strcasecmp(\"Amiga\", \"AMIGA\")",
		(STRPTR)"strcasecmp must treat differing case as equal for letters",
		0, strcasecmp((STRPTR)"Amiga", (STRPTR)"AMIGA"));

	ct_check_long((STRPTR)"string", (STRPTR)"strcasecmp_mixed_equal",
		(STRPTR)"strcasecmp(\"Hello\", \"hELLO\")",
		(STRPTR)"strcasecmp must fold case across the full string length",
		0, strcasecmp((STRPTR)"Hello", (STRPTR)"hELLO"));

	ct_check_true((STRPTR)"string", (STRPTR)"strcasecmp_unequal",
		(STRPTR)"strcasecmp(\"abc\", \"abd\") != 0",
		(STRPTR)"strcasecmp must return non-zero when strings differ beyond case",
		strcasecmp((STRPTR)"abc", (STRPTR)"abd") != 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strcasecmp_prefix_unequal",
		(STRPTR)"strcasecmp(\"abc\", \"abcd\") != 0",
		(STRPTR)"strcasecmp must detect length differences even when case matches",
		strcasecmp((STRPTR)"abc", (STRPTR)"abcd") != 0);

	ct_check_long((STRPTR)"string", (STRPTR)"strcasecmp_empty",
		(STRPTR)"strcasecmp(\"\", \"\")",
		(STRPTR)"strcasecmp of two empty strings must return 0",
		0, strcasecmp((STRPTR)"", (STRPTR)""));

	/* --- strcmpa (legacy alias of strcasecmp) --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strcmpa_casefold_equal",
		(STRPTR)"strcmpa(\"Amiga\", \"AMIGA\")",
		(STRPTR)"strcmpa must treat differing case as equal for letters",
		0, strcmpa((STRPTR)"Amiga", (STRPTR)"AMIGA"));

	ct_check_long((STRPTR)"string", (STRPTR)"strcmpa_mixed_equal",
		(STRPTR)"strcmpa(\"Hello\", \"hELLO\")",
		(STRPTR)"strcmpa must fold case across the full string length",
		0, strcmpa((STRPTR)"Hello", (STRPTR)"hELLO"));

	ct_check_true((STRPTR)"string", (STRPTR)"strcmpa_unequal",
		(STRPTR)"strcmpa(\"abc\", \"abd\") != 0",
		(STRPTR)"strcmpa must return non-zero when strings differ beyond case",
		strcmpa((STRPTR)"abc", (STRPTR)"abd") != 0);

	ct_check_true((STRPTR)"string", (STRPTR)"strcmpa_prefix_unequal",
		(STRPTR)"strcmpa(\"abc\", \"abcd\") != 0",
		(STRPTR)"strcmpa must detect length differences even when case matches",
		strcmpa((STRPTR)"abc", (STRPTR)"abcd") != 0);

	ct_check_long((STRPTR)"string", (STRPTR)"strcmpa_empty",
		(STRPTR)"strcmpa(\"\", \"\")",
		(STRPTR)"strcmpa of two empty strings must return 0",
		0, strcmpa((STRPTR)"", (STRPTR)""));

	ct_check_true((STRPTR)"string", (STRPTR)"strcmpa_matches_strcasecmp",
		(STRPTR)"strcmpa(\"AmIgA\", \"amiga\") == strcasecmp(\"AmIgA\", \"amiga\")",
		(STRPTR)"strcmpa and strcasecmp must agree (shared LVO)",
		strcmpa((STRPTR)"AmIgA", (STRPTR)"amiga")
		== strcasecmp((STRPTR)"AmIgA", (STRPTR)"amiga"));

	/* --- strncasecmp --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strncasecmp_prefix_equal",
		(STRPTR)"strncasecmp(\"AmigaOS\", \"AMIGAxx\", 5)",
		(STRPTR)"strncasecmp must compare only n characters case-insensitively",
		0, strncasecmp((STRPTR)"AmigaOS", (STRPTR)"AMIGAxx", 5));

	ct_check_true((STRPTR)"string", (STRPTR)"strncasecmp_diff_in_n",
		(STRPTR)"strncasecmp(\"abc\", \"abd\", 3) != 0",
		(STRPTR)"strncasecmp must detect a difference within n",
		strncasecmp((STRPTR)"abc", (STRPTR)"abd", 3) != 0);

	ct_check_long((STRPTR)"string", (STRPTR)"strncasecmp_n_zero",
		(STRPTR)"strncasecmp(\"a\", \"b\", 0)",
		(STRPTR)"strncasecmp with n=0 must return 0",
		0, strncasecmp((STRPTR)"a", (STRPTR)"b", 0));

	/* --- strdup / strndup --- */
	p = strdup((STRPTR)"dupme");
	ct_check_true((STRPTR)"string", (STRPTR)"strdup_content",
		(STRPTR)"strdup(\"dupme\")",
		(STRPTR)"strdup must return a heap copy of the source",
		p != (STRPTR)NULL && strcmp(p, (STRPTR)"dupme") == 0);
	if (p != (STRPTR)NULL)
		free((APTR)p);

	p = strndup((STRPTR)"ABCDEF", 3);
	ct_check_true((STRPTR)"string", (STRPTR)"strndup_3",
		(STRPTR)"strndup(\"ABCDEF\", 3)",
		(STRPTR)"strndup must copy at most n chars and NUL-terminate",
		p != (STRPTR)NULL && strcmp(p, (STRPTR)"ABC") == 0);
	if (p != (STRPTR)NULL)
		free((APTR)p);

	/* --- strnlen / stpcpy / stpncpy --- */
	ct_check_long((STRPTR)"string", (STRPTR)"strnlen_full",
		(STRPTR)"strnlen(\"hello\", 10)",
		(STRPTR)"strnlen must return strlen when shorter than maxlen",
		5, strnlen((STRPTR)"hello", 10));

	ct_check_long((STRPTR)"string", (STRPTR)"strnlen_capped",
		(STRPTR)"strnlen(\"hello\", 3)",
		(STRPTR)"strnlen must stop at maxlen when no NUL is found sooner",
		3, strnlen((STRPTR)"hello", 3));

	q = stpcpy(a, (STRPTR)"xy");
	ct_check_true((STRPTR)"string", (STRPTR)"stpcpy_return",
		(STRPTR)"stpcpy(a, \"xy\")",
		(STRPTR)"stpcpy must return a pointer to the terminating NUL",
		q != (STRPTR)NULL && q == a + 2 && strcmp(a, (STRPTR)"xy") == 0);

	memset(a, 'Z', 8);
	q = stpncpy(a, (STRPTR)"hi", 5);
	ct_check_true((STRPTR)"string", (STRPTR)"stpncpy_pad",
		(STRPTR)"stpncpy(a, \"hi\", 5)",
		(STRPTR)"stpncpy must copy, pad with NULs, and return the last byte pointer",
		q == a + 4 && a[0] == 'h' && a[1] == 'i' && a[2] == 0 && a[3] == 0 && a[4] == 0);

	/* --- strtok --- */
	strcpy(a, (STRPTR)"one two three");
	p = strtok(a, (STRPTR)" ");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_first",
		(STRPTR)"strtok(a, \" \") first call on \"one two three\"",
		(STRPTR)"first strtok token must be the leading word",
		(STRPTR)"one",
		p);

	q = strtok((STRPTR)NULL, (STRPTR)" ");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_second",
		(STRPTR)"strtok(NULL, \" \") second call",
		(STRPTR)"second strtok token must be the middle word",
		(STRPTR)"two",
		q);

	q = strtok((STRPTR)NULL, (STRPTR)" ");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_third",
		(STRPTR)"strtok(NULL, \" \") third call",
		(STRPTR)"third strtok token must be the final word",
		(STRPTR)"three",
		q);

	q = strtok((STRPTR)NULL, (STRPTR)" ");
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strtok_done",
		(STRPTR)"strtok(NULL, \" \") after last token",
		(STRPTR)"further strtok must return NULL when tokens are exhausted",
		(APTR)q);

	strcpy(b, (STRPTR)"a;b,c");
	p = strtok(b, (STRPTR)";,");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_multi_delim_first",
		(STRPTR)"strtok(\"a;b,c\", \";,\") first token",
		(STRPTR)"strtok must treat any character in the delimiter set as a separator",
		(STRPTR)"a",
		p);

	q = strtok((STRPTR)NULL, (STRPTR)";,");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_multi_delim_second",
		(STRPTR)"strtok(NULL, \";,\") second token",
		(STRPTR)"strtok must continue splitting on multiple delimiter characters",
		(STRPTR)"b",
		q);

	q = strtok((STRPTR)NULL, (STRPTR)";,");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_multi_delim_third",
		(STRPTR)"strtok(NULL, \";,\") third token",
		(STRPTR)"strtok must yield the segment after a comma delimiter",
		(STRPTR)"c",
		q);

	strcpy(c, (STRPTR)"a,,b");
	p = strtok(c, (STRPTR)",");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_consec_first",
		(STRPTR)"strtok(\"a,,b\", \",\") first token",
		(STRPTR)"first token before consecutive delimiters",
		(STRPTR)"a",
		p);

	q = strtok((STRPTR)NULL, (STRPTR)",");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_consec_second",
		(STRPTR)"strtok(NULL, \",\") skipping empty field",
		(STRPTR)"strtok must skip consecutive delimiters without returning empty tokens",
		(STRPTR)"b",
		q);

	q = strtok((STRPTR)NULL, (STRPTR)",");
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strtok_consec_end",
		(STRPTR)"strtok(NULL, \",\") after consecutive-delimiter string",
		(STRPTR)"strtok must return NULL at end of token stream",
		(APTR)q);

	strcpy(a, (STRPTR)"solo");
	p = strtok(a, (STRPTR)" ");
	ct_check_string((STRPTR)"string",
		(STRPTR)"strtok_single_token",
		(STRPTR)"strtok(\"solo\", \" \")",
		(STRPTR)"strtok on a string with no delimiters must return the whole string",
		(STRPTR)"solo",
		p);

	q = strtok((STRPTR)NULL, (STRPTR)" ");
	ct_check_ptr_null((STRPTR)"string", (STRPTR)"strtok_single_end",
		(STRPTR)"strtok(NULL, \" \") after single-token string",
		(STRPTR)"second strtok call must return NULL when only one token existed",
		(APTR)q);

	/* --- combination scenarios --- */
	strcpy(a, (STRPTR)"foo");
	strcat(a, (STRPTR)"bar");
	n = strlen(a);
	ct_check_long((STRPTR)"string", (STRPTR)"combo_strcpy_strcat_strlen",
		(STRPTR)"strlen after strcpy+strcat building \"foobar\"",
		(STRPTR)"chained strcpy and strcat must produce a string whose strlen matches total length",
		6, n);

	strcpy(a, (STRPTR)"hello");
	strupr(a);
	ct_check_long((STRPTR)"string", (STRPTR)"combo_strupr_strcmp",
		(STRPTR)"strcmp(\"HELLO\", a) after strupr",
		(STRPTR)"strupr followed by strcmp must yield equal strings when case matches",
		0, strcmp((STRPTR)"HELLO", a));

	strcpy(a, (STRPTR)"abc");
	strrv(a);
	ct_check_long((STRPTR)"string", (STRPTR)"combo_strrv_strcmp",
		(STRPTR)"strcmp(\"cba\", a) after strrv(\"abc\")",
		(STRPTR)"strrv then strcmp must confirm the reversed result",
		0, strcmp((STRPTR)"cba", a));

	memset(a, 0, sizeof(a));
	strncpy(a, (STRPTR)"ab", 4);
	strncat(a, (STRPTR)"CDEF", 2);
	ct_check_string((STRPTR)"string",
		(STRPTR)"combo_strncpy_strncat",
		(STRPTR)"strncpy(4) then strncat(2) on padded buffer",
		(STRPTR)"strncpy zero-pad plus limited strncat must build the expected prefix",
		(STRPTR)"abCD",
		a);

	strcpy(a, (STRPTR)"");
	strcpy(b, (STRPTR)"part1");
	strcat(a, b);
	strcpy(b, (STRPTR)"part2");
	strcat(a, b);
	ct_check_string((STRPTR)"string",
		(STRPTR)"combo_multi_strcat",
		(STRPTR)"strcat chaining part1 and part2",
		(STRPTR)"multiple strcat calls must concatenate in order",
		(STRPTR)"part1part2",
		a);

	strcpy(a, (STRPTR)"MiXeD");
	strupr(a);
	ct_check_long((STRPTR)"string", (STRPTR)"combo_strupr_strcasecmp",
		(STRPTR)"strcasecmp(\"MIXED\", a) after strupr",
		(STRPTR)"strupr then strcasecmp must confirm case-insensitive equality",
		0, strcasecmp((STRPTR)"MIXED", a));

	ct_check_long((STRPTR)"string", (STRPTR)"combo_strupr_strcmpa",
		(STRPTR)"strcmpa(\"MIXED\", a) after strupr",
		(STRPTR)"strupr then strcmpa must confirm case-insensitive equality",
		0, strcmpa((STRPTR)"MIXED", a));

	strcpy(a, (STRPTR)"findme");
	p = strstr(a, (STRPTR)"nd");
	r = strchr(a, (LONG)'m');
	ct_check_true((STRPTR)"string", (STRPTR)"combo_strstr_strchr",
		(STRPTR)"strstr and strchr on same buffer",
		(STRPTR)"strstr and strchr must both locate substrings within the same buffer",
		p != (STRPTR)NULL && r != (STRPTR)NULL
		&& p[0] == 'n' && r[0] == 'm');

	strcpy(a, (STRPTR)"alpha");
	strcpy(b, (STRPTR)"beta");
	i = strcmp(a, b);
	ct_check_true((STRPTR)"string", (STRPTR)"combo_strcmp_order",
		(STRPTR)"strcmp(\"alpha\", \"beta\")",
		(STRPTR)"strcmp must order two independently built strings correctly",
		i < 0);

	strcpy(a, (STRPTR)"token1-token2");
	p = strtok(a, (STRPTR)"-");
	q = strtok((STRPTR)NULL, (STRPTR)"-");
	ct_check_string((STRPTR)"string",
		(STRPTR)"combo_strtok_pair",
		(STRPTR)"strtok on hyphen-delimited pair",
		(STRPTR)"strtok must split hyphen-delimited tokens correctly",
		(STRPTR)"token1",
		p);
	ct_check_string((STRPTR)"string",
		(STRPTR)"combo_strtok_pair_second",
		(STRPTR)"strtok second hyphen token",
		(STRPTR)"second hyphen-delimited strtok token must be token2",
		(STRPTR)"token2",
		q);

	/* --- strlcpy / strlcat / strtok_r --- */
	{
		char *save;
		ULONG ul;

		ul = strlcpy(a, (STRPTR)"hello", 4UL);
		ct_check_ulong((STRPTR)"string", (STRPTR)"strlcpy_trunc_len",
			(STRPTR)"strlcpy(a,\"hello\",4)",
			(STRPTR)"strlcpy must return the full source length",
			5UL, ul);
		ct_check_string((STRPTR)"string",
			(STRPTR)"strlcpy_trunc",
			(STRPTR)"strlcpy into size 4",
			(STRPTR)"strlcpy must NUL-terminate within the buffer",
			(STRPTR)"hel",
			a);

		strcpy(a, (STRPTR)"hi");
		ul = strlcat(a, (STRPTR)"there", 8UL);
		ct_check_ulong((STRPTR)"string", (STRPTR)"strlcat_len",
			(STRPTR)"strlcat(\"hi\",\"there\",8)",
			(STRPTR)"strlcat must return would-be total length",
			7UL, ul);
		ct_check_string((STRPTR)"string",
			(STRPTR)"strlcat_ok",
			(STRPTR)"strlcat result",
			(STRPTR)"strlcat must append within the buffer size",
			(STRPTR)"hithere",
			a);

		strcpy(a, (STRPTR)"one two");
		save = (char *)NULL;
		p = strtok_r(a, (STRPTR)" ", (APTR)&save);
		ct_check_string((STRPTR)"string",
			(STRPTR)"strtok_r_first",
			(STRPTR)"strtok_r first token",
			(STRPTR)"strtok_r must return the first token",
			(STRPTR)"one",
			p);
		q = strtok_r((STRPTR)NULL, (STRPTR)" ", (APTR)&save);
		ct_check_string((STRPTR)"string",
			(STRPTR)"strtok_r_second",
			(STRPTR)"strtok_r second token",
			(STRPTR)"strtok_r must continue with the save pointer",
			(STRPTR)"two",
			q);
	}

	strcpy(a, (STRPTR)"AbC");
	strlwr(a);
	ct_check_string((STRPTR)"string",
		(STRPTR)"strlwr_ok",
		(STRPTR)"strlwr(\"AbC\")",
		(STRPTR)"strlwr must fold ASCII letters to lower case",
		(STRPTR)"abc",
		a);
}
