/*
 * cc_locale.c - POSIX locale session (Wave 0-1 LVOs + Wave 2-3 FormatDate)
 *
 * Default is "C" (locale NULL).  setlocale soft-opens locale.library on the
 * first non-"C" request; failure keeps "C" and returns NULL.
 * Non-"C" LC_TIME strftime uses locale.library/FormatDate.
 */

#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <utility/date.h>
#include <utility/hooks.h>
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

/* ---- Wave 5-6: ctype via table or locale.library Is* ---------------- */

/* Same bit masks as ctype.h / cc_ctype_tab. */
#define CC_CT_U 0x01
#define CC_CT_L 0x02
#define CC_CT_D 0x04
#define CC_CT_X 0x08
#define CC_CT_S 0x10
#define CC_CT_C 0x20
#define CC_CT_P 0x40
#define CC_CT_ALNUM (CC_CT_U | CC_CT_L | CC_CT_D)
#define CC_CT_ALPHA (CC_CT_U | CC_CT_L)
/* Historic ctype.h masks: print includes high bit used for space. */
#define CC_CT_PRINT 0xc7
#define CC_CT_GRAPH 0x47

static LONG
cc_ctype_use_locale(cc_task_UserData *ud)
{
	if (ud == (cc_task_UserData *)NULL)
		return 0L;
	if ((ud->lc_mask & CC_LC_CTYPE) == 0UL)
		return 0L;
	if (ud->locale == (APTR)NULL)
		return 0L;
	if (LocaleBase == (struct LocaleBase *)NULL)
		return 0L;
	return 1L;
}

static LONG
cc_ctype_tab_test(LONG c, UBYTE mask)
{
	if (c < 0L || c > 255L)
		return 0L;
	return (cc_ctype_tab[(ULONG)c + 1UL] & mask) ? 1L : 0L;
}

LONG
cc_isalnum_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsAlNum((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_ALNUM);
}

LONG
cc_isalpha_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsAlpha((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_ALPHA);
}

LONG
cc_iscntrl_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsCntrl((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_C);
}

LONG
cc_isdigit_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsDigit((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_D);
}

LONG
cc_isgraph_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsGraph((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_GRAPH);
}

LONG
cc_islower_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsLower((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_L);
}

LONG
cc_isprint_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsPrint((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_PRINT);
}

LONG
cc_ispunct_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsPunct((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_P);
}

LONG
cc_isspace_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsSpace((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_S);
}

LONG
cc_isupper_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsUpper((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_U);
}

LONG
cc_isxdigit_ch(cc_task_UserData *ud, LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	if (cc_ctype_use_locale(ud))
		return IsXDigit((struct Locale *)ud->locale, (ULONG)c) ? 1L : 0L;
	return cc_ctype_tab_test(c, (UBYTE)CC_CT_X);
}

LONG
cc_isblank_ch(cc_task_UserData *ud, LONG c)
{
	/* No IsBlank in locale.library - space/tab only (C99). */
	(void)ud;
	if (c < 0L || c > 255L)
		return 0L;
	if (c == (LONG)' ' || c == (LONG)'\t')
		return 1L;
	return 0L;
}

