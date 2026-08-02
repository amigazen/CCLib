/*
 * cctest.c - unit-test driver (stdout only, verbose RESULT records)
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

extern FILE *stderr;

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	(void)stderr; /* harness never writes to stderr */

	ct_init();

	ct_suite_crt();
	ct_suite_string();
	ct_suite_mem();
	ct_suite_conv();
	ct_suite_ctype();
	ct_suite_heap();
	ct_suite_list();
	ct_suite_fmt();
	ct_suite_stdio();
	ct_suite_io();
	ct_suite_time();
	ct_suite_env();
	ct_suite_sort();
	ct_suite_dir();
	ct_suite_path();
	ct_suite_locale();

	ct_summary();

	if (ct_fail != 0)
		return 20;
	return 0;
}
