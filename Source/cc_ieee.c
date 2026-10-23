/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cc_ieee.c - IEEEDP* calls through SetupSTDIO's per-task mathbase
 *
 * No C floating-point operators - only pass DOUBLE bits to mathieeedoubbas.
 * LVO jumpers live in cc_ieee_lvo.asm; C uses __ASM__/__REG__ prototypes
 * from <clib/compiler-specific.h> (CLib39x style).
 */

#include <exec/types.h>
#include <clib/compiler-specific.h>

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "private/cc_ieee.h"

/* Defined in cc_ieee_lvo.asm - base in A6, IEEE args in d0/d1[/d2/d3]. */
DOUBLE __ASM__ cc_ieeedp_flt_raw(
	__REG__(a6, APTR base), __REG__(d0, LONG v));
LONG __ASM__ cc_ieeedp_fix_raw(
	__REG__(a6, APTR base), __REG__(d0, DOUBLE v));
LONG __ASM__ cc_ieeedp_tst_raw(
	__REG__(a6, APTR base), __REG__(d0, DOUBLE v));
LONG __ASM__ cc_ieeedp_cmp_raw(
	__REG__(a6, APTR base),
	__REG__(d0, DOUBLE a),
	__REG__(d2, DOUBLE b));
DOUBLE __ASM__ cc_ieeedp_abs_raw(
	__REG__(a6, APTR base), __REG__(d0, DOUBLE v));
DOUBLE __ASM__ cc_ieeedp_neg_raw(
	__REG__(a6, APTR base), __REG__(d0, DOUBLE v));
DOUBLE __ASM__ cc_ieeedp_add_raw(
	__REG__(a6, APTR base),
	__REG__(d0, DOUBLE a),
	__REG__(d2, DOUBLE b));
DOUBLE __ASM__ cc_ieeedp_sub_raw(
	__REG__(a6, APTR base),
	__REG__(d0, DOUBLE a),
	__REG__(d2, DOUBLE b));
DOUBLE __ASM__ cc_ieeedp_mul_raw(
	__REG__(a6, APTR base),
	__REG__(d0, DOUBLE a),
	__REG__(d2, DOUBLE b));
DOUBLE __ASM__ cc_ieeedp_div_raw(
	__REG__(a6, APTR base),
	__REG__(d0, DOUBLE a),
	__REG__(d2, DOUBLE b));
DOUBLE __ASM__ cc_ieeedp_floor_raw(
	__REG__(a6, APTR base), __REG__(d0, DOUBLE v));

APTR
cc_get_mathbase(void)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return (APTR)NULL;
	return ud->mathbase;
}

DOUBLE
cc_ieeedp_zero(void)
{
	union {
		DOUBLE d;
		ULONG w[2];
	} u;

	u.w[0] = 0;
	u.w[1] = 0;
	return u.d;
}

DOUBLE
cc_ieeedp_flt(APTR base, LONG v)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_flt_raw(base, v);
}

LONG
cc_ieeedp_fix(APTR base, DOUBLE v)
{
	if (base == (APTR)NULL)
		return 0;
	return cc_ieeedp_fix_raw(base, v);
}

LONG
cc_ieeedp_tst(APTR base, DOUBLE v)
{
	if (base == (APTR)NULL)
		return 0;
	return cc_ieeedp_tst_raw(base, v);
}

LONG
cc_ieeedp_cmp(APTR base, DOUBLE a, DOUBLE b)
{
	if (base == (APTR)NULL)
		return 0;
	return cc_ieeedp_cmp_raw(base, a, b);
}

DOUBLE
cc_ieeedp_abs(APTR base, DOUBLE v)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_abs_raw(base, v);
}

DOUBLE
cc_ieeedp_neg(APTR base, DOUBLE v)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_neg_raw(base, v);
}

DOUBLE
cc_ieeedp_add(APTR base, DOUBLE a, DOUBLE b)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_add_raw(base, a, b);
}

DOUBLE
cc_ieeedp_sub(APTR base, DOUBLE a, DOUBLE b)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_sub_raw(base, a, b);
}

DOUBLE
cc_ieeedp_mul(APTR base, DOUBLE a, DOUBLE b)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_mul_raw(base, a, b);
}

DOUBLE
cc_ieeedp_div(APTR base, DOUBLE a, DOUBLE b)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_div_raw(base, a, b);
}

