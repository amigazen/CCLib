/*
 * conv.c - numeric and case conversion LVOs for cclib.library
 *
 * atof uses mathieeedoubbas via the task's SetupSTDIO mathbase when present.
 * Fallback builds IEEE bits with integer math only (no C `double` ops / sc.lib).
 */

#include <exec/types.h>
#include <sys/types.h>

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "private/cc_ieee.h"
#include "cclib_funcs.h"

static STRPTR cc_skip_space(STRPTR s)
{
	if (s == (STRPTR)NULL)
		return s;
	while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' ||
	       *s == '\f' || *s == '\v')
		s++;
	return s;
}

static DOUBLE cc_dp_words(ULONG hi, ULONG lo)
{
	union {
		DOUBLE d;
		ULONG w[2];
	} u;

	u.w[0] = hi;
	u.w[1] = lo;
	return u.d;
}

static DOUBLE cc_dp_zero(void)
{
	return cc_dp_words(0, 0);
}

/*
 * Fallback when mathbase is NULL: mag * 10^(-scale10) as IEEE bits.
 */
static DOUBLE cc_mag_scale_to_dp(ULONG mag, LONG scale10, LONG sign)
{
	ULONG m;
	LONG exp2;
	ULONG frac_hi;
	ULONG frac_lo;
	ULONG hi;
	ULONG lo;
	LONG i;

	if (mag == 0)
		return cc_dp_zero();

	m = mag;
	exp2 = 0;

	{
		ULONG hi64;
		ULONG lo64;
		LONG lz;

		hi64 = m;
		lo64 = 0;
		lz = 0;
		while (hi64 < 0x80000000UL) {
			hi64 <<= 1;
			lz++;
		}
		exp2 = 31 - lz;

		i = 0;
		while (i < scale10) {
			ULONG q0, q1, rem;
			ULONG n0 = hi64;
			ULONG n1 = lo64;
			ULONG mid, qmid, rmid, loh, qloh;

			rem = n0 % 10UL;
			q0 = n0 / 10UL;
			mid = (rem << 16) | (n1 >> 16);
			qmid = mid / 10UL;
			rmid = mid % 10UL;
			loh = (rmid << 16) | (n1 & 0xFFFFUL);
			qloh = loh / 10UL;
			q1 = (qmid << 16) | qloh;
			hi64 = q0;
			lo64 = q1;
			i++;
			while (hi64 < 0x80000000UL && (hi64 | lo64) != 0) {
				hi64 = (hi64 << 1) | (lo64 >> 31);
				lo64 <<= 1;
				exp2--;
			}
		}

		if (hi64 == 0 && lo64 == 0)
			return cc_dp_zero();

		{
			LONG bexp;

			bexp = exp2 + 1023;
			if (bexp <= 0)
				return cc_dp_zero();
			if (bexp >= 2047)
				return cc_dp_words(sign ? 0xFFF00000UL : 0x7FF00000UL, 0);

			frac_hi = (hi64 >> 11) & 0x000FFFFFUL;
			frac_lo = (hi64 << 21) | (lo64 >> 11);
			hi = ((ULONG)sign << 31) | ((ULONG)bexp << 20) | frac_hi;
			lo = frac_lo;
			return cc_dp_words(hi, lo);
		}
	}
}

