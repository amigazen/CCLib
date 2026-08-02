/*
 * cctest_util.c - verbose unit-test harness (all output to stdout)
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

extern FILE *stdout;

LONG ct_pass;
LONG ct_fail;
LONG ct_total;

LONG ct_panic_hits;

/*
 * __saveds: library LVOs run with the library A4; without this, writes to
 * ct_panic_hits would hit the wrong near-data section and forged-handle
 * tests would see hits stuck at 0.  __stdargs matches abort_func calls
 * (same convention as CRT exit).
 */
void __stdargs __saveds
ct_panic_hook(LONG code)
{
	(void)code;
	ct_panic_hits++;
}

void
ct_install_panic_hook(void)
{
	ct_panic_hits = 0;
	/* Replace CRT exit hook so forged-handle panics do not kill CCTest. */
	SetAbortFunc((APTR)ct_panic_hook);
}

void
ct_restore_panic_hook(void)
{
	extern void __stdargs __saveds exit(LONG code);

	/* Restore the CRT terminate path used by SetupSTDIO. */
	SetAbortFunc((APTR)exit);
}

static void
ct_out(STRPTR fmt, APTR args)
{
	LONG dummy[1];

	if (stdout == (FILE *)NULL)
		return;
	if (args == (APTR)NULL)
	{
		dummy[0] = 0;
		args = (APTR)dummy;
	}
	fprintf(stdout, fmt, args);
	fflush(stdout);
}

void
ct_check(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	STRPTR expected, STRPTR actual, LONG pass)
{
	LONG av[6];
	STRPTR e;
	STRPTR a;
	STRPTR c;
	STRPTR d;

	ct_total++;
	if (pass)
		ct_pass++;
	else
		ct_fail++;

	if (stdout == (FILE *)NULL)
		return;

	e = expected;
	a = actual;
	c = ctcall;
	d = desc;
	if (e == (STRPTR)NULL)
		e = (STRPTR)"(no expected text)";
	if (a == (STRPTR)NULL)
		a = (STRPTR)"(no actual text)";
	if (c == (STRPTR)NULL)
		c = (STRPTR)"(call not stated)";
	if (d == (STRPTR)NULL)
		d = (STRPTR)"(no description)";

	av[0] = ct_total;
	ct_out((STRPTR)"--------\n", (APTR)NULL);
	ct_out((STRPTR)"TEST #%ld\n", (APTR)av);

	av[0] = (LONG)group;
	ct_out((STRPTR)"GROUP: %s\n", (APTR)av);

	av[0] = (LONG)name;
	ct_out((STRPTR)"NAME: %s\n", (APTR)av);

	av[0] = (LONG)c;
	ct_out((STRPTR)"CALL: %s\n", (APTR)av);

	av[0] = (LONG)d;
	ct_out((STRPTR)"DESC: %s\n", (APTR)av);

	av[0] = (LONG)e;
	ct_out((STRPTR)"EXPECTED: %s\n", (APTR)av);

	av[0] = (LONG)a;
	ct_out((STRPTR)"ACTUAL: %s\n", (APTR)av);

	av[0] = (LONG)(pass ? (STRPTR)"PASS" : (STRPTR)"FAIL");
	ct_out((STRPTR)"STATUS: %s\n", (APTR)av);

	ct_out((STRPTR)"--------\n", (APTR)NULL);
}

void
ct_init_named(STRPTR harness_name)
{
	LONG av[1];
	STRPTR name;

	ct_pass = 0;
	ct_fail = 0;
	ct_total = 0;

	if (stdout == (FILE *)NULL)
		return;
	name = harness_name;
	if (name == (STRPTR)NULL)
		name = (STRPTR)"CCTest";
	av[0] = (LONG)name;
	ct_out((STRPTR)"# %s - cclib.library unit tests\n", (APTR)av);
	ct_out((STRPTR)"# All harness output is on stdout (never stderr).\n",
		(APTR)NULL);
	ct_out((STRPTR)"# Each test block lists CALL, DESC, EXPECTED, ACTUAL, STATUS.\n",
		(APTR)NULL);
	av[0] = (LONG)CCLibBase;
	ct_out((STRPTR)"# CCLibBase=%ld\n", (APTR)av);
}

void
ct_init(void)
{
	ct_init_named((STRPTR)"CCTest");
}

void
ct_suite_begin(STRPTR group, STRPTR title)
{
	LONG av[2];

	if (stdout == (FILE *)NULL)
		return;
	av[0] = (LONG)group;
	av[1] = (LONG)title;
	ct_out((STRPTR)"\n# === SUITE %s - %s ===\n", (APTR)av);
}

void
ct_summary(void)
{
	LONG av[3];

	if (stdout == (FILE *)NULL)
		return;
	av[0] = ct_pass;
	av[1] = ct_fail;
	av[2] = ct_total;
	ct_out((STRPTR)"\nSUMMARY passed=%ld failed=%ld total=%ld\n", (APTR)av);
}

