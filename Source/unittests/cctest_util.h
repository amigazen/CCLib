/*
 * cctest_util.h - verbose unit-test harness (stdout only)
 *
 * Each check prints a multi-line record that is human-readable and
 * machine-parseable (one blank-line-separated block per test):
 *
 * --------
 * TEST #<n>
 * GROUP: <suite name>
 * NAME: <short stable id>
 * CALL: <exact call / scenario under test>
 * DESC: <why this matters / what contract is verified>
 * EXPECTED: <full explanation of the required outcome, including value>
 * ACTUAL: <full explanation of what CClib produced, including value>
 * STATUS: PASS|FAIL
 * --------
 *
 * Ends with: SUMMARY passed=.. failed=.. total=..
 */

#ifndef CCTEST_UTIL_H
#define CCTEST_UTIL_H

#include <exec/types.h>
#include <libraries/cclib.h>

extern FILE *stdout;

extern LONG ct_pass;
extern LONG ct_fail;
extern LONG ct_total;

/* Panic-hook bookkeeping for forged-handle tests (abort_func returns). */
extern LONG ct_panic_hits;
void __stdargs __saveds ct_panic_hook(LONG code);
void ct_install_panic_hook(void);
void ct_restore_panic_hook(void);

void ct_init(void);
void ct_init_named(STRPTR harness_name);
void ct_summary(void);
void ct_suite_begin(STRPTR group, STRPTR title);

/*
 * call  = exact expression / scenario, e.g. strlen("abcd")
 * desc  = contract being verified in plain English
 * exp_s / act_s for free-form; typed helpers build those strings.
 */
void ct_check(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	STRPTR expected, STRPTR actual, LONG pass);

void ct_check_long(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	LONG expected, LONG actual);
void ct_check_ulong(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	ULONG expected, ULONG actual);
void ct_check_string(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	STRPTR expected, STRPTR actual);
void ct_check_true(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	LONG cond);
void ct_check_false(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	LONG cond);
void ct_check_ptr_nonnull(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	APTR p);
void ct_check_ptr_null(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	APTR p);
void ct_check_ptr_eq(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	APTR expected, APTR actual);

/* Suite entry points — CCTest (functional) */
void ct_suite_crt(void);
void ct_suite_string(void);
void ct_suite_mem(void);
void ct_suite_conv(void);
void ct_suite_ctype(void);
void ct_suite_heap(void);
void ct_suite_list(void);
void ct_suite_fmt(void);
void ct_suite_stdio(void);
void ct_suite_io(void);
void ct_suite_time(void);
void ct_suite_env(void);
void ct_suite_sort(void);
void ct_suite_dir(void);
void ct_suite_path(void);
void ct_suite_locale(void);

/* Suite entry points — CCPanic (validation / abort_func) */
void ct_suite_panic_heap(void);
void ct_suite_panic_stdio(void);
void ct_suite_panic_io(void);

#endif /* CCTEST_UTIL_H */
