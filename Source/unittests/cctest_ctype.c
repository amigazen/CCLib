/*
 * cctest_ctype.c - toupper / tolower / is* LVOs
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#define CC_CTYPE_FUNCS 1
#include <ctype.h>
#include "cctest_util.h"

void
ct_suite_ctype(void)
{
	LONG c;
	LONG r;
	LONG i;
	char nbuf[24];
	STRPTR name;

	ct_suite_begin((STRPTR)"ctype",
		(STRPTR)"toupper / tolower / is* LVOs");

	ct_check_true((STRPTR)"ctype", (STRPTR)"isblank_space",
		(STRPTR)"isblank(' ')",
		(STRPTR)"isblank must be true for space",
		isblank(' ') != 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isblank_tab",
		(STRPTR)"isblank('\\t')",
		(STRPTR)"isblank must be true for horizontal tab",
		isblank('\t') != 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isblank_newline_false",
		(STRPTR)"!isblank('\\n')",
		(STRPTR)"isblank must be false for newline (unlike isspace)",
		isblank('\n') == 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isblank_letter_false",
		(STRPTR)"!isblank('a')",
		(STRPTR)"isblank must be false for letters",
		isblank('a') == 0);

	ct_check_true((STRPTR)"ctype", (STRPTR)"isalpha_a",
		(STRPTR)"isalpha('a')",
		(STRPTR)"isalpha LVO must be true for 'a' in \"C\"",
		isalpha((LONG)'a') != 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isdigit_5",
		(STRPTR)"isdigit('5')",
		(STRPTR)"isdigit LVO must be true for '5'",
		isdigit((LONG)'5') != 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isdigit_a_false",
		(STRPTR)"!isdigit('a')",
		(STRPTR)"isdigit LVO must be false for letters",
		isdigit((LONG)'a') == 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isspace_space",
		(STRPTR)"isspace(' ')",
		(STRPTR)"isspace LVO must be true for space",
		isspace((LONG)' ') != 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isascii_A",
		(STRPTR)"isascii('A')",
		(STRPTR)"isascii LVO must be true for 7-bit 'A'",
		isascii((LONG)'A') != 0);
	ct_check_true((STRPTR)"ctype", (STRPTR)"isascii_eof_false",
		(STRPTR)"!isascii(-1)",
		(STRPTR)"isascii/is* LVOs must treat EOF as false",
		isascii(-1L) == 0 && isalpha(-1L) == 0);

	c = 'a';
	r = toupper(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"toupper_a",
		(STRPTR)"toupper('a')",
		(STRPTR)"toupper('a') must return 'A'",
		(LONG)'A', r);

	c = 'z';
	r = toupper(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"toupper_z",
		(STRPTR)"toupper('z')",
		(STRPTR)"toupper('z') must return 'Z'",
		(LONG)'Z', r);

	c = 'm';
	r = toupper(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"toupper_m",
		(STRPTR)"toupper('m')",
		(STRPTR)"toupper('m') must return 'M'",
		(LONG)'M', r);

	c = 'A';
	r = toupper(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"toupper_A_unchanged",
		(STRPTR)"toupper('A')",
		(STRPTR)"toupper of an uppercase letter must leave it unchanged",
		(LONG)'A', r);

	c = '0';
	r = toupper(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"toupper_digit",
		(STRPTR)"toupper('0')",
		(STRPTR)"toupper of a digit must leave it unchanged",
		(LONG)'0', r);

	c = '!';
	r = toupper(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"toupper_punct",
		(STRPTR)"toupper('!')",
		(STRPTR)"toupper of punctuation must leave it unchanged",
		(LONG)'!', r);

	c = 'A';
	r = tolower(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"tolower_A",
		(STRPTR)"tolower('A')",
		(STRPTR)"tolower('A') must return 'a'",
		(LONG)'a', r);

	c = 'Z';
	r = tolower(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"tolower_Z",
		(STRPTR)"tolower('Z')",
		(STRPTR)"tolower('Z') must return 'z'",
		(LONG)'z', r);

	c = 'M';
	r = tolower(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"tolower_M",
		(STRPTR)"tolower('M')",
		(STRPTR)"tolower('M') must return 'm'",
		(LONG)'m', r);

	c = 'a';
	r = tolower(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"tolower_a_unchanged",
		(STRPTR)"tolower('a')",
		(STRPTR)"tolower of a lowercase letter must leave it unchanged",
		(LONG)'a', r);

	c = '9';
	r = tolower(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"tolower_digit",
		(STRPTR)"tolower('9')",
		(STRPTR)"tolower of a digit must leave it unchanged",
		(LONG)'9', r);

	c = '#';
	r = tolower(c);
	ct_check_long((STRPTR)"ctype", (STRPTR)"tolower_punct",
		(STRPTR)"tolower('#')",
		(STRPTR)"tolower of punctuation must leave it unchanged",
		(LONG)'#', r);

	i = 0;
	while (i < 26)
	{
		char callbuf[40];

		c = 'a' + i;
		sprintf(nbuf, (STRPTR)"toupper_lc_%ld", i);
		name = nbuf;
		sprintf(callbuf, (STRPTR)"toupper('%c')", c);
		r = toupper(c);
		ct_check_long((STRPTR)"ctype", name, callbuf,
			(STRPTR)"Each lowercase a-z must map to the matching uppercase A-Z",
			(LONG)('A' + i), r);
		i++;
	}

	i = 0;
	while (i < 26)
	{
		char callbuf[40];

		c = 'A' + i;
		sprintf(nbuf, (STRPTR)"tolower_uc_%ld", i);
		name = nbuf;
		sprintf(callbuf, (STRPTR)"tolower('%c')", c);
		r = tolower(c);
		ct_check_long((STRPTR)"ctype", name, callbuf,
			(STRPTR)"Each uppercase A-Z must map to the matching lowercase a-z",
			(LONG)('a' + i), r);
		i++;
	}
}
