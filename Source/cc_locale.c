/*
 * cc_locale.c - POSIX locale session seams (Wave 0)
 *
 * Holds per-task locale pointer and character helpers used by string/conv
 * LVOs.  setlocale / OpenLocale arrive in Wave 1; until then locale stays
 * NULL ("C") and helpers use utility.library or ASCII folds.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/locale.h>

#include "private/cc_internal.h"

/* Soft-opened on first non-"C" setlocale (Wave 1); NULL today. */
struct LocaleBase *LocaleBase;

void
cc_locale_init(cc_task_UserData *ud)
{
	if (ud == (cc_task_UserData *)NULL)
		return;
	ud->locale = (APTR)NULL;
	ud->lc_mask = 0UL;
	ud->locale_name[0] = 'C';
	ud->locale_name[1] = '\0';
}

void
cc_locale_clear(cc_task_UserData *ud)
{
	struct Locale *loc;

	if (ud == (cc_task_UserData *)NULL)
		return;

	loc = (struct Locale *)ud->locale;
	if (loc != (struct Locale *)NULL
	    && LocaleBase != (struct Library *)NULL)
		CloseLocale(loc);

	ud->locale = (APTR)NULL;
	ud->lc_mask = 0UL;
	ud->locale_name[0] = 'C';
	ud->locale_name[1] = '\0';
}

LONG
cc_toupper_ch(cc_task_UserData *ud, LONG c)
{
	ULONG u;
	struct Locale *loc;

	u = (ULONG)c & 0xFFUL;

	if (ud != (cc_task_UserData *)NULL
	    && (ud->lc_mask & CC_LC_CTYPE) != 0UL
	    && ud->locale != (APTR)NULL
	    && LocaleBase != (struct Library *)NULL)
	{
		loc = (struct Locale *)ud->locale;
		return (LONG)ConvToUpper(loc, u);
	}

	if (UtilityBase != (struct Library *)NULL)
		return (LONG)ToUpper(u);

	if (c >= 'a' && c <= 'z')
		return c - ('a' - 'A');
	return c;
}

LONG
cc_tolower_ch(cc_task_UserData *ud, LONG c)
{
	ULONG u;
	struct Locale *loc;

	u = (ULONG)c & 0xFFUL;

	if (ud != (cc_task_UserData *)NULL
	    && (ud->lc_mask & CC_LC_CTYPE) != 0UL
	    && ud->locale != (APTR)NULL
	    && LocaleBase != (struct Library *)NULL)
	{
		loc = (struct Locale *)ud->locale;
		return (LONG)ConvToLower(loc, u);
	}

	if (UtilityBase != (struct Library *)NULL)
		return (LONG)ToLower(u);

	if (c >= 'A' && c <= 'Z')
		return c + ('a' - 'A');
	return c;
}

LONG
cc_str_casecmp(cc_task_UserData *ud, STRPTR s1, STRPTR s2)
{
	UBYTE *p1;
	UBYTE *p2;
	LONG c1;
	LONG c2;
	LONG use_loc;

	use_loc = 0;
	if (ud != (cc_task_UserData *)NULL
	    && (ud->lc_mask & CC_LC_CTYPE) != 0UL
	    && ud->locale != (APTR)NULL
	    && LocaleBase != (struct Library *)NULL)
		use_loc = 1;

	/* "C" path: utility Stricmp when available (historic Direct map). */
	if (!use_loc && UtilityBase != (struct Library *)NULL)
		return (LONG)Stricmp(s1, s2);

	p1 = (UBYTE *)s1;
	p2 = (UBYTE *)s2;
	if (p1 == p2)
		return 0L;
	if (p1 == (UBYTE *)NULL)
		return -1L;
	if (p2 == (UBYTE *)NULL)
		return 1L;

	for (;;)
	{
		c1 = cc_tolower_ch(ud, (LONG)*p1);
		c2 = cc_tolower_ch(ud, (LONG)*p2);
		if (c1 != c2)
			return c1 - c2;
		if (c1 == 0L)
			return 0L;
		p1++;
		p2++;
	}
}

LONG
cc_str_ncasecmp(cc_task_UserData *ud, STRPTR s1, STRPTR s2, LONG n)
{
	UBYTE *p1;
	UBYTE *p2;
	LONG c1;
	LONG c2;
	LONG i;
	LONG use_loc;

	if (n <= 0L)
		return 0L;

	use_loc = 0;
	if (ud != (cc_task_UserData *)NULL
	    && (ud->lc_mask & CC_LC_CTYPE) != 0UL
	    && ud->locale != (APTR)NULL
	    && LocaleBase != (struct Library *)NULL)
		use_loc = 1;

	if (!use_loc && UtilityBase != (struct Library *)NULL)
		return (LONG)Strnicmp(s1, s2, n);

	p1 = (UBYTE *)s1;
	p2 = (UBYTE *)s2;
	if (p1 == p2)
		return 0L;
	if (p1 == (UBYTE *)NULL)
		return -1L;
	if (p2 == (UBYTE *)NULL)
		return 1L;

	i = 0L;
	while (i < n)
	{
		c1 = cc_tolower_ch(ud, (LONG)p1[i]);
		c2 = cc_tolower_ch(ud, (LONG)p2[i]);
		if (c1 != c2)
			return c1 - c2;
		if (c1 == 0L)
			return 0L;
		i++;
	}
	return 0L;
}

LONG
cc_strcoll(cc_task_UserData *ud, STRPTR s1, STRPTR s2)
{
	struct Locale *loc;

	/*
	 * Wave 3: StrnCmp(locale, s1, s2, -1, SC_COLLATE2).
	 * Until then, collation == strcmp for every session.
	 */
	if (ud != (cc_task_UserData *)NULL
	    && (ud->lc_mask & CC_LC_COLLATE) != 0UL
	    && ud->locale != (APTR)NULL
	    && LocaleBase != (struct Library *)NULL)
	{
		loc = (struct Locale *)ud->locale;
		return StrnCmp(loc, s1, s2, -1L, SC_COLLATE2);
	}

	if (s1 == s2)
		return 0L;
	if (s1 == (STRPTR)NULL)
		return -1L;
	if (s2 == (STRPTR)NULL)
		return 1L;

	while (*s1 != '\0' && *s1 == *s2)
	{
		s1++;
		s2++;
	}
	return (LONG)(UBYTE)*s1 - (LONG)(UBYTE)*s2;
}