static DOUBLE cc_atof_bits(STRPTR s)
{
	STRPTR p;
	LONG sign;
	ULONG ipart;
	ULONG fpart;
	LONG fdigits;
	LONG digit;
	LONG any;
	LONG k;
	ULONG numer;
	LONG i;

	if (s == (STRPTR)NULL)
		return cc_dp_zero();

	p = cc_skip_space(s);
	sign = 0;
	if (*p == '-') {
		sign = 1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	ipart = 0;
	any = 0;
	while (*p >= '0' && *p <= '9') {
		digit = (LONG)(*p - '0');
		ipart = ipart * 10UL + (ULONG)digit;
		any = 1;
		p++;
	}

	fpart = 0;
	fdigits = 0;
	if (*p == '.') {
		p++;
		while (*p >= '0' && *p <= '9') {
			digit = (LONG)(*p - '0');
			if (fdigits < 9) {
				fpart = fpart * 10UL + (ULONG)digit;
				fdigits++;
			}
			any = 1;
			p++;
		}
	}

	if ((*p == 'e' || *p == 'E') && any) {
		LONG esign;
		LONG expv;

		p++;
		esign = 1;
		if (*p == '-') {
			esign = -1;
			p++;
		} else if (*p == '+') {
			p++;
		}
		expv = 0;
		while (*p >= '0' && *p <= '9') {
			expv = expv * 10 + (LONG)(*p - '0');
			p++;
			any = 1;
		}
		if (esign < 0)
			fdigits += expv;
		else {
			k = 0;
			while (k < expv && ipart < 429496729UL) {
				ipart = ipart * 10UL;
				k++;
			}
		}
	}

	if (!any)
		return cc_dp_zero();

	numer = ipart;
	i = 0;
	while (i < fdigits && numer <= 429496729UL) {
		numer = numer * 10UL;
		i++;
	}
	numer = numer + fpart;

	return cc_mag_scale_to_dp(numer, fdigits, sign);
}

LONG __ASM__ __SAVE_DS__ atoi(
	__REG__(a0, STRPTR s))
{
	cc_check_break();
	return atol(s);
}

LONG __ASM__ __SAVE_DS__ atol(
	__REG__(a0, STRPTR s))
{
	STRPTR p;
	LONG sign;
	LONG n;
	LONG digit;
	LONG any;
	cc_check_break();

	if (s == (STRPTR)NULL)
		return 0;

	p = cc_skip_space(s);
	sign = 1;
	if (*p == '-') {
		sign = -1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	n = 0;
	any = 0;
	while (*p >= '0' && *p <= '9') {
		digit = (LONG)(*p - '0');
		n = n * 10 + digit;
		any = 1;
		p++;
	}

	if (!any)
		return 0;
	if (sign < 0)
		n = -n;
	return n;
}

DOUBLE __ASM__ __SAVE_DS__ atof(
	__REG__(a0, STRPTR s))
{
	APTR base;
	cc_check_break();

	base = cc_get_mathbase();
	if (base != (APTR)NULL)
		return cc_atof_ieee(base, s);
	return cc_atof_bits(s);
}

void __ASM__ __SAVE_DS__ ltoa(
	__REG__(d0, LONG n),
	__REG__(a0, STRPTR s))
{
	char tmp[12];
	LONG i;
	LONG j;
	LONG v;
	LONG neg;
	cc_check_break();

	if (s == (STRPTR)NULL)
		return;

	if (n == 0) {
		s[0] = '0';
		s[1] = '\0';
		return;
	}

	neg = 0;
	v = n;
	if (v < 0) {
		neg = 1;
		if (v == (LONG)0x80000000) {
			s[0] = '-';
			s[1] = '2';
			s[2] = '1';
			s[3] = '4';
			s[4] = '7';
			s[5] = '4';
			s[6] = '8';
			s[7] = '3';
			s[8] = '6';
			s[9] = '4';
			s[10] = '8';
			s[11] = '\0';
			return;
		}
		v = -v;
	}

	i = 0;
	while (v > 0) {
		tmp[i] = (char)('0' + (v % 10));
		i++;
		v = v / 10;
	}

	j = 0;
	if (neg) {
		s[j] = '-';
		j++;
	}
	while (i > 0) {
		i--;
		s[j] = tmp[i];
		j++;
	}
	s[j] = '\0';
}

void __ASM__ __SAVE_DS__ stoa(
	__REG__(d0, LONG n),
	__REG__(a0, STRPTR s))
{
	SHORT sn;
	cc_check_break();

	sn = (SHORT)n;
	ltoa((LONG)sn, s);
}

LONG __ASM__ __SAVE_DS__ toupper(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_toupper_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ tolower(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_tolower_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isalnum(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isalnum_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isalpha(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isalpha_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ iscntrl(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_iscntrl_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isdigit(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isdigit_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isgraph(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isgraph_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ islower(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_islower_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isprint(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isprint_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ ispunct(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_ispunct_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isspace(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isspace_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isupper(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isupper_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isxdigit(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isxdigit_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isblank(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isblank_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ isascii(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isascii_ch(c);
}

/* Digit value for bases 2..36; -1 if not a digit in that base. */
static LONG cc_digit_val(LONG ch, LONG base)
{
	LONG v;

	if (ch >= '0' && ch <= '9')
		v = ch - '0';
	else if (ch >= 'a' && ch <= 'z')
		v = ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'Z')
		v = ch - 'A' + 10;
	else
		return -1L;
	if (v >= base)
		return -1L;
	return v;
}

/*
 * Shared unsigned parser for strtol/strtoul.  Writes *out, *end_out,
 * *overflow, *any.  Returns 0 always (status via outs).
 */
static void cc_parse_ulong(
	STRPTR s,
	LONG base,
	ULONG *out,
	STRPTR *end_out,
	LONG *overflow,
	LONG *any,
	LONG *neg_out)
{
	STRPTR p;
	STRPTR start;
	LONG sign;
	LONG b;
	LONG digit;
	ULONG lim;
	ULONG acc;
	LONG ov;

	*out = 0UL;
	*end_out = s;
	*overflow = 0;
	*any = 0;
	*neg_out = 0;

	if (s == (STRPTR)NULL)
		return;

	p = cc_skip_space(s);
	sign = 0;
	if (*p == '-') {
		sign = 1;
		p++;
	} else if (*p == '+') {
		p++;
	}

	b = base;
	if (b == 0L) {
		if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
			b = 16L;
			p += 2;
		} else if (p[0] == '0') {
			b = 8L;
			p++;
			*any = 1;
			*end_out = p;
		} else {
			b = 10L;
		}
	} else if (b == 16L && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
		p += 2;
	}

	if (b < 2L || b > 36L) {
		cc_set_errno(CC_EINVAL);
		*end_out = s;
		return;
	}

	start = p;
	acc = 0UL;
	ov = 0;
	lim = 0xFFFFFFFFUL / (ULONG)b;

	for (;;) {
		digit = cc_digit_val((LONG)(UBYTE)*p, b);
		if (digit < 0L)
			break;
		*any = 1;
		if (acc > lim ||
		    (acc == lim && (ULONG)digit > (0xFFFFFFFFUL % (ULONG)b)))
			ov = 1;
		else
			acc = acc * (ULONG)b + (ULONG)digit;
		p++;
	}

	if (!*any) {
		*end_out = s;
		return;
	}

	*out = acc;
	*end_out = p;
	*overflow = ov;
	*neg_out = sign;
	(void)start;
}

LONG __ASM__ __SAVE_DS__ strtol(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR * endp),
	__REG__(d0, LONG base))
{
	ULONG acc;
	STRPTR end;
	LONG ov;
	LONG any;
	LONG neg;
	LONG v;
	cc_check_break();

	cc_parse_ulong(s, base, &acc, &end, &ov, &any, &neg);

	if (endp != (STRPTR *)NULL)
		*endp = any ? end : s;

	if (!any)
		return 0L;

	if (ov) {
		cc_set_errno(CC_ERANGE);
		return neg ? (LONG)0x80000000L : (LONG)0x7FFFFFFFL;
	}

	if (neg) {
		if (acc > 0x80000000UL) {
			cc_set_errno(CC_ERANGE);
			return (LONG)0x80000000L;
		}
		if (acc == 0x80000000UL)
			return (LONG)0x80000000L;
		v = -(LONG)acc;
		return v;
	}

	if (acc > 0x7FFFFFFFUL) {
		cc_set_errno(CC_ERANGE);
		return (LONG)0x7FFFFFFFL;
	}
	return (LONG)acc;
}

ULONG __ASM__ __SAVE_DS__ strtoul(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR * endp),
	__REG__(d0, LONG base))
{
	ULONG acc;
	STRPTR end;
	LONG ov;
	LONG any;
	LONG neg;
	cc_check_break();

	cc_parse_ulong(s, base, &acc, &end, &ov, &any, &neg);

	if (endp != (STRPTR *)NULL)
		*endp = any ? end : s;

	if (!any)
		return 0UL;

	if (ov) {
		cc_set_errno(CC_ERANGE);
		return 0xFFFFFFFFUL;
	}

	/* POSIX: negative subject sequence wraps through unsigned. */
	if (neg)
		return (ULONG)(-(LONG)acc);
	return acc;
}

DOUBLE __ASM__ __SAVE_DS__ strtod(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR * endp))
{
	STRPTR p;
	STRPTR start;
	LONG any;
	DOUBLE v;
	APTR base;
	cc_check_break();

	if (s == (STRPTR)NULL) {
		if (endp != (STRPTR *)NULL)
			*endp = s;
		return cc_dp_zero();
	}

	/*
	 * Walk the same shape as atof to place endptr, then convert via
	 * the existing IEEE/bit parsers on the original string.
	 */
	p = cc_skip_space(s);
	start = p;
	any = 0;
	if (*p == '-' || *p == '+')
		p++;
	while (*p >= '0' && *p <= '9') {
		any = 1;
		p++;
	}
	if (*p == '.') {
		p++;
		while (*p >= '0' && *p <= '9') {
			any = 1;
			p++;
		}
	}
	if ((*p == 'e' || *p == 'E') && any) {
		p++;
		if (*p == '-' || *p == '+')
			p++;
		while (*p >= '0' && *p <= '9') {
			any = 1;
			p++;
		}
	}

	if (endp != (STRPTR *)NULL)
		*endp = any ? p : s;

	if (!any)
		return cc_dp_zero();

	(void)start;
	base = cc_get_mathbase();
	if (base != (APTR)NULL)
		v = cc_atof_ieee(base, s);
	else
		v = cc_atof_bits(s);
	return v;
}

LONG __ASM__ __SAVE_DS__ atoll(
	__REG__(a0, STRPTR s))
{
	/* 32-bit Amiga: atoll is an alias of strtol(..., 10). */
	cc_check_break();

	return strtol(s, (APTR)NULL, 10L);
}

void __ASM__ __SAVE_DS__ srand(
	__REG__(d0, ULONG seed))
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return;
	ud->rand_seed = seed;
}

LONG __ASM__ __SAVE_DS__ rand(void)
{
	cc_task_UserData *ud;
	ULONG s;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return 0L;

	/* Historic ANSI LCG (RAND_MAX 32767). */
	s = ud->rand_seed;
	s = s * 1103515245UL + 12345UL;
	ud->rand_seed = s;
	return (LONG)((s >> 16) & 0x7FFFUL);
}

/*
 * div/ldiv LVO: result via A0 (SAS/C libcall cannot return 8-byte structs).
 */
static void cc_div_body(LONG numer, LONG denom, LONG *q_out, LONG *rem_out)
{
	LONG q;
	LONG rem;

	if (denom == 0L) {
		cc_set_errno(CC_EDOM);
		*q_out = 0L;
		*rem_out = 0L;
		return;
	}

	/*
	 * ISO C toward-zero division.  SAS/C emits utility SDivMod32 for
	 * the / and % operators (see cc_cxutil / SCOPTIONS utillib).
	 */
	q = numer / denom;
	rem = numer % denom;
	*q_out = q;
	*rem_out = rem;
}

void __ASM__ __SAVE_DS__ div(
	__REG__(a0, APTR result),
	__REG__(d0, LONG numer),
	__REG__(d1, LONG denom))
{
	div_t *r;
	LONG q;
	LONG rem;
	cc_check_break();

	r = (div_t *)result;
	if (r == (div_t *)NULL)
		return;
	cc_div_body(numer, denom, &q, &rem);
	r->quot = (int)q;
	r->rem = (int)rem;
}

void __ASM__ __SAVE_DS__ ldiv(
	__REG__(a0, APTR result),
	__REG__(d0, LONG numer),
	__REG__(d1, LONG denom))
{
	ldiv_t *r;
	LONG q;
	LONG rem;
	cc_check_break();

	r = (ldiv_t *)result;
	if (r == (ldiv_t *)NULL)
		return;
	cc_div_body(numer, denom, &q, &rem);
	r->quot = q;
	r->rem = rem;
}

LONG __ASM__ __SAVE_DS__ labs(
	__REG__(d0, LONG n))
{
	cc_check_break();
	if (n < 0L) {
		if (n == (LONG)0x80000000L)
			return n;
		return -n;
	}
	return n;
}
