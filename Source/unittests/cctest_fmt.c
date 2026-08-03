/*
 * cctest_fmt.c - sprintf / snprintf / sscanf formatting
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_fmt(void)
{
	char buf[128];
	char sbuf[64];
	char cbuf[8];
	LONG av[8];
	APTR args[8];
	LONG n;
	LONG v;
	LONG v2;
	LONG v3;
	ULONG uv;
	DOUBLE d;

	ct_suite_begin((STRPTR)"fmt", (STRPTR)"sprintf / snprintf / sscanf");

	buf[0] = '\0';
	n = sprintf(buf, (STRPTR)"");
	ct_check_long((STRPTR)"fmt", (STRPTR)"sprintf_empty_len",
		(STRPTR)"sprintf with empty format",
		(STRPTR)"sprintf with empty format must return 0",
		0, n);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_empty_str",
		(STRPTR)"sprintf with empty format",
		(STRPTR)"sprintf with empty format must leave an empty string",
		(STRPTR)"",
		buf);

	strcpy(buf, (STRPTR)"xy");
	n = sprintf(buf, (STRPTR)"literal");
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_literal",
		(STRPTR)"sprintf with literal-only format",
		(STRPTR)"sprintf with literal-only format must copy the text",
		(STRPTR)"literal",
		buf);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sprintf_literal_len",
		(STRPTR)"sprintf literal",
		(STRPTR)"sprintf literal must return length 7",
		7, n);

	n = sprintf(buf, (STRPTR)"n=%ld", 7);
	ct_check_true((STRPTR)"fmt", (STRPTR)"sprintf_ld_len",
		(STRPTR)"sprintf(\"n=%%ld\", 7)",
		(STRPTR)"sprintf(\"n=%%ld\", 7) must return a positive length",
		n > 0);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_ld",
		(STRPTR)"sprintf with %%ld",
		(STRPTR)"sprintf with %%ld must format the long 7 as \"n=7\"",
		(STRPTR)"n=7",
		buf);

	sprintf(buf, (STRPTR)"%ld", -123);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_ld_neg",
		(STRPTR)"sprintf %%ld",
		(STRPTR)"sprintf %%ld must format -123 with a leading minus",
		(STRPTR)"-123",
		buf);

	sprintf(buf, (STRPTR)"%ld", 0);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_ld_zero",
		(STRPTR)"sprintf %%ld of zero",
		(STRPTR)"sprintf %%ld of zero must be \"0\"",
		(STRPTR)"0",
		buf);

	sprintf(buf, (STRPTR)"%d", 456);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_d",
		(STRPTR)"sprintf with %%d",
		(STRPTR)"sprintf with %%d must format 456 as decimal",
		(STRPTR)"456",
		buf);

	sprintf(buf, (STRPTR)"%d", -9);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_d_neg",
		(STRPTR)"sprintf with %%d",
		(STRPTR)"sprintf with %%d must format -9",
		(STRPTR)"-9",
		buf);

	sprintf(buf, (STRPTR)"hello %s", (STRPTR)"world");
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_s",
		(STRPTR)"sprintf with %%s",
		(STRPTR)"sprintf with %%s must interpolate the string argument",
		(STRPTR)"hello world",
		buf);

	sprintf(buf, (STRPTR)"|%5s|", (STRPTR)"ab");
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_s_width",
		(STRPTR)"sprintf %%5s",
		(STRPTR)"sprintf %%5s must right-pad with spaces",
		(STRPTR)"|   ab|",
		buf);

	sprintf(buf, (STRPTR)"|%-5s|", (STRPTR)"ab");
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_s_left",
		(STRPTR)"sprintf %%-5s",
		(STRPTR)"sprintf %%-5s must left-align",
		(STRPTR)"|ab   |",
		buf);

	sprintf(buf, (STRPTR)"%c", (LONG)'Q');
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_c",
		(STRPTR)"sprintf with %%c",
		(STRPTR)"sprintf with %%c must emit the character Q",
		(STRPTR)"Q",
		buf);

	sprintf(buf, (STRPTR)"%x", 255);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_x",
		(STRPTR)"sprintf with %%x",
		(STRPTR)"sprintf with %%x must format 255 as lowercase hex \"ff\"",
		(STRPTR)"ff",
		buf);

	sprintf(buf, (STRPTR)"%X", 255);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_X",
		(STRPTR)"sprintf with %%X",
		(STRPTR)"sprintf with %%X must format 255 as uppercase hex \"FF\"",
		(STRPTR)"FF",
		buf);

	sprintf(buf, (STRPTR)"%o", 8);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_o",
		(STRPTR)"sprintf with %%o",
		(STRPTR)"sprintf with %%o must format 8 as octal \"10\"",
		(STRPTR)"10",
		buf);

	sprintf(buf, (STRPTR)"%u", 42);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_u",
		(STRPTR)"sprintf with %%u",
		(STRPTR)"sprintf with %%u must format 42 as unsigned decimal",
		(STRPTR)"42",
		buf);

	sprintf(buf, (STRPTR)"|%5ld|", 42);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_width",
		(STRPTR)"sprintf with width 5",
		(STRPTR)"sprintf with width 5 must right-pad: \"|   42|\"",
		(STRPTR)"|   42|",
		buf);

	sprintf(buf, (STRPTR)"|%-5ld|", 42);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_left",
		(STRPTR)"sprintf with %%-5ld",
		(STRPTR)"sprintf with %%-5ld must left-align: \"|42   |\"",
		(STRPTR)"|42   |",
		buf);

	sprintf(buf, (STRPTR)"%05ld", 7);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_zpad",
		(STRPTR)"sprintf %%05ld",
		(STRPTR)"sprintf %%05ld must zero-pad to width 5",
		(STRPTR)"00007",
		buf);

	sprintf(buf, (STRPTR)"%ld+%ld", 3, 4);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_two_args",
		(STRPTR)"sprintf",
		(STRPTR)"sprintf must consume two long arguments in order",
		(STRPTR)"3+4",
		buf);

	sprintf(buf, (STRPTR)"%ld/%ld/%ld", 1, 2, 3);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_three_args",
		(STRPTR)"sprintf",
		(STRPTR)"sprintf must format three longs in order",
		(STRPTR)"1/2/3",
		buf);

	sprintf(buf, (STRPTR)"%s=%d", (STRPTR)"A", 99);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_mixed_args",
		(STRPTR)"sprintf",
		(STRPTR)"sprintf must mix %%s and %%d arguments",
		(STRPTR)"A=99",
		buf);

	sprintf(buf, (STRPTR)"100%% done");
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_percent",
		(STRPTR)"%%%% in the format",
		(STRPTR)"%%%% in the format must emit a single percent character",
		(STRPTR)"100% done",
		buf);

	av[0] = 99;
	n = vsprintf(buf, (STRPTR)"vs=%ld", (APTR)av);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"vsprintf_ld",
		(STRPTR)"vsprintf",
		(STRPTR)"vsprintf must format like sprintf using an explicit arg block",
		(STRPTR)"vs=99",
		buf);
	ct_check_true((STRPTR)"fmt", (STRPTR)"vsprintf_len",
		(STRPTR)"vsprintf",
		(STRPTR)"vsprintf must return the number of characters written",
		n >= 5);

	av[0] = (LONG)(STRPTR)"ok";
	av[1] = 5;
	n = vsprintf(buf, (STRPTR)"%s:%ld", (APTR)av);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"vsprintf_two",
		(STRPTR)"vsprintf",
		(STRPTR)"vsprintf must handle multiple arguments",
		(STRPTR)"ok:5",
		buf);
	ct_check_long((STRPTR)"fmt", (STRPTR)"vsprintf_two_len",
		(STRPTR)"vsprintf \"ok:5\"",
		(STRPTR)"vsprintf \"ok:5\" must return length 4",
		4, n);

	/* --- snprintf (varargs tagcall) / vsnprintf (APTR args LVO) --- */
	memset(buf, (ULONG)'X', 16);
	n = snprintf(buf, 16UL, (STRPTR)"n=%ld", 42L);
	ct_check_long((STRPTR)"fmt", (STRPTR)"snprintf_fit_len",
		(STRPTR)"snprintf(buf,16,\"n=%%ld\",42)",
		(STRPTR)"snprintf must return the untruncated length",
		4, n);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"snprintf_fit",
		(STRPTR)"snprintf fitting in buffer",
		(STRPTR)"snprintf with room must write the full string",
		(STRPTR)"n=42",
		buf);

	memset(buf, (ULONG)'X', 16);
	n = snprintf(buf, 5UL, (STRPTR)"n=%ld", 12345L);
	ct_check_long((STRPTR)"fmt", (STRPTR)"snprintf_trunc_len",
		(STRPTR)"snprintf(buf,5,\"n=%%ld\",12345)",
		(STRPTR)"snprintf must return full length even when truncated",
		7, n);
	ct_check_true((STRPTR)"fmt", (STRPTR)"snprintf_trunc_nul",
		(STRPTR)"snprintf truncated buffer",
		(STRPTR)"truncated snprintf must NUL-terminate within size",
		buf[0] == 'n' && buf[1] == '=' && buf[2] == '1' && buf[3] == '2'
		&& buf[4] == '\0');

	n = snprintf((STRPTR)NULL, 0UL, (STRPTR)"ab%ld", 12345L);
	ct_check_long((STRPTR)"fmt", (STRPTR)"snprintf_size0",
		(STRPTR)"snprintf(NULL,0,\"ab%%ld\",12345)",
		(STRPTR)"snprintf with size 0 must return would-be length without writing",
		7, n);

	av[0] = 7;
	n = vsnprintf(buf, 8UL, (STRPTR)"v=%ld", (APTR)av);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"vsnprintf_ok",
		(STRPTR)"vsnprintf",
		(STRPTR)"vsnprintf must format like snprintf",
		(STRPTR)"v=7",
		buf);
	ct_check_long((STRPTR)"fmt", (STRPTR)"vsnprintf_len",
		(STRPTR)"vsnprintf \"v=7\"",
		(STRPTR)"vsnprintf must return length 3",
		3, n);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"123", (STRPTR)"%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_ld_count",
		(STRPTR)"sscanf(\"123\",\"%%ld\")",
		(STRPTR)"sscanf(\"123\",\"%%ld\") must assign 1 field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_ld_value",
		(STRPTR)"sscanf",
		(STRPTR)"sscanf must store 123 into the long destination",
		123, v);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"  456", (STRPTR)"%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_ld_ws",
		(STRPTR)"sscanf",
		(STRPTR)"sscanf must skip leading whitespace before %%ld",
		456, v);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"-77", (STRPTR)"%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_ld_neg",
		(STRPTR)"sscanf",
		(STRPTR)"sscanf must parse a negative long",
		-77, v);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"321", (STRPTR)"%d", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_d_count",
		(STRPTR)"sscanf with %%d",
		(STRPTR)"sscanf with %%d must assign one field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_d_value",
		(STRPTR)"sscanf %%d",
		(STRPTR)"sscanf %%d must store 321",
		321, v);

	uv = 0;
	args[0] = (APTR)&uv;
	n = sscanf((STRPTR)"88", (STRPTR)"%u", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_u_count",
		(STRPTR)"sscanf with %%u",
		(STRPTR)"sscanf with %%u must assign one field",
		1, n);
	ct_check_ulong((STRPTR)"fmt", (STRPTR)"sscanf_u_value",
		(STRPTR)"sscanf %%u",
		(STRPTR)"sscanf %%u must store 88",
		(ULONG)88, uv);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"ff", (STRPTR)"%x", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_x_count",
		(STRPTR)"sscanf with %%x",
		(STRPTR)"sscanf with %%x must assign one field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_x_value",
		(STRPTR)"sscanf %%x",
		(STRPTR)"sscanf %%x must parse hex ff as 255",
		255, v);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"AB", (STRPTR)"%X", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_X_value",
		(STRPTR)"sscanf %%X",
		(STRPTR)"sscanf %%X must parse AB as 171",
		171, v);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"17", (STRPTR)"%o", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_o_value",
		(STRPTR)"sscanf %%o",
		(STRPTR)"sscanf %%o must parse octal 17 as 15",
		15, v);

	sbuf[0] = '\0';
	args[0] = (APTR)sbuf;
	n = sscanf((STRPTR)"hello", (STRPTR)"%s", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_s_count",
		(STRPTR)"sscanf %%s",
		(STRPTR)"sscanf %%s must assign one field",
		1, n);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sscanf_s_value",
		(STRPTR)"sscanf %%s",
		(STRPTR)"sscanf %%s must copy \"hello\"",
		(STRPTR)"hello",
		sbuf);

	cbuf[0] = '\0';
	args[0] = (APTR)cbuf;
	n = sscanf((STRPTR)"Z", (STRPTR)"%c", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_c_count",
		(STRPTR)"sscanf %%c",
		(STRPTR)"sscanf %%c must assign one field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_c_value",
		(STRPTR)"sscanf %%c",
		(STRPTR)"sscanf %%c must store 'Z'",
		(LONG)'Z', (LONG)cbuf[0]);

	sbuf[0] = '\0';
	v = 0;
	args[0] = (APTR)sbuf;
	args[1] = (APTR)&v;
	n = sscanf((STRPTR)"hi 9", (STRPTR)"%s %ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_two_count",
		(STRPTR)"sscanf of \"hi 9\" with %%s %%ld",
		(STRPTR)"sscanf of \"hi 9\" with %%s %%ld must assign 2 fields",
		2, n);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sscanf_two_str",
		(STRPTR)"First field",
		(STRPTR)"First field must be the string \"hi\"",
		(STRPTR)"hi",
		sbuf);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_two_long",
		(STRPTR)"Second field",
		(STRPTR)"Second field must be the long 9",
		9, v);

	v = 0;
	v2 = 0;
	v3 = 0;
	args[0] = (APTR)&v;
	args[1] = (APTR)&v2;
	args[2] = (APTR)&v3;
	n = sscanf((STRPTR)"1,2,3", (STRPTR)"%ld,%ld,%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_three_count",
		(STRPTR)"sscanf with three %%ld fields",
		(STRPTR)"sscanf with three %%ld fields must assign 3",
		3, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_three_v1",
		(STRPTR)"First parsed long",
		(STRPTR)"First parsed long must be 1",
		1, v);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_three_v2",
		(STRPTR)"Second parsed long",
		(STRPTR)"Second parsed long must be 2",
		2, v2);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_three_v3",
		(STRPTR)"Third parsed long",
		(STRPTR)"Third parsed long must be 3",
		3, v3);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"abc", (STRPTR)"%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_no_match",
		(STRPTR)"sscanf %%ld on non-digits",
		(STRPTR)"sscanf %%ld on non-digits must assign 0 fields",
		0, n);

	v = 0;
	args[0] = (APTR)&v;
	n = sscanf((STRPTR)"x=5", (STRPTR)"x=%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_literal_match",
		(STRPTR)"sscanf",
		(STRPTR)"sscanf must match literal text before the field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"sscanf_literal_value",
		(STRPTR)"Value after literal \"x=\"",
		(STRPTR)"Value after literal \"x=\" must be 5",
		5, v);

	d = atof((STRPTR)"1.5");
	sprintf(buf, (STRPTR)"%.1f", d);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_f_1",
		(STRPTR)"sprintf %%.1f of atof(\"1.5\")",
		(STRPTR)"sprintf %%.1f of atof(\"1.5\") must be \"1.5\"",
		(STRPTR)"1.5",
		buf);

	d = atof((STRPTR)"-3.25");
	sprintf(buf, (STRPTR)"%.2f", d);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_f_2",
		(STRPTR)"sprintf %%.2f of atof(\"-3.25\")",
		(STRPTR)"sprintf %%.2f of atof(\"-3.25\") must be \"-3.25\"",
		(STRPTR)"-3.25",
		buf);

	d = atof((STRPTR)"1e3");
	sprintf(buf, (STRPTR)"%.0f", d);
	ct_check_string((STRPTR)"fmt",
		(STRPTR)"sprintf_f_0",
		(STRPTR)"sprintf %%.0f of atof(\"1e3\")",
		(STRPTR)"sprintf %%.0f of atof(\"1e3\") must be \"1000\"",
		(STRPTR)"1000",
		buf);

	d = atof((STRPTR)"12.5");
	sprintf(buf, (STRPTR)"%.2e", d);
	ct_check_true((STRPTR)"fmt", (STRPTR)"sprintf_e_nonempty",
		(STRPTR)"sprintf %%.2e",
		(STRPTR)"sprintf %%.2e must produce a non-empty scientific string",
		buf[0] != '\0');
	ct_check_true((STRPTR)"fmt", (STRPTR)"sprintf_e_has_e",
		(STRPTR)"sprintf %%.2e output",
		(STRPTR)"sprintf %%.2e output must contain 'e' or 'E'",
		strchr(buf, (LONG)'e') != (STRPTR)NULL ||
		strchr(buf, (LONG)'E') != (STRPTR)NULL);

	sprintf(buf, (STRPTR)"%ld", 98765);
	v = 0;
	args[0] = (APTR)&v;
	n = sscanf(buf, (STRPTR)"%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_ld_count",
		(STRPTR)"sscanf after sprintf %%ld",
		(STRPTR)"sscanf after sprintf %%ld must assign 1 field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_ld_value",
		(STRPTR)"Round-tripped long",
		(STRPTR)"Round-tripped long must equal 98765",
		98765, v);

	sprintf(buf, (STRPTR)"%d", -4321);
	v = 0;
	args[0] = (APTR)&v;
	n = sscanf(buf, (STRPTR)"%d", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_d_count",
		(STRPTR)"sscanf after sprintf %%d",
		(STRPTR)"sscanf after sprintf %%d must assign 1 field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_d_value",
		(STRPTR)"Round-tripped int",
		(STRPTR)"Round-tripped int must equal -4321",
		-4321, v);

	sprintf(buf, (STRPTR)"%u", 60000);
	uv = 0;
	args[0] = (APTR)&uv;
	n = sscanf(buf, (STRPTR)"%u", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_u_count",
		(STRPTR)"sscanf after sprintf %%u",
		(STRPTR)"sscanf after sprintf %%u must assign 1 field",
		1, n);
	ct_check_ulong((STRPTR)"fmt", (STRPTR)"roundtrip_u_value",
		(STRPTR)"Round-tripped unsigned",
		(STRPTR)"Round-tripped unsigned must equal 60000",
		(ULONG)60000, uv);

	sprintf(buf, (STRPTR)"%x", 4095);
	v = 0;
	args[0] = (APTR)&v;
	n = sscanf(buf, (STRPTR)"%x", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_x_count",
		(STRPTR)"sscanf after sprintf %%x",
		(STRPTR)"sscanf after sprintf %%x must assign 1 field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_x_value",
		(STRPTR)"Round-tripped hex",
		(STRPTR)"Round-tripped hex must equal 4095",
		4095, v);

	sprintf(buf, (STRPTR)"val=%ld", 1234);
	v = 0;
	args[0] = (APTR)&v;
	n = sscanf(buf, (STRPTR)"val=%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_literal_count",
		(STRPTR)"Round-trip with literal prefix",
		(STRPTR)"Round-trip with literal prefix must assign 1 field",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"roundtrip_literal_value",
		(STRPTR)"Round-trip literal value",
		(STRPTR)"Round-trip literal value must be 1234",
		1234, v);

	/* --- vsscanf --- */
	v = 0;
	args[0] = (APTR)&v;
	n = vsscanf((STRPTR)"77", (STRPTR)"%ld", (APTR)args);
	ct_check_long((STRPTR)"fmt", (STRPTR)"vsscanf_count",
		(STRPTR)"vsscanf(\"77\",\"%%ld\",args)",
		(STRPTR)"vsscanf must assign like sscanf via APTR args",
		1, n);
	ct_check_long((STRPTR)"fmt", (STRPTR)"vsscanf_value",
		(STRPTR)"vsscanf parsed value",
		(STRPTR)"vsscanf must store the scanned integer",
		77, v);
}
