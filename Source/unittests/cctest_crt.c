/*
 * cctest_crt.c - CRT / support surface after SetupSTDIO
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
extern struct Library *CCLibBase;

static void ct_abort_smoke(void)
{
}

void
ct_suite_crt(void)
{
	APTR ud;
	APTR ud2;
	STRPTR s;

	ct_suite_begin((STRPTR)"crt", (STRPTR)"CRT, stdio, strerror, abort hooks");

	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"CCLibBase_open",
		(STRPTR)"CCLibBase",
		(STRPTR)"CRT must OpenLibrary(CClib.library) and leave CCLibBase set",
		(APTR)CCLibBase);

	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"stdin_attached",
		(STRPTR)"stdin",
		(STRPTR)"SetupSTDIO must attach a FILE* for standard input",
		(APTR)stdin);

	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"stdout_attached",
		(STRPTR)"stdout",
		(STRPTR)"SetupSTDIO must attach a FILE* for standard output (harness writes here)",
		(APTR)stdout);

	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"stderr_attached",
		(STRPTR)"stderr",
		(STRPTR)"SetupSTDIO must attach a FILE* for standard error (harness must not use it)",
		(APTR)stderr);

	ud = GetSTDIO();
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"GetSTDIO",
		(STRPTR)"GetSTDIO()",
		(STRPTR)"GetSTDIO must return the per-task task_UserData block",
		ud);

	s = strerror(0);
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"strerror_0_ptr",
		(STRPTR)"strerror(0)",
		(STRPTR)"strerror(0) must return a non-NULL message string",
		(APTR)s);
	ct_check_string((STRPTR)"crt",
		(STRPTR)"strerror_0_msg",
		(STRPTR)"strerror(0)",
		(STRPTR)"strerror(0) must map to the historic No Error string",
		(STRPTR)"No Error",
		s);

	s = strerror(1);
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"strerror_1_ptr",
		(STRPTR)"strerror(1)",
		(STRPTR)"strerror(1) must return a non-NULL message for CC_ENOENT",
		(APTR)s);
	ct_check_true((STRPTR)"crt", (STRPTR)"strerror_1_nonempty",
		(STRPTR)"strlen(strerror(1)) > 0",
		(STRPTR)"strerror(1) message must be non-empty",
		s != (STRPTR)NULL && s[0] != '\0');

	s = strerror(3);
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"strerror_3_ptr",
		(STRPTR)"strerror(3)",
		(STRPTR)"strerror(3) must return a non-NULL message for CC_ENOMEM",
		(APTR)s);
	ct_check_string((STRPTR)"crt",
		(STRPTR)"strerror_3_msg",
		(STRPTR)"strerror(3)",
		(STRPTR)"strerror(3) must map to the historic Not Enough Memory string",
		(STRPTR)"Not Enough Memory",
		s);

	s = strerror(5);
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"strerror_5_ptr",
		(STRPTR)"strerror(5)",
		(STRPTR)"strerror(5) must return a non-NULL message for CC_EINVAL",
		(APTR)s);
	ct_check_true((STRPTR)"crt", (STRPTR)"strerror_5_nonempty",
		(STRPTR)"strlen(strerror(5)) > 0",
		(STRPTR)"strerror(5) message must be non-empty",
		s != (STRPTR)NULL && s[0] != '\0');

	s = strerror(10);
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"strerror_10_ptr",
		(STRPTR)"strerror(10)",
		(STRPTR)"strerror(10) must return a non-NULL message for CC_EDOM",
		(APTR)s);
	ct_check_string((STRPTR)"crt",
		(STRPTR)"strerror_10_msg",
		(STRPTR)"strerror(10)",
		(STRPTR)"strerror(10) must map to the historic Domain Error string",
		(STRPTR)"Domain Error",
		s);

	s = strerror(999);
	ct_check_ptr_nonnull((STRPTR)"crt", (STRPTR)"strerror_unknown_ptr",
		(STRPTR)"strerror(999)",
		(STRPTR)"strerror of an out-of-range code must still return a fallback string",
		(APTR)s);
	ct_check_string((STRPTR)"crt",
		(STRPTR)"strerror_unknown_msg",
		(STRPTR)"strerror(999)",
		(STRPTR)"Out-of-range errno must map to Unknown Error",
		(STRPTR)"Unknown Error",
		s);

	SetAbortFunc((APTR)NULL);
	ct_check_true((STRPTR)"crt", (STRPTR)"SetAbortFunc_null",
		(STRPTR)"SetAbortFunc(NULL)",
		(STRPTR)"SetAbortFunc(NULL) must complete without fault",
		1);

	SetAbortFunc((APTR)ct_abort_smoke);
	ct_check_true((STRPTR)"crt", (STRPTR)"SetAbortFunc_hook",
		(STRPTR)"SetAbortFunc(ct_abort_smoke)",
		(STRPTR)"SetAbortFunc with a tiny static hook must complete without fault",
		1);

	ClearAbortFunc();
	ct_check_true((STRPTR)"crt", (STRPTR)"ClearAbortFunc",
		(STRPTR)"ClearAbortFunc()",
		(STRPTR)"ClearAbortFunc must clear the abort hook without fault",
		1);

	ud2 = GetSTDIO();
	ct_check_ptr_eq((STRPTR)"crt", (STRPTR)"GetSTDIO_stable",
		(STRPTR)"GetSTDIO()",
		(STRPTR)"GetSTDIO must return the same task_UserData block across calls",
		ud, ud2);
}
