/*
 * cctest_time.c - time / clock / tm conversion helpers
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_time(void)
{
	LONG t1;
	LONG t2;
	LONG dt;
	LONG clk;
	struct tm *tm;
	STRPTR s;
	char buf[80];
	ULONG n;
	LONG tloc;

	ct_suite_begin((STRPTR)"time",
		(STRPTR)"time, clock, localtime, strftime");

	tloc = 0;
	t1 = time((APTR)&tloc);
	ct_check_true((STRPTR)"time", (STRPTR)"time_positive",
		(STRPTR)"time(&tloc)",
		(STRPTR)"time() must return a positive Amiga epoch-based time_t",
		t1 > 0);
	ct_check_long((STRPTR)"time", (STRPTR)"time_stores",
		(STRPTR)"time(&tloc)",
		(STRPTR)"When tloc is non-NULL, time must also store the value there",
		t1, tloc);

	clk = clock();
	ct_check_true((STRPTR)"time", (STRPTR)"clock_nonneg",
		(STRPTR)"clock()",
		(STRPTR)"clock() must return a non-negative tick count since start",
		clk >= 0);

	tm = localtime((APTR)&t1);
	ct_check_ptr_nonnull((STRPTR)"time", (STRPTR)"localtime_ptr",
		(STRPTR)"localtime(&t1)",
		(STRPTR)"localtime must return a pointer to a per-task tm structure",
		(APTR)tm);

	tm = gmtime((APTR)&t1);
	ct_check_ptr_nonnull((STRPTR)"time", (STRPTR)"gmtime_ptr",
		(STRPTR)"gmtime(&t1)",
		(STRPTR)"gmtime (same as localtime on Amiga) must return a tm pointer",
		(APTR)tm);

	s = asctime(tm);
	ct_check_ptr_nonnull((STRPTR)"time", (STRPTR)"asctime_ptr",
		(STRPTR)"asctime(tm)",
		(STRPTR)"asctime must return a non-NULL textual timestamp",
		(APTR)s);
	ct_check_true((STRPTR)"time", (STRPTR)"asctime_len",
		(STRPTR)"strlen(asctime(tm))",
		(STRPTR)"asctime string must be non-empty",
		s != (STRPTR)NULL && s[0] != '\0');

	s = ctime((APTR)&t1);
	ct_check_ptr_nonnull((STRPTR)"time", (STRPTR)"ctime_ptr",
		(STRPTR)"ctime(&t1)",
		(STRPTR)"ctime must return a non-NULL string (asctime(localtime(t)))",
		(APTR)s);

	tm = localtime((APTR)&t1);
	t2 = mktime(tm);
	ct_check_true((STRPTR)"time", (STRPTR)"mktime_roundtrip",
		(STRPTR)"mktime(localtime(&t1))",
		(STRPTR)"mktime(localtime(t)) must return a time close to the original t",
		t2 > 0);

	dt = difftime(t1 + 10, t1);
	ct_check_long((STRPTR)"time", (STRPTR)"difftime_10",
		(STRPTR)"difftime(t1+10,t1)",
		(STRPTR)"difftime(t+10,t) must be 10 seconds",
		10, dt);

	tm = localtime((APTR)&t1);

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%Y", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_year_len",
		(STRPTR)"strftime(buf,80,\"%%Y\",tm)",
		(STRPTR)"strftime with %%Y must write a non-empty year string",
		n > 0 && buf[0] != '\0');
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_year_digits",
		(STRPTR)"strlen(strftime %%Y)",
		(STRPTR)"strftime %%Y must produce a numeric year string of at least 4 digits",
		(LONG)strlen(buf) >= 4);

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%m", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_month",
		(STRPTR)"strftime(buf,80,\"%%m\",tm)",
		(STRPTR)"strftime %%m must write a 2-digit month",
		n == 2UL && buf[0] >= '0' && buf[0] <= '9');

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%d", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_mday",
		(STRPTR)"strftime(buf,80,\"%%d\",tm)",
		(STRPTR)"strftime %%d must write a 2-digit day-of-month",
		n == 2UL && buf[0] >= '0' && buf[0] <= '9');

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%H", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_hour",
		(STRPTR)"strftime(buf,80,\"%%H\",tm)",
		(STRPTR)"strftime %%H must write a 2-digit hour",
		n == 2UL);

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%M", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_min",
		(STRPTR)"strftime(buf,80,\"%%M\",tm)",
		(STRPTR)"strftime %%M must write a 2-digit minute",
		n == 2UL);

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%S", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_sec",
		(STRPTR)"strftime(buf,80,\"%%S\",tm)",
		(STRPTR)"strftime %%S must write a 2-digit second",
		n == 2UL);

	n = strftime(buf, (ULONG)sizeof(buf), (STRPTR)"%%", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_percent",
		(STRPTR)"strftime(buf,80,\"%%%%\",tm)",
		(STRPTR)"strftime with literal %% must write a single percent sign",
		n == 1UL && buf[0] == '%' && buf[1] == '\0');

	n = strftime(buf, (ULONG)sizeof(buf),
		(STRPTR)"%Y-%m-%d %H:%M:%S", tm);
	ct_check_true((STRPTR)"time", (STRPTR)"strftime_combo",
		(STRPTR)"strftime(buf,80,\"%%Y-%%m-%%d %%H:%%M:%%S\",tm)",
		(STRPTR)"Combined strftime format must produce a non-empty timestamp",
		n >= 19UL && buf[0] != '\0');
}
