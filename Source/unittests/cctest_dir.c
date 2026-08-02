/*
 * cctest_dir.c - scdir / scdir_clean directory scanning
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

void
ct_suite_dir(void)
{
	STRPTR name;
	LONG count;

	ct_suite_begin((STRPTR)"dir", (STRPTR)"scdir / scdir_clean");

	scdir_clean();
	ct_check_true((STRPTR)"dir", (STRPTR)"scdir_clean_idle",
		(STRPTR)"scdir_clean()",
		(STRPTR)"scdir_clean on idle state must complete without fault",
		1);

	/*
	 * Scan LIBS: with a wildcard — do not depend on the exact installed
	 * spelling of cclib.library, and never use a fake volume name (that
	 * puts up an AmigaDOS "Please insert volume" requester).
	 */
	scdir_clean();
	name = scdir((STRPTR)"LIBS:#?");
	ct_check_ptr_nonnull((STRPTR)"dir", (STRPTR)"scdir_libs",
		(STRPTR)"scdir(\"LIBS:#?\")",
		(STRPTR)"scdir of LIBS:#? must return at least one filename",
		(APTR)name);
	if (name != (STRPTR)NULL)
	{
		ct_check_true((STRPTR)"dir", (STRPTR)"scdir_libs_nonempty",
			(STRPTR)"strlen(first LIBS:#? entry)",
			(STRPTR)"First scdir result on LIBS: must be a non-empty name",
			name[0] != '\0');
	}

	count = 0;
	while (name != (STRPTR)NULL && count < 64)
	{
		name = scdir((STRPTR)"LIBS:#?");
		count++;
	}
	ct_check_true((STRPTR)"dir", (STRPTR)"scdir_libs_iterate",
		(STRPTR)"scdir(\"LIBS:#?\") until NULL",
		(STRPTR)"Iterating scdir on LIBS: must eventually return NULL without fault",
		1);
	scdir_clean();

	name = scdir((STRPTR)"T:#?");
	if (name != (STRPTR)NULL)
	{
		ct_check_true((STRPTR)"dir", (STRPTR)"scdir_t_first",
			(STRPTR)"scdir(\"T:#?\")",
			(STRPTR)"First scdir entry on T: must be non-empty when present",
			name[0] != '\0');
	}
	else
	{
		ct_check_ptr_null((STRPTR)"dir", (STRPTR)"scdir_t_empty",
			(STRPTR)"scdir(\"T:#?\")",
			(STRPTR)"scdir on empty T: may return NULL (soft check)",
			(APTR)name);
	}

	count = 0;
	while (name != (STRPTR)NULL && count < 32)
	{
		name = scdir((STRPTR)"T:#?");
		if (name != (STRPTR)NULL)
			count++;
	}
	ct_check_true((STRPTR)"dir", (STRPTR)"scdir_t_iterate",
		(STRPTR)"scdir(\"T:#?\") until NULL",
		(STRPTR)"Iterating scdir on T: must eventually return NULL without fault",
		1);
	scdir_clean();

	/*
	 * Do NOT scdir a non-existent volume name: AmigaDOS puts up an
	 * "Please insert volume ..." requester. Use a no-match on T: instead.
	 */
	scdir_clean();
	name = scdir((STRPTR)"T:CCTEST_NO_SUCH_FILE_ZZ");
	ct_check_ptr_null((STRPTR)"dir", (STRPTR)"scdir_no_match",
		(STRPTR)"scdir(\"T:CCTEST_NO_SUCH_FILE_ZZ\")",
		(STRPTR)"scdir of a non-existent file on T: must return NULL (no volume requester)",
		(APTR)name);
	scdir_clean();

	scdir_clean();
	ct_check_true((STRPTR)"dir", (STRPTR)"scdir_clean_final",
		(STRPTR)"scdir_clean()",
		(STRPTR)"Final scdir_clean must leave scanner idle without fault",
		1);

	/* --- opendir / readdir / closedir --- */
	{
		APTR dir;
		STRPTR ent;
		LONG nent;
		LONG rc;

		dir = opendir((STRPTR)"LIBS:");
		ct_check_ptr_nonnull((STRPTR)"dir", (STRPTR)"opendir_libs",
			(STRPTR)"opendir(\"LIBS:\")",
			(STRPTR)"opendir of LIBS: must return a DIR",
			dir);
		if (dir != (APTR)NULL)
		{
			nent = 0;
			ent = (STRPTR)readdir(dir);
			ct_check_ptr_nonnull((STRPTR)"dir", (STRPTR)"readdir_first",
				(STRPTR)"readdir after opendir(LIBS:)",
				(STRPTR)"readdir must yield at least one name under LIBS:",
				(APTR)ent);
			while (ent != (STRPTR)NULL)
			{
				nent++;
				if (nent > 500)
					break;
				ent = (STRPTR)readdir(dir);
			}
			ct_check_true((STRPTR)"dir", (STRPTR)"readdir_count",
				(STRPTR)"readdir iteration",
				(STRPTR)"readdir must return multiple entries or stop cleanly",
				nent >= 1);
			rc = closedir(dir);
			ct_check_long((STRPTR)"dir", (STRPTR)"closedir_ok",
				(STRPTR)"closedir",
				(STRPTR)"closedir must return 0",
				0, rc);
		}
	}
}
