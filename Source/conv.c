/*
 * conv.c - numeric and case conversion LVOs for cclib.library
 *
 * atof uses mathieeedoubbas via the task's SetupSTDIO mathbase when present.
 * Fallback builds IEEE bits with integer math only (no C `double` ops / sc.lib).
 */

#include <exec/types.h>

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

	sn = (SHORT)n;
	ltoa((LONG)sn, s);
}

LONG __ASM__ __SAVE_DS__ toupper(
	__REG__(d0, LONG c))
{
	return cc_toupper_ch(cc_get_ud(), c);
}

LONG __ASM__ __SAVE_DS__ tolower(
	__REG__(d0, LONG c))
{
	return cc_tolower_ch(cc_get_ud(), c);
}
