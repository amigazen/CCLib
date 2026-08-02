/*
 * cc_locale.c - POSIX locale session (Wave 0 helpers + Wave 1 LVOs)
 *
 * Default is "C" (locale NULL).  setlocale soft-opens locale.library on the
 * first non-"C" request; failure keeps "C" and returns NULL.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/locale.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/locale.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* Soft-opened on first non-"C" setlocale; NULL until then. */
struct LocaleBase *LocaleBase;

/* Static "C" / active lconv storage for localeconv(). */
static char cc_lconv_dot[] = ".";
static char cc_lconv_empty[] = "";
static char cc_lconv_group[] = "";
static char cc_lconv_char_max = (char)127;

typedef struct
{
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
} cc_lconv;

static cc_lconv cc_lconv_buf;

static void
cc_lconv_set_C(void)
{
	cc_lconv_buf.decimal_point = cc_lconv_dot;
	cc_lconv_buf.thousands_sep = cc_lconv_empty;
	cc_lconv_buf.grouping = cc_lconv_group;
	cc_lconv_buf.int_curr_symbol = cc_lconv_empty;
	cc_lconv_buf.currency_symbol = cc_lconv_empty;
	cc_lconv_buf.mon_decimal_point = cc_lconv_empty;
	cc_lconv_buf.mon_thousands_sep = cc_lconv_empty;
	cc_lconv_buf.mon_grouping = cc_lconv_group;
	cc_lconv_buf.positive_sign = cc_lconv_empty;
	cc_lconv_buf.negative_sign = cc_lconv_empty;
	cc_lconv_buf.int_frac_digits = cc_lconv_char_max;
	cc_lconv_buf.frac_digits = cc_lconv_char_max;
	cc_lconv_buf.p_cs_precedes = cc_lconv_char_max;
	cc_lconv_buf.p_sep_by_space = cc_lconv_char_max;
	cc_lconv_buf.n_cs_precedes = cc_lconv_char_max;
	cc_lconv_buf.n_sep_by_space = cc_lconv_char_max;
	cc_lconv_buf.p_sign_posn = cc_lconv_char_max;
	cc_lconv_buf.n_sign_posn = cc_lconv_char_max;
}

static void
cc_lconv_from_locale(struct Locale *loc)
{
	if (loc == (struct Locale *)NULL)
	{
		cc_lconv_set_C();
		return;
	}

	cc_lconv_buf.decimal_point =
		loc->loc_DecimalPoint ? (char *)loc->loc_DecimalPoint : cc_lconv_dot;
	cc_lconv_buf.thousands_sep =
		loc->loc_GroupSeparator ? (char *)loc->loc_GroupSeparator : cc_lconv_empty;
	cc_lconv_buf.grouping =
		loc->loc_Grouping ? (char *)loc->loc_Grouping : cc_lconv_group;
	cc_lconv_buf.int_curr_symbol =
		loc->loc_MonIntCS ? (char *)loc->loc_MonIntCS : cc_lconv_empty;
	cc_lconv_buf.currency_symbol =
		loc->loc_MonCS ? (char *)loc->loc_MonCS : cc_lconv_empty;
	cc_lconv_buf.mon_decimal_point =
		loc->loc_MonDecimalPoint ? (char *)loc->loc_MonDecimalPoint : cc_lconv_empty;
	cc_lconv_buf.mon_thousands_sep =
		loc->loc_MonGroupSeparator ? (char *)loc->loc_MonGroupSeparator : cc_lconv_empty;
	cc_lconv_buf.mon_grouping =
		loc->loc_MonGrouping ? (char *)loc->loc_MonGrouping : cc_lconv_group;
	cc_lconv_buf.positive_sign =
		loc->loc_MonPositiveSign ? (char *)loc->loc_MonPositiveSign : cc_lconv_empty;
	cc_lconv_buf.negative_sign =
		loc->loc_MonNegativeSign ? (char *)loc->loc_MonNegativeSign : cc_lconv_empty;
	cc_lconv_buf.int_frac_digits = (char)loc->loc_MonIntFracDigits;
	cc_lconv_buf.frac_digits = (char)loc->loc_MonFracDigits;
	cc_lconv_buf.p_cs_precedes = (char)loc->loc_MonPositiveCSPos;
	cc_lconv_buf.p_sep_by_space = (char)loc->loc_MonPositiveSpaceSep;
	cc_lconv_buf.n_cs_precedes = (char)loc->loc_MonNegativeCSPos;
	cc_lconv_buf.n_sep_by_space = (char)loc->loc_MonNegativeSpaceSep;
	cc_lconv_buf.p_sign_posn = (char)loc->loc_MonPositiveSignPos;
	cc_lconv_buf.n_sign_posn = (char)loc->loc_MonNegativeSignPos;
}

static ULONG
cc_lc_bit(LONG category)
{
	switch (category)
	{
	case 0: /* LC_ALL */
		return CC_LC_ALL;
	case 1: /* LC_COLLATE */
		return CC_LC_COLLATE;
	case 2: /* LC_CTYPE */
		return CC_LC_CTYPE;
	case 3: /* LC_MONETARY */
		return CC_LC_MONETARY;
	case 4: /* LC_NUMERIC */
		return CC_LC_NUMERIC;
	case 5: /* LC_TIME */
		return CC_LC_TIME;
	case 6: /* LC_MESSAGES */
		return CC_LC_MESSAGES;
	default:
		return 0UL;
	}
}

static LONG
cc_locale_name_is_C(STRPTR name)
{
	if (name == (STRPTR)NULL)
		return 0;
	if (name[0] == 'C' && name[1] == '\0')
		return 1;
	if (name[0] == 'P' && name[1] == 'O' && name[2] == 'S'
	    && name[3] == 'I' && name[4] == 'X' && name[5] == '\0')
		return 1;
	return 0;
}