LONG
cc_isascii_ch(LONG c)
{
	if (c < 0L || c > 255L)
		return 0L;
	return (c & 0x80L) ? 0L : 1L;
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
	if (!cc_str_mapped(name))
		return (STRPTR)NULL;

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
	if (!cc_str_arg(s1) || !cc_str_arg(s2))
		return 0L;
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
	if (!cc_str_arg(src) || (n > 0UL && dest != (STRPTR)NULL
	    && !cc_buf_mapped((APTR)dest, n)))
		return 0UL;
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

/* ---- Wave 2-3: FormatDate-backed strftime --------------------------- */

typedef struct
{
	STRPTR buf;
	ULONG maxsize;
	ULONG pos;
	LONG overflow;
} cc_sf_hook_data;

/*
 * FormatDate putCharFunc: A1 holds the character value (not a pointer),
 * including a final NUL.  Keep collecting until NUL even after overflow
 * so the device/library finishes the request cleanly.
 */
static ULONG __ASM__ __SAVE_DS__
cc_sf_putchar(
	__REG__(a0, struct Hook *hook),
	__REG__(a2, APTR object),
	__REG__(a1, ULONG ch))
{
	cc_sf_hook_data *ctx;

	(void)object;
	ctx = (cc_sf_hook_data *)hook->h_Data;
	if (ctx == (cc_sf_hook_data *)NULL)
		return 0UL;

	if (ch == 0UL)
	{
		if (!ctx->overflow && ctx->pos < ctx->maxsize)
			ctx->buf[ctx->pos] = '\0';
		return 0UL;
	}

	if (ctx->overflow)
		return 0UL;

	/* Reserve room for the terminating NUL. */
	if (ctx->pos + 1UL >= ctx->maxsize)
	{
		ctx->overflow = 1L;
		return 0UL;
	}

	ctx->buf[ctx->pos] = (char)ch;
	ctx->pos++;
	return 0UL;
}

static LONG
cc_tm_to_datestamp(cc_tm *t, struct DateStamp *ds)
{
	struct ClockData cd;
	ULONG secs;
	LONG year;
	LONG mon;

	if (t == (cc_tm *)NULL || ds == (struct DateStamp *)NULL)
		return 0L;

	year = (LONG)t->tm_year + 1900L;
	mon = (LONG)t->tm_mon;
	if (mon < 0L || mon > 11L || t->tm_mday < 1)
		return 0L;

	cd.sec = (UWORD)t->tm_sec;
	cd.min = (UWORD)t->tm_min;
	cd.hour = (UWORD)t->tm_hour;
	cd.mday = (UWORD)t->tm_mday;
	cd.month = (UWORD)(mon + 1L);
	cd.year = (UWORD)year;
	cd.wday = 0;

	if (UtilityBase == (struct Library *)NULL)
		return 0L;

	secs = Date2Amiga(&cd);
	ds->ds_Days = (LONG)(secs / 86400UL);
	ds->ds_Minute = (LONG)((secs % 86400UL) / 60UL);
	ds->ds_Tick = (LONG)((secs % 60UL) * 50UL);
	return 1L;
}

LONG
cc_strftime_locale(cc_task_UserData *ud, STRPTR s, ULONG maxsize,
	STRPTR format, APTR tm, ULONG *out_len)
{
	struct Locale *loc;
	struct DateStamp ds;
	struct Hook hook;
	cc_sf_hook_data ctx;
	STRPTR tmpl;
	cc_tm *t;

	if (out_len != (ULONG *)NULL)
		*out_len = 0UL;

	if (ud == (cc_task_UserData *)NULL
	    || (ud->lc_mask & CC_LC_TIME) == 0UL
	    || ud->locale == (APTR)NULL
	    || LocaleBase == (struct LocaleBase *)NULL
	    || s == (STRPTR)NULL
	    || format == (STRPTR)NULL
	    || tm == (APTR)NULL
	    || maxsize == 0UL)
		return 0L;

	loc = (struct Locale *)ud->locale;
	t = (cc_tm *)tm;
	if (!cc_tm_to_datestamp(t, &ds))
		return 0L;

	/*
	 * Bare %c / %x / %X -> locale preferred templates (POSIX "appropriate
	 * date and time representation").  Other formats go straight to
	 * FormatDate so %a/%A/%b/%B/%p pick up localized names.
	 */
	tmpl = format;
	if (format[0] == '%' && format[1] != '\0' && format[2] == '\0')
	{
		if (format[1] == 'c'
		    && loc->loc_DateTimeFormat != (STRPTR)NULL)
			tmpl = loc->loc_DateTimeFormat;
		else if (format[1] == 'x'
			 && loc->loc_DateFormat != (STRPTR)NULL)
			tmpl = loc->loc_DateFormat;
		else if (format[1] == 'X'
			 && loc->loc_TimeFormat != (STRPTR)NULL)
			tmpl = loc->loc_TimeFormat;
	}

	ctx.buf = s;
	ctx.maxsize = maxsize;
	ctx.pos = 0UL;
	ctx.overflow = 0L;

	hook.h_Entry = (HOOKFUNC)cc_sf_putchar;
	hook.h_SubEntry = (HOOKFUNC)NULL;
	hook.h_Data = (APTR)&ctx;

	FormatDate(loc, tmpl, &ds, &hook);

	/*
	 * We owned the FormatDate attempt.  Overflow -> length 0 (strftime
	 * returns 0); do not fall back to Pure C (would hide truncation).
	 */
	if (ctx.overflow || ctx.pos >= maxsize)
	{
		if (out_len != (ULONG *)NULL)
			*out_len = 0UL;
		return 1L;
	}

	if (out_len != (ULONG *)NULL)
		*out_len = ctx.pos;
	return 1L;
}
