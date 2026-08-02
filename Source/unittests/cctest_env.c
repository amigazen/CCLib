/*
 * cctest_env.c - setenv / getenv
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_env(void)
{
	LONG rc;
	STRPTR v;

	ct_suite_begin((STRPTR)"env", (STRPTR)"setenv / getenv roundtrips");

	rc = setenv((STRPTR)"CCTEST_VAR", (STRPTR)"cclib_ok");
	ct_check_long((STRPTR)"env", (STRPTR)"setenv_ok",
		(STRPTR)"setenv(\"CCTEST_VAR\",\"cclib_ok\")",
		(STRPTR)"setenv(CCTEST_VAR, cclib_ok) must return 0 on success",
		0, rc);

	v = getenv((STRPTR)"CCTEST_VAR");
	ct_check_ptr_nonnull((STRPTR)"env", (STRPTR)"getenv_ptr",
		(STRPTR)"getenv(\"CCTEST_VAR\")",
		(STRPTR)"getenv(CCTEST_VAR) must return a non-NULL value pointer",
		(APTR)v);
	if (v != (STRPTR)NULL)
	{
		ct_check_string((STRPTR)"env",
		(STRPTR)"getenv_value",
		(STRPTR)"getenv(\"CCTEST_VAR\")",
		(STRPTR)"getenv must return the string previously set by setenv",
		(STRPTR)"cclib_ok",
		v);
	}

	v = getenv((STRPTR)"CCTEST_NO_SUCH_VAR_ZZ");
	ct_check_ptr_null((STRPTR)"env", (STRPTR)"getenv_missing",
		(STRPTR)"getenv(\"CCTEST_NO_SUCH_VAR_ZZ\")",
		(STRPTR)"getenv of an unset variable must return NULL",
		(APTR)v);

	rc = setenv((STRPTR)"CCTEST_VAR", (STRPTR)"changed");
	ct_check_long((STRPTR)"env", (STRPTR)"setenv_overwrite",
		(STRPTR)"setenv(\"CCTEST_VAR\",\"changed\")",
		(STRPTR)"setenv must allow overwriting an existing variable",
		0, rc);
	v = getenv((STRPTR)"CCTEST_VAR");
	ct_check_string((STRPTR)"env",
		(STRPTR)"getenv_changed",
		(STRPTR)"getenv(\"CCTEST_VAR\") after overwrite",
		(STRPTR)"getenv after overwrite must return the new value",
		(STRPTR)"changed",
		v);

	rc = setenv((STRPTR)"CCTEST_VAR2", (STRPTR)"second");
	ct_check_long((STRPTR)"env", (STRPTR)"setenv_second",
		(STRPTR)"setenv(\"CCTEST_VAR2\",\"second\")",
		(STRPTR)"setenv of a second variable must succeed",
		0, rc);
	v = getenv((STRPTR)"CCTEST_VAR2");
	ct_check_string((STRPTR)"env",
		(STRPTR)"getenv_second",
		(STRPTR)"getenv(\"CCTEST_VAR2\")",
		(STRPTR)"getenv must return the second variable independently",
		(STRPTR)"second",
		v);

	rc = setenv((STRPTR)"CCTEST_VAR3", (STRPTR)"third");
	ct_check_long((STRPTR)"env", (STRPTR)"setenv_third",
		(STRPTR)"setenv(\"CCTEST_VAR3\",\"third\")",
		(STRPTR)"setenv of a third variable must succeed",
		0, rc);
	v = getenv((STRPTR)"CCTEST_VAR");
	ct_check_string((STRPTR)"env",
		(STRPTR)"getenv_first_still",
		(STRPTR)"getenv(\"CCTEST_VAR\") with multiple vars set",
		(STRPTR)"First variable must remain readable after setting others",
		(STRPTR)"changed",
		v);

	rc = setenv((STRPTR)"CCTEST_EMPTY", (STRPTR)"");
	ct_check_true((STRPTR)"env", (STRPTR)"setenv_empty_delete",
		(STRPTR)"setenv(\"CCTEST_EMPTY\",\"\")",
		(STRPTR)"setenv with empty value deletes the variable (0 success, -1 if absent)",
		rc == 0L || rc == -1L);
	v = getenv((STRPTR)"CCTEST_EMPTY");
	ct_check_ptr_null((STRPTR)"env", (STRPTR)"getenv_empty_gone",
		(STRPTR)"getenv(\"CCTEST_EMPTY\") after delete",
		(STRPTR)"getenv after empty setenv must return NULL",
		(APTR)v);

	rc = setenv((STRPTR)"CCTEST_VAR", (STRPTR)"");
	ct_check_true((STRPTR)"env", (STRPTR)"setenv_clear_main",
		(STRPTR)"setenv(\"CCTEST_VAR\",\"\")",
		(STRPTR)"Clearing CCTEST_VAR via empty setenv must complete",
		rc == 0L || rc == -1L);
	v = getenv((STRPTR)"CCTEST_VAR");
	ct_check_ptr_null((STRPTR)"env", (STRPTR)"getenv_main_cleared",
		(STRPTR)"getenv(\"CCTEST_VAR\") after clear",
		(STRPTR)"getenv after clearing main test var must return NULL",
		(APTR)v);

	rc = setenv((STRPTR)"CCTEST_UNSET", (STRPTR)"bye");
	ct_check_long((STRPTR)"env", (STRPTR)"setenv_for_unsetenv",
		(STRPTR)"setenv(\"CCTEST_UNSET\",\"bye\")",
		(STRPTR)"setenv before unsetenv must succeed",
		0, rc);
	rc = unsetenv((STRPTR)"CCTEST_UNSET");
	ct_check_long((STRPTR)"env", (STRPTR)"unsetenv_ok",
		(STRPTR)"unsetenv(\"CCTEST_UNSET\")",
		(STRPTR)"unsetenv must delete the variable and return 0",
		0, rc);
	v = getenv((STRPTR)"CCTEST_UNSET");
	ct_check_ptr_null((STRPTR)"env", (STRPTR)"getenv_after_unsetenv",
		(STRPTR)"getenv after unsetenv",
		(STRPTR)"getenv after unsetenv must return NULL",
		(APTR)v);
}