void
ct_check_long(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	LONG expected, LONG actual)
{
	char ebuf[96];
	char abuf[96];
	LONG av[1];

	av[0] = expected;
	sprintf(ebuf, (STRPTR)"signed long integer %ld", (APTR)av);
	av[0] = actual;
	sprintf(abuf, (STRPTR)"signed long integer %ld", (APTR)av);
	ct_check(group, name, ctcall, desc, ebuf, abuf,
		(expected == actual) ? 1L : 0L);
}

void
ct_check_ulong(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	ULONG expected, ULONG actual)
{
	char ebuf[96];
	char abuf[96];
	LONG av[1];

	av[0] = (LONG)expected;
	sprintf(ebuf, (STRPTR)"unsigned long integer %lu", (APTR)av);
	av[0] = (LONG)actual;
	sprintf(abuf, (STRPTR)"unsigned long integer %lu", (APTR)av);
	ct_check(group, name, ctcall, desc, ebuf, abuf,
		(expected == actual) ? 1L : 0L);
}

void
ct_check_string(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	STRPTR expected, STRPTR actual)
{
	char ebuf[160];
	char abuf[160];
	LONG av[1];
	STRPTR e;
	STRPTR a;
	LONG pass;

	e = expected;
	a = actual;
	if (e == (STRPTR)NULL)
		e = (STRPTR)"(null)";
	if (a == (STRPTR)NULL)
		a = (STRPTR)"(null)";

	av[0] = (LONG)e;
	sprintf(ebuf, (STRPTR)"NUL-terminated string \"%s\"", (APTR)av);
	av[0] = (LONG)a;
	sprintf(abuf, (STRPTR)"NUL-terminated string \"%s\"", (APTR)av);

	pass = 0;
	if (expected != (STRPTR)NULL && actual != (STRPTR)NULL
	    && strcmp(expected, actual) == 0)
		pass = 1;
	else if (expected == (STRPTR)NULL && actual == (STRPTR)NULL)
		pass = 1;

	ct_check(group, name, ctcall, desc, ebuf, abuf, pass);
}

void
ct_check_true(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	LONG cond)
{
	ct_check(group, name, ctcall, desc,
		(STRPTR)"condition is TRUE (non-zero / predicate holds)",
		cond
			? (STRPTR)"condition is TRUE (predicate holds)"
			: (STRPTR)"condition is FALSE (predicate failed)",
		cond ? 1L : 0L);
}

void
ct_check_false(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	LONG cond)
{
	ct_check(group, name, ctcall, desc,
		(STRPTR)"condition is FALSE (zero / predicate does not hold)",
		cond
			? (STRPTR)"condition is TRUE (unexpected - predicate held)"
			: (STRPTR)"condition is FALSE (as required)",
		cond ? 0L : 1L);
}

void
ct_check_ptr_nonnull(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	APTR p)
{
	char abuf[48];
	LONG av[1];

	if (p == (APTR)NULL)
	{
		ct_check(group, name, ctcall, desc,
			(STRPTR)"a non-NULL pointer (allocation or lookup succeeded)",
			(STRPTR)"NULL pointer (call failed or found nothing)",
			0L);
	}
	else
	{
		av[0] = (LONG)p;
		sprintf(abuf, (STRPTR)"non-NULL pointer 0x%08lx", (APTR)av);
		ct_check(group, name, ctcall, desc,
			(STRPTR)"a non-NULL pointer (allocation or lookup succeeded)",
			abuf, 1L);
	}
}

void
ct_check_ptr_null(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	APTR p)
{
	char abuf[48];
	LONG av[1];

	if (p == (APTR)NULL)
	{
		ct_check(group, name, ctcall, desc,
			(STRPTR)"NULL pointer (not found / empty result)",
			(STRPTR)"NULL pointer",
			1L);
	}
	else
	{
		av[0] = (LONG)p;
		sprintf(abuf,
			(STRPTR)"non-NULL pointer 0x%08lx (expected NULL)",
			(APTR)av);
		ct_check(group, name, ctcall, desc,
			(STRPTR)"NULL pointer (not found / empty result)",
			abuf, 0L);
	}
}

void
ct_check_ptr_eq(STRPTR group, STRPTR name, STRPTR ctcall, STRPTR desc,
	APTR expected, APTR actual)
{
	char ebuf[48];
	char abuf[48];
	LONG av[1];

	av[0] = (LONG)expected;
	sprintf(ebuf, (STRPTR)"pointer value 0x%08lx", (APTR)av);
	av[0] = (LONG)actual;
	sprintf(abuf, (STRPTR)"pointer value 0x%08lx", (APTR)av);
	ct_check(group, name, ctcall, desc, ebuf, abuf,
		(expected == actual) ? 1L : 0L);
}
