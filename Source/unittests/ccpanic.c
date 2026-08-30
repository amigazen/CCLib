/*
 * ccpanic.c - driver for CCPanic (validation / abort_func harness)
 *
 * Functional coverage stays in CCTest.  This binary exercises forged
 * handles, sentinel fds, double-close/free, and errno without mixing
 * those adversarial cases into the happy-path suite.
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
	(void)stderr;

	ct_init_named((STRPTR)"CCPanic");

	ct_suite_panic_heap();
	ct_suite_panic_stdio();
	ct_suite_panic_io();
	ct_suite_panic_dir();

	ct_summary();

	if (ct_fail != 0)
		return 20;
	return 0;
}
