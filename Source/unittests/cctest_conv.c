/*
 * cctest_conv.c - atoi/atol/atof/ltoa/stoa/toupper/tolower
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_conv(void)
{
	char buf[40];
	char buf2[40];
	DOUBLE d;
	union {
		DOUBLE d;
		ULONG w[2];
	} u;
	LONG fav[2];
	LONG n;
	LONG v;
	LONG parsed;

	ct_suite_begin((STRPTR)"conv", (STRPTR)"atoi atol atof ltoa stoa toupper tolower");

	/* --- atoi positive / negative / edge --- */
	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_pos",
		(STRPTR)"atoi(\"1234\")",
		(STRPTR)"atoi must parse a positive decimal integer",
		1234, atoi((STRPTR)"1234"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_neg",
		(STRPTR)"atoi(\"-42\")",
		(STRPTR)"atoi must parse a negative decimal integer",
		-42, atoi((STRPTR)"-42"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_zero",
		(STRPTR)"atoi(\"0\")",
		(STRPTR)"atoi of zero must return 0",
		0, atoi((STRPTR)"0"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_single_digit",
		(STRPTR)"atoi(\"7\")",
		(STRPTR)"atoi must parse a single digit",
		7, atoi((STRPTR)"7"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_plus_sign",
		(STRPTR)"atoi(\"+15\")",
		(STRPTR)"atoi must accept a leading plus sign when present",
		15, atoi((STRPTR)"+15"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_leading_ws",
		(STRPTR)"atoi(\"  7\")",
		(STRPTR)"atoi must skip leading whitespace before digits",
		7, atoi((STRPTR)"  7"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_leading_tab",
		(STRPTR)"atoi(\"\\t9\")",
		(STRPTR)"atoi must skip leading tab whitespace",
		9, atoi((STRPTR)"\t9"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_trailing_nondigit",
		(STRPTR)"atoi(\"99x\")",
		(STRPTR)"atoi must stop at the first non-digit character",
		99, atoi((STRPTR)"99x"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_ws_and_trailing",
		(STRPTR)"atoi(\"  123abc\")",
		(STRPTR)"atoi must skip leading whitespace then parse digits until non-digit",
		123, atoi((STRPTR)"  123abc"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_large_pos",
		(STRPTR)"atoi(\"99999\")",
		(STRPTR)"atoi must parse a multi-digit positive value",
		99999, atoi((STRPTR)"99999"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atoi_neg_with_ws",
		(STRPTR)"atoi(\"  -8\")",
		(STRPTR)"atoi must skip whitespace before a negative sign",
		-8, atoi((STRPTR)"  -8"));

	/* --- atol --- */
	ct_check_long((STRPTR)"conv", (STRPTR)"atol_large",
		(STRPTR)"atol(\"100000\")",
		(STRPTR)"atol must parse a large decimal long value",
		100000, atol((STRPTR)"100000"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atol_zero",
		(STRPTR)"atol(\"0\")",
		(STRPTR)"atol of zero must return 0",
		0, atol((STRPTR)"0"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atol_neg",
		(STRPTR)"atol(\"-500\")",
		(STRPTR)"atol must parse a negative long value",
		-500, atol((STRPTR)"-500"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atol_plus",
		(STRPTR)"atol(\"+250\")",
		(STRPTR)"atol must accept a leading plus sign",
		250, atol((STRPTR)"+250"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atol_ws",
		(STRPTR)"atol(\"  42\")",
		(STRPTR)"atol must skip leading whitespace",
		42, atol((STRPTR)"  42"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atol_trailing",
		(STRPTR)"atol(\"77z\")",
		(STRPTR)"atol must stop at the first non-digit after digits",
		77, atol((STRPTR)"77z"));

	ct_check_long((STRPTR)"conv", (STRPTR)"atol_single",
		(STRPTR)"atol(\"3\")",
		(STRPTR)"atol must parse a single-digit long",
		3, atol((STRPTR)"3"));

	/* --- ltoa direct --- */
	ltoa(12345, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"ltoa_pos",
		(STRPTR)"ltoa(12345, buf)",
		(STRPTR)"ltoa must write the decimal digits of a positive long",
		(STRPTR)"12345",
		buf);

	ltoa(-99, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"ltoa_neg",
		(STRPTR)"ltoa(-99, buf)",
		(STRPTR)"ltoa must include a leading minus sign for negative values",
		(STRPTR)"-99",
		buf);

	ltoa(0, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"ltoa_zero",
		(STRPTR)"ltoa(0, buf)",
		(STRPTR)"ltoa of zero must write a single zero digit",
		(STRPTR)"0",
		buf);

	ltoa(7, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"ltoa_single_digit",
		(STRPTR)"ltoa(7, buf)",
		(STRPTR)"ltoa must format a single-digit positive long",
		(STRPTR)"7",
		buf);

	ltoa(-1, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"ltoa_neg_one",
		(STRPTR)"ltoa(-1, buf)",
		(STRPTR)"ltoa of -1 must write \"-1\"",
		(STRPTR)"-1",
		buf);

	/* --- ltoa / atoi roundtrips --- */
	v = 42;
	ltoa(v, buf);
	parsed = atoi(buf);
	ct_check_long((STRPTR)"conv", (STRPTR)"roundtrip_ltoa_atoi_pos",
		(STRPTR)"atoi(ltoa(42))",
		(STRPTR)"ltoa followed by atoi must recover the original positive value",
		42, parsed);

	v = -123;
	ltoa(v, buf);
	parsed = atoi(buf);
	ct_check_long((STRPTR)"conv", (STRPTR)"roundtrip_ltoa_atoi_neg",
		(STRPTR)"atoi(ltoa(-123))",
		(STRPTR)"ltoa followed by atoi must recover the original negative value",
		-123, parsed);

	v = 0;
	ltoa(v, buf);
	parsed = atoi(buf);
	ct_check_long((STRPTR)"conv", (STRPTR)"roundtrip_ltoa_atoi_zero",
		(STRPTR)"atoi(ltoa(0))",
		(STRPTR)"ltoa of zero round-tripped through atoi must remain zero",
		0, parsed);

	v = 9999;
	ltoa(v, buf);
	parsed = atol(buf);
	ct_check_long((STRPTR)"conv", (STRPTR)"roundtrip_ltoa_atol",
		(STRPTR)"atol(ltoa(9999))",
		(STRPTR)"ltoa followed by atol must recover a multi-digit value",
		9999, parsed);

	v = 1234;
	ltoa(v, buf);
	parsed = atoi(buf);
	ltoa(parsed, buf2);
	ct_check_string((STRPTR)"conv", (STRPTR)"roundtrip_ltoa_twice",
		(STRPTR)"ltoa(atoi(ltoa(1234)))",
		(STRPTR)"double ltoa after atoi must produce the same string",
		buf, buf2);

	/* --- stoa --- */
	stoa(65, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"stoa_pos",
		(STRPTR)"stoa(65, buf)",
		(STRPTR)"stoa must convert a short-range positive value to decimal digits",
		(STRPTR)"65",
		buf);

	stoa(-12, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"stoa_neg",
		(STRPTR)"stoa(-12, buf)",
		(STRPTR)"stoa must format a negative short-range value with a minus sign",
		(STRPTR)"-12",
		buf);

	stoa(0, buf);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"stoa_zero",
		(STRPTR)"stoa(0, buf)",
		(STRPTR)"stoa of zero must write \"0\"",
		(STRPTR)"0",
		buf);

	stoa(9, buf);
	parsed = atoi(buf);
	ct_check_long((STRPTR)"conv", (STRPTR)"roundtrip_stoa_atoi",
		(STRPTR)"atoi(stoa(9))",
		(STRPTR)"stoa followed by atoi must recover the original value",
		9, parsed);

	stoa(100, buf);
	parsed = atoi(buf);
	ct_check_long((STRPTR)"conv", (STRPTR)"roundtrip_stoa_atoi_100",
		(STRPTR)"atoi(stoa(100))",
		(STRPTR)"stoa of 100 round-tripped through atoi must equal 100",
		100, parsed);

	/* --- toupper --- */
	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_a",
		(STRPTR)"toupper('a')",
		(STRPTR)"toupper of lowercase 'a' must return 'A'",
		(LONG)'A', toupper((LONG)'a'));

	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_z",
		(STRPTR)"toupper('z')",
		(STRPTR)"toupper of lowercase 'z' must return 'Z'",
		(LONG)'Z', toupper((LONG)'z'));

	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_m",
		(STRPTR)"toupper('m')",
		(STRPTR)"toupper of a middle lowercase letter must map correctly",
		(LONG)'M', toupper((LONG)'m'));

	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_already_upper",
		(STRPTR)"toupper('A')",
		(STRPTR)"toupper of an already-uppercase letter must leave it unchanged",
		(LONG)'A', toupper((LONG)'A'));

	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_digit",
		(STRPTR)"toupper('3')",
		(STRPTR)"toupper must leave digit characters unchanged",
		(LONG)'3', toupper((LONG)'3'));

	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_punct",
		(STRPTR)"toupper('@')",
		(STRPTR)"toupper must leave punctuation unchanged",
		(LONG)'@', toupper((LONG)'@'));

	ct_check_long((STRPTR)"conv", (STRPTR)"toupper_space",
		(STRPTR)"toupper(' ')",
		(STRPTR)"toupper must leave whitespace unchanged",
		(LONG)' ', toupper((LONG)' '));

	/* --- tolower --- */
	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_Z",
		(STRPTR)"tolower('Z')",
		(STRPTR)"tolower of uppercase 'Z' must return 'z'",
		(LONG)'z', tolower((LONG)'Z'));

	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_A",
		(STRPTR)"tolower('A')",
		(STRPTR)"tolower of uppercase 'A' must return 'a'",
		(LONG)'a', tolower((LONG)'A'));

	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_M",
		(STRPTR)"tolower('M')",
		(STRPTR)"tolower of a middle uppercase letter must map correctly",
		(LONG)'m', tolower((LONG)'M'));

	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_already_lower",
		(STRPTR)"tolower('a')",
		(STRPTR)"tolower of an already-lowercase letter must leave it unchanged",
		(LONG)'a', tolower((LONG)'a'));

	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_digit",
		(STRPTR)"tolower('5')",
		(STRPTR)"tolower must leave digit characters unchanged",
		(LONG)'5', tolower((LONG)'5'));

	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_punct",
		(STRPTR)"tolower('#')",
		(STRPTR)"tolower must leave punctuation unchanged",
		(LONG)'#', tolower((LONG)'#'));

	ct_check_long((STRPTR)"conv", (STRPTR)"tolower_space",
		(STRPTR)"tolower(' ')",
		(STRPTR)"tolower must leave whitespace unchanged",
		(LONG)' ', tolower((LONG)' '));

	/* --- atof with sprintf roundtrips --- */
	d = atof((STRPTR)"0");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.1f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_zero",
		(STRPTR)"sprintf(\"%%.1f\", atof(\"0\"))",
		(STRPTR)"atof of zero formatted with %.1f must be \"0.0\"",
		(STRPTR)"0.0",
		buf);

	d = atof((STRPTR)"1");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.1f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_one",
		(STRPTR)"sprintf(\"%%.1f\", atof(\"1\"))",
		(STRPTR)"atof of 1 formatted with %.1f must be \"1.0\"",
		(STRPTR)"1.0",
		buf);

	d = atof((STRPTR)"-1");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.1f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_neg_one",
		(STRPTR)"sprintf(\"%%.1f\", atof(\"-1\"))",
		(STRPTR)"atof of -1 formatted with %.1f must be \"-1.0\"",
		(STRPTR)"-1.0",
		buf);

	d = atof((STRPTR)"12.5");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	n = sprintf(buf, (STRPTR)"%.1f", (APTR)fav);
	ct_check_true((STRPTR)"conv", (STRPTR)"atof_12_5_len",
		(STRPTR)"sprintf length for atof(\"12.5\")",
		(STRPTR)"sprintf of atof(\"12.5\") with %.1f must produce a non-empty string",
		n > 0);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_12_5_value",
		(STRPTR)"sprintf(\"%%.1f\", atof(\"12.5\"))",
		(STRPTR)"atof(\"12.5\") formatted with %.1f must be \"12.5\"",
		(STRPTR)"12.5",
		buf);

	d = atof((STRPTR)"-3.25");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.2f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_neg_frac",
		(STRPTR)"sprintf(\"%%.2f\", atof(\"-3.25\"))",
		(STRPTR)"atof(\"-3.25\") formatted with %.2f must be \"-3.25\"",
		(STRPTR)"-3.25",
		buf);

	d = atof((STRPTR)"1e3");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.0f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_scientific",
		(STRPTR)"sprintf(\"%%.0f\", atof(\"1e3\"))",
		(STRPTR)"atof(\"1e3\") formatted with %.0f must be \"1000\"",
		(STRPTR)"1000",
		buf);

	d = atof((STRPTR)"0.5");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.1f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_half",
		(STRPTR)"sprintf(\"%%.1f\", atof(\"0.5\"))",
		(STRPTR)"atof(\"0.5\") formatted with %.1f must be \"0.5\"",
		(STRPTR)"0.5",
		buf);

	d = atof((STRPTR)"0.5");
	u.d = d;
	fav[0] = (LONG)u.w[0];
	fav[1] = (LONG)u.w[1];
	sprintf(buf, (STRPTR)"%.2f", (APTR)fav);
	ct_check_string((STRPTR)"conv",
		(STRPTR)"atof_half_two_digits",
		(STRPTR)"sprintf(\"%%.2f\", atof(\"0.5\"))",
		(STRPTR)"atof(\"0.5\") formatted with %.2f must be \"0.50\"",
		(STRPTR)"0.50",
		buf);

	/* --- strtol / strtoul / atoll / labs / div / rand --- */
	{
		char *endp;
		struct { LONG quot; LONG rem; } dr;
		struct { LONG quot; LONG rem; } lr;
		ULONG u;
		LONG r1;
		LONG r2;

		ct_check_long((STRPTR)"conv", (STRPTR)"strtol_dec",
			(STRPTR)"strtol(\"1234\", NULL, 10)",
			(STRPTR)"strtol base 10 must parse decimal",
			1234, strtol((STRPTR)"1234", (APTR)NULL, 10L));

		n = strtol((STRPTR)"-42xyz", (APTR)&endp, 10L);
		ct_check_long((STRPTR)"conv", (STRPTR)"strtol_neg_end",
			(STRPTR)"strtol(\"-42xyz\", &end, 10)",
			(STRPTR)"strtol must stop at first non-digit and set endptr",
			-42, n);
		ct_check_true((STRPTR)"conv", (STRPTR)"strtol_endptr",
			(STRPTR)"*end == 'x' after strtol(\"-42xyz\")",
			(STRPTR)"endptr must point at the first unconverted character",
			endp != (char *)NULL && *endp == 'x');

		ct_check_long((STRPTR)"conv", (STRPTR)"strtol_hex",
			(STRPTR)"strtol(\"0xFF\", NULL, 0)",
			(STRPTR)"strtol base 0 must accept 0x hex prefix",
			255, strtol((STRPTR)"0xFF", (APTR)NULL, 0L));

		u = strtoul((STRPTR)"4294967295", (APTR)NULL, 10L);
		ct_check_ulong((STRPTR)"conv", (STRPTR)"strtoul_max",
			(STRPTR)"strtoul(\"4294967295\", NULL, 10)",
			(STRPTR)"strtoul must parse ULONG_MAX decimal",
			0xFFFFFFFFUL, u);

		ct_check_long((STRPTR)"conv", (STRPTR)"atoll_pos",
			(STRPTR)"atoll(\"99\")",
			(STRPTR)"atoll must parse like strtol base 10",
			99, atoll((STRPTR)"99"));

		ct_check_long((STRPTR)"conv", (STRPTR)"labs_neg",
			(STRPTR)"labs(-5)",
			(STRPTR)"labs must return the absolute value of a negative long",
			5, labs(-5L));

		div((APTR)&dr, 10L, 3L);
		ct_check_long((STRPTR)"conv", (STRPTR)"div_quot",
			(STRPTR)"div(10,3).quot",
			(STRPTR)"div must store toward-zero quotient",
			3, dr.quot);
		ct_check_long((STRPTR)"conv", (STRPTR)"div_rem",
			(STRPTR)"div(10,3).rem",
			(STRPTR)"div must store the corresponding remainder",
			1, dr.rem);

		ldiv((APTR)&lr, -10L, 3L);
		ct_check_long((STRPTR)"conv", (STRPTR)"ldiv_quot",
			(STRPTR)"ldiv(-10,3).quot",
			(STRPTR)"ldiv toward-zero quotient for negative dividend",
			-3, lr.quot);

		srand(1UL);
		r1 = rand();
		srand(1UL);
		r2 = rand();
		ct_check_long((STRPTR)"conv", (STRPTR)"rand_repeatable",
			(STRPTR)"rand after srand(1) twice",
			(STRPTR)"same seed must yield the same first rand value",
			r1, r2);
		ct_check_true((STRPTR)"conv", (STRPTR)"rand_range",
			(STRPTR)"rand() in 0..RAND_MAX",
			(STRPTR)"rand must stay within 0..32767",
			r1 >= 0 && r1 <= 32767);
	}
}