DOUBLE
cc_ieeedp_floor(APTR base, DOUBLE v)
{
	if (base == (APTR)NULL)
		return cc_ieeedp_zero();
	return cc_ieeedp_floor_raw(base, v);
}

DOUBLE
cc_ieeedp_ten(APTR base)
{
	return cc_ieeedp_flt(base, 10);
}

DOUBLE
cc_ieeedp_pow10(APTR base, LONG exp)
{
	DOUBLE ten;
	DOUBLE r;
	LONG n;
	LONG i;

	if (base == (APTR)NULL)
		return cc_ieeedp_zero();

	ten = cc_ieeedp_ten(base);
	r = cc_ieeedp_flt(base, 1);
	if (exp == 0)
		return r;

	n = exp;
	if (n < 0)
		n = -n;

	i = 0;
	while (i < n)
	{
		r = cc_ieeedp_mul(base, r, ten);
		i++;
	}

	if (exp < 0)
		r = cc_ieeedp_div(base, cc_ieeedp_flt(base, 1), r);
	return r;
}

DOUBLE
cc_atof_ieee(APTR base, STRPTR s)
{
	STRPTR p;
	DOUBLE ten;
	DOUBLE r;
	DOUBLE frac;
	DOUBLE scale;
	LONG sign;
	LONG digit;
	LONG any;
	LONG esign;
	LONG expv;

	if (base == (APTR)NULL || s == (STRPTR)NULL)
		return cc_ieeedp_zero();

	ten = cc_ieeedp_ten(base);
	r = cc_ieeedp_zero();
	any = 0;
	sign = 0;

	p = s;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ||
	       *p == '\f' || *p == '\v')
		p++;

	if (*p == '-')
	{
		sign = 1;
		p++;
	}
	else if (*p == '+')
		p++;

	while (*p >= '0' && *p <= '9')
	{
		digit = (LONG)(*p - '0');
		r = cc_ieeedp_mul(base, r, ten);
		r = cc_ieeedp_add(base, r, cc_ieeedp_flt(base, digit));
		any = 1;
		p++;
	}

	if (*p == '.')
	{
		p++;
		frac = cc_ieeedp_zero();
		scale = cc_ieeedp_flt(base, 1);
		while (*p >= '0' && *p <= '9')
		{
			digit = (LONG)(*p - '0');
			scale = cc_ieeedp_div(base, scale, ten);
			frac = cc_ieeedp_add(base, frac,
				cc_ieeedp_mul(base, cc_ieeedp_flt(base, digit), scale));
			any = 1;
			p++;
		}
		r = cc_ieeedp_add(base, r, frac);
	}

	if ((*p == 'e' || *p == 'E') && any)
	{
		p++;
		esign = 1;
		if (*p == '-')
		{
			esign = -1;
			p++;
		}
		else if (*p == '+')
			p++;
		expv = 0;
		while (*p >= '0' && *p <= '9')
		{
			expv = expv * 10 + (LONG)(*p - '0');
			p++;
		}
		if (esign < 0)
			expv = -expv;
		if (expv != 0)
			r = cc_ieeedp_mul(base, r, cc_ieeedp_pow10(base, expv));
	}

	if (!any)
		return cc_ieeedp_zero();
	if (sign)
		r = cc_ieeedp_neg(base, r);
	return r;
}

/*
 * Format using only IEEEDP*.  'f' fixed, 'e'/'E' scientific, 'g'/'G' choose.
 */