static LONG
cc_ensure_locale_lib(void)
{
	if (LocaleBase != (struct LocaleBase *)NULL)
		return 1;
	LocaleBase = (struct LocaleBase *)OpenLibrary(
		(STRPTR)"locale.library", 38L);
	return LocaleBase != (struct LocaleBase *)NULL;
}

void
cc_locale_init(cc_task_UserData *ud)
{
	if (ud == (cc_task_UserData *)NULL)
		return;
	ud->locale = (APTR)NULL;
	ud->lc_mask = 0UL;
	ud->locale_name[0] = 'C';
	ud->locale_name[1] = '\0';
	cc_lconv_set_C();
}

void
cc_locale_clear(cc_task_UserData *ud)
{
	struct Locale *loc;

	if (ud == (cc_task_UserData *)NULL)
		return;

	loc = (struct Locale *)ud->locale;
	if (loc != (struct Locale *)NULL
	    && LocaleBase != (struct LocaleBase *)NULL)
		CloseLocale(loc);

	ud->locale = (APTR)NULL;
	ud->lc_mask = 0UL;
	ud->locale_name[0] = 'C';
	ud->locale_name[1] = '\0';
	cc_lconv_set_C();
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
	    && LocaleBase != (struct LocaleBase *)NULL)
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
	    && LocaleBase != (struct LocaleBase *)NULL)
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
	    && LocaleBase != (struct LocaleBase *)NULL)
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
	    && LocaleBase != (struct LocaleBase *)NULL)
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

	if (ud != (cc_task_UserData *)NULL
	    && (ud->lc_mask & CC_LC_COLLATE) != 0UL
	    && ud->locale != (APTR)NULL
	    && LocaleBase != (struct LocaleBase *)NULL)
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

STRPTR __ASM__ __SAVE_DS__ setlocale(
	__REG__(d0, LONG category),
	__REG__(a0, STRPTR name))
{
	cc_task_UserData *ud;
	ULONG bits;
	struct Locale *loc;
	struct Locale *old;
	STRPTR open_name;
	LONG i;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return (STRPTR)NULL;

	bits = cc_lc_bit(category);
	if (bits == 0UL)
		return (STRPTR)NULL;

	/* Query: return last successful name. */
	if (name == (STRPTR)NULL)
		return (STRPTR)ud->locale_name;

	if (cc_locale_name_is_C(name))
	{
		old = (struct Locale *)ud->locale;
		if (old != (struct Locale *)NULL
		    && LocaleBase != (struct LocaleBase *)NULL)
			CloseLocale(old);
		ud->locale = (APTR)NULL;
		ud->lc_mask = 0UL;
		ud->locale_name[0] = 'C';
		ud->locale_name[1] = '\0';
		cc_lconv_set_C();
		return (STRPTR)ud->locale_name;
	}

	if (!cc_ensure_locale_lib())
		return (STRPTR)NULL;

	open_name = name;
	if (name[0] == '\0')
		open_name = (STRPTR)NULL;

	loc = OpenLocale(open_name);
	if (loc == (struct Locale *)NULL)
		return (STRPTR)NULL;

	old = (struct Locale *)ud->locale;
	if (old != (struct Locale *)NULL)
		CloseLocale(old);

	ud->locale = (APTR)loc;
	/*
	 * Amiga locales are all-or-nothing: installing any category shares
	 * one Locale *.  LC_ALL sets every bit; otherwise OR the requested.
	 */
	if (category == 0)
		ud->lc_mask = CC_LC_ALL;
	else
		ud->lc_mask |= bits;

	if (open_name == (STRPTR)NULL)
	{
		ud->locale_name[0] = '\0';
	}
	else
	{
		i = 0;
		while (i < 31 && name[i] != '\0')
		{
			ud->locale_name[i] = name[i];
			i++;
		}
		ud->locale_name[i] = '\0';
	}

	cc_lconv_from_locale(loc);
	return (STRPTR)ud->locale_name;
}

APTR __ASM__ __SAVE_DS__ localeconv(void)
{
	cc_task_UserData *ud;
	struct Locale *loc;
	cc_check_break();

	ud = cc_get_ud();
	if (ud != (cc_task_UserData *)NULL
	    && ud->locale != (APTR)NULL)
	{
		loc = (struct Locale *)ud->locale;
		cc_lconv_from_locale(loc);
	}
	else
	{
		cc_lconv_set_C();
	}
	return (APTR)&cc_lconv_buf;
}

LONG __ASM__ __SAVE_DS__ strcoll(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2))
{
	cc_check_break();
	return cc_strcoll(cc_get_ud(), s1, s2);
}

ULONG __ASM__ __SAVE_DS__ strxfrm(
	__REG__(a0, STRPTR dest),
	__REG__(a1, STRPTR src),
	__REG__(d0, ULONG n))
{
	ULONG len;
	ULONG i;

	/*

	 * Wave 3 may use StrConvert.  Until then "C" transform == copy;
	 * with a collate locale we still copy (byte identity is safe).
	 */
	cc_check_break();
	len = 0UL;
	if (src != (STRPTR)NULL)
	{
		while (src[len] != '\0')
			len++;
	}

	if (n == 0UL || dest == (STRPTR)NULL)
		return len;

	if (src == (STRPTR)NULL)
	{
		dest[0] = '\0';
		return 0UL;
	}

	i = 0UL;
	while (i + 1UL < n && src[i] != '\0')
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = '\0';
	return len;
}
