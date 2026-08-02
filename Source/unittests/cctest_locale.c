/*
 * cctest_locale.c - setlocale / localeconv / strcoll / strxfrm ("C")
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

/* Category constants match <locale.h>; avoid including it beside pragmas. */
#define CT_LC_ALL 0

struct ct_lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
};

void
ct_suite_locale(void)
{
	STRPTR name;
	APTR lc;
	char buf[32];
	ULONG n;
	LONG c;

	ct_suite_begin((STRPTR)"locale", (STRPTR)"setlocale localeconv strcoll strxfrm");

	name = setlocale(CT_LC_ALL, (STRPTR)NULL);
	ct_check_string((STRPTR)"locale",
		(STRPTR)"query_default",
		(STRPTR)"setlocale(LC_ALL, NULL)",
		(STRPTR)"default locale name must be \"C\"",
		(STRPTR)"C",
		name);

	name = setlocale(CT_LC_ALL, (STRPTR)"C");
	ct_check_string((STRPTR)"locale",
		(STRPTR)"set_C",
		(STRPTR)"setlocale(LC_ALL, \"C\")",
		(STRPTR)"setting \"C\" must succeed and report \"C\"",
		(STRPTR)"C",
		name);

	lc = localeconv();
	ct_check_ptr_nonnull((STRPTR)"locale", (STRPTR)"localeconv_ptr",
		(STRPTR)"localeconv()",
		(STRPTR)"localeconv must return a non-NULL lconv pointer",
		lc);
	if (lc != (APTR)NULL)
	{
		ct_check_string((STRPTR)"locale",
			(STRPTR)"localeconv_dot",
			(STRPTR)"localeconv()->decimal_point",
			(STRPTR)"\"C\" locale decimal_point must be \".\"",
			(STRPTR)".",
			((struct ct_lconv *)lc)->decimal_point);
	}

	c = strcoll((STRPTR)"abc", (STRPTR)"abd");
	ct_check_true((STRPTR)"locale", (STRPTR)"strcoll_order",
		(STRPTR)"strcoll(\"abc\",\"abd\")",
		(STRPTR)"strcoll in \"C\" must order like strcmp",
		c < 0);

	n = strxfrm(buf, (STRPTR)"hi", 32UL);
	ct_check_ulong((STRPTR)"locale", (STRPTR)"strxfrm_len",
		(STRPTR)"strxfrm(\"hi\", 32)",
		(STRPTR)"strxfrm must return the source length",
		2UL, n);
	ct_check_string((STRPTR)"locale",
		(STRPTR)"strxfrm_copy",
		(STRPTR)"strxfrm result",
		(STRPTR)"\"C\" strxfrm must copy the source",
		(STRPTR)"hi",
		buf);
}