LONG
cc_dtoa_ieee(APTR base, DOUBLE x, STRPTR buf, LONG prec, LONG style)
{
	DOUBLE ten;
	DOUBLE ax;
	DOUBLE y;
	DOUBLE ip;
	DOUBLE frac;
	LONG neg;
	LONG exp10;
	LONG dig;
	LONG i;
	LONG n;
	LONG use_exp;
	LONG st;
	char *out;
	char digits[32];
	LONG ndig;

	if (buf == (STRPTR)NULL)
		return 0;
	buf[0] = '\0';
	if (base == (APTR)NULL)
	{
		buf[0] = '0';
		buf[1] = '\0';
		return 1;
	}

	if (prec < 0)
		prec = 6;
	if (prec > 18)
		prec = 18;

	st = style;
	if (st >= 'A' && st <= 'Z')
		st = st - 'A' + 'a';

	ten = cc_ieeedp_ten(base);
	neg = 0;
	if (cc_ieeedp_tst(base, x) < 0)
	{
		neg = 1;
		ax = cc_ieeedp_neg(base, x);
	}
	else
		ax = x;

	if (cc_ieeedp_tst(base, ax) == 0)
	{
		out = (char *)buf;
		if (neg)
			*out++ = '-';
		*out++ = '0';
		if (st == 'f' || (st == 'g' && prec > 0))
		{
			*out++ = '.';
			i = 0;
			while (i < prec)
			{
				*out++ = '0';
				i++;
			}
		}
		*out = '\0';
		return (LONG)(out - (char *)buf);
	}

	/* Decimal exponent: scale ax into [1,10). */
	exp10 = 0;
	y = ax;
	while (cc_ieeedp_cmp(base, y, ten) >= 0)
	{
		y = cc_ieeedp_div(base, y, ten);
		exp10++;
		if (exp10 > 308)
			break;
	}
	while (cc_ieeedp_cmp(base, y, cc_ieeedp_flt(base, 1)) < 0)
	{
		y = cc_ieeedp_mul(base, y, ten);
		exp10--;
		if (exp10 < -308)
			break;
	}

	use_exp = 0;
	if (st == 'e')
		use_exp = 1;
	else if (st == 'g')
	{
		if (exp10 < -4 || exp10 >= prec)
			use_exp = 1;
	}

	ndig = 0;
	if (use_exp)
	{
		n = prec + 1;
		i = 0;
		while (i < n && i < 24)
		{
			dig = cc_ieeedp_fix(base, y);
			if (dig < 0)
				dig = 0;
			if (dig > 9)
				dig = 9;
			digits[ndig++] = (char)('0' + dig);
			y = cc_ieeedp_sub(base, y, cc_ieeedp_flt(base, dig));
			y = cc_ieeedp_mul(base, y, ten);
			i++;
		}

		out = (char *)buf;
		if (neg)
			*out++ = '-';
		*out++ = digits[0];
		if (prec > 0)
		{
			*out++ = '.';
			i = 1;
			while (i < ndig)
			{
				*out++ = digits[i];
				i++;
			}
		}
		*out++ = (char)((style == 'E' || style == 'G') ? 'E' : 'e');
		if (exp10 < 0)
		{
			*out++ = '-';
			n = -exp10;
		}
		else
		{
			*out++ = '+';
			n = exp10;
		}
		if (n >= 100)
		{
			*out++ = (char)('0' + (n / 100));
			n = n % 100;
		}
		*out++ = (char)('0' + (n / 10));
		*out++ = (char)('0' + (n % 10));
		*out = '\0';
		return (LONG)(out - (char *)buf);
	}

	y = ax;
	ip = cc_ieeedp_floor(base, y);
	frac = cc_ieeedp_sub(base, y, ip);

	{
		char itmp[24];
		LONG ni;
		DOUBLE t;
		DOUBLE q;

		ni = 0;
		t = ip;
		if (cc_ieeedp_tst(base, t) == 0)
		{
			itmp[ni++] = '0';
		}
		else
		{
			while (cc_ieeedp_tst(base, t) > 0 && ni < 24)
			{
				q = cc_ieeedp_div(base, t, ten);
				q = cc_ieeedp_floor(base, q);
				dig = cc_ieeedp_fix(base,
					cc_ieeedp_sub(base, t, cc_ieeedp_mul(base, q, ten)));
				if (dig < 0)
					dig = -dig;
				itmp[ni++] = (char)('0' + dig);
				t = q;
			}
		}

		out = (char *)buf;
		if (neg)
			*out++ = '-';
		i = ni;
		while (i > 0)
		{
			i--;
			*out++ = itmp[i];
		}
	}

	if (prec > 0)
	{
		*out++ = '.';
		i = 0;
		while (i < prec)
		{
			frac = cc_ieeedp_mul(base, frac, ten);
			dig = cc_ieeedp_fix(base, frac);
			if (dig < 0)
				dig = 0;
			if (dig > 9)
				dig = 9;
			*out++ = (char)('0' + dig);
			frac = cc_ieeedp_sub(base, frac, cc_ieeedp_flt(base, dig));
			i++;
		}
	}

	*out = '\0';
	return (LONG)(out - (char *)buf);
}
