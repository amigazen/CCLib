#ifndef LOCALE_H
#define LOCALE_H 1

/*
 * locale.h — POSIX locale surface for CClib.
 *
 * Wave 1: setlocale / localeconv / strcoll / strxfrm are LVOs.
 * Default session locale is "C" (no OpenLocale).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

/* Category selectors (POSIX).  Mapped to CC_LC_* bits inside the library. */
#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MESSAGES 6

struct lconv {
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
};

#ifdef ANSIC
char *setlocale(int category, const char *locale);
struct lconv *localeconv(void);
long strcoll(const char *s1, const char *s2);
unsigned long strxfrm(char *dest, const char *src, unsigned long n);
#else
char *setlocale();
struct lconv *localeconv();
long strcoll();
unsigned long strxfrm();
#endif

#endif /* LOCALE_H */
