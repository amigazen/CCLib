/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * meval.c - evaluate a floating expression (CClib CRT + ma.lib link test)
 *
 * Ordinary hosted C for stdio; math symbols come from ma.lib (static).
 * Compile MATH=IEEE; link amiga.lib + ma.lib — no sc.lib.
 *
 * Usage: meval 'sin(pi/4)*sqrt(2)'
 *        meval 2+2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

/* Expression cursor and last error message (NULL if ok). */
static const char *ep;
static const char *eerr;

static double parse_expr(void);
static double parse_term(void);
static double parse_power(void);
static double parse_unary(void);
static double parse_primary(void);

static void
skip_ws(void)
{
	while (*ep != '\0' && isspace((unsigned char)*ep))
		ep++;
}

static int
match(char c)
{
	skip_ws();
	if (*ep == c)
	{
		ep++;
		return 1;
	}
	return 0;
}

static int
is_ident_start(char c)
{
	return (isalpha((unsigned char)c) || c == '_');
}

static int
is_ident_char(char c)
{
	return (isalnum((unsigned char)c) || c == '_');
}

/*
 * Read an identifier into buf (lowercase). Returns 0 on success.
 */
static int
read_ident(char *buf, size_t buflen)
{
	size_t n;

	n = 0;
	skip_ws();
	if (!is_ident_start(*ep))
		return -1;
	while (is_ident_char(*ep))
	{
		if (n + 1 >= buflen)
		{
			eerr = "name too long";
			return -1;
		}
		buf[n] = (char)tolower((unsigned char)*ep);
		n++;
		ep++;
	}
	buf[n] = '\0';
	return 0;
}

static double
call1(double (*fn)(double), double x)
{
	errno = 0;
	return fn(x);
}

static double
call2(double (*fn)(double, double), double x, double y)
{
	errno = 0;
	return fn(x, y);
}

static double
parse_primary(void)
{
	double v;
	double a;
	double b;
	char name[32];
	char *end;

	skip_ws();
	if (*ep == '\0')
	{
		eerr = "unexpected end";
		return 0.0;
	}

	if (match('('))
	{
		v = parse_expr();
		if (eerr != (const char *)NULL)
			return 0.0;
		if (!match(')'))
		{
			eerr = "expected ')'";
			return 0.0;
		}
		return v;
	}

	if (is_ident_start(*ep))
	{
		if (read_ident(name, sizeof(name)) != 0)
			return 0.0;

		/* Named constants */
		if (strcmp(name, "pi") == 0)
			return M_PI;
		if (strcmp(name, "e") == 0)
			return M_E;
		if (strcmp(name, "phi") == 0)
			return M_PHI;

		if (!match('('))
		{
			eerr = "expected '(' after function";
			return 0.0;
		}
		a = parse_expr();
		if (eerr != (const char *)NULL)
			return 0.0;

		/* Binary functions: pow(x,y) hypot(x,y) atan2(y,x) fmod(x,y) */
		if (match(','))
		{
			b = parse_expr();
			if (eerr != (const char *)NULL)
				return 0.0;
			if (!match(')'))
			{
				eerr = "expected ')'";
				return 0.0;
			}
			if (strcmp(name, "pow") == 0)
				return call2(pow, a, b);
			if (strcmp(name, "hypot") == 0)
				return call2(hypot, a, b);
			if (strcmp(name, "atan2") == 0)
				return call2(atan2, a, b);
			if (strcmp(name, "fmod") == 0)
				return call2(fmod, a, b);
			if (strcmp(name, "fmin") == 0)
				return call2(fmin, a, b);
			if (strcmp(name, "fmax") == 0)
				return call2(fmax, a, b);
			eerr = "unknown binary function";
			return 0.0;
		}

		if (!match(')'))
		{
			eerr = "expected ')'";
			return 0.0;
		}

		if (strcmp(name, "sin") == 0)
			return call1(sin, a);
		if (strcmp(name, "cos") == 0)
			return call1(cos, a);
		if (strcmp(name, "tan") == 0)
			return call1(tan, a);
		if (strcmp(name, "asin") == 0)
			return call1(asin, a);
		if (strcmp(name, "acos") == 0)
			return call1(acos, a);
		if (strcmp(name, "atan") == 0)
			return call1(atan, a);
		if (strcmp(name, "sinh") == 0)
			return call1(sinh, a);
		if (strcmp(name, "cosh") == 0)
			return call1(cosh, a);
		if (strcmp(name, "tanh") == 0)
			return call1(tanh, a);
		if (strcmp(name, "exp") == 0)
			return call1(exp, a);
		if (strcmp(name, "log") == 0)
			return call1(log, a);
		if (strcmp(name, "log10") == 0)
			return call1(log10, a);
		if (strcmp(name, "log2") == 0)
			return call1(log2, a);
		if (strcmp(name, "sqrt") == 0)
			return call1(sqrt, a);
		if (strcmp(name, "cbrt") == 0)
			return call1(cbrt, a);
		if (strcmp(name, "fabs") == 0)
			return call1(fabs, a);
		if (strcmp(name, "floor") == 0)
			return call1(floor, a);
		if (strcmp(name, "ceil") == 0)
			return call1(ceil, a);
		if (strcmp(name, "trunc") == 0)
			return call1(trunc, a);
		if (strcmp(name, "round") == 0)
			return call1(round, a);
		if (strcmp(name, "expm1") == 0)
			return call1(expm1, a);
		if (strcmp(name, "log1p") == 0)
			return call1(log1p, a);

		eerr = "unknown function";
		return 0.0;
	}

	/* Number via CClib strtod */
	v = strtod(ep, &end);
	if (end == ep)
	{
		eerr = "expected number or '('";
		return 0.0;
	}
	ep = end;
	return v;
}

static double
parse_unary(void)
{
	skip_ws();
	if (match('+'))
		return parse_unary();
	if (match('-'))
		return -parse_unary();
	return parse_primary();
}

/* Right-associative ^ (pow). */
static double
parse_power(void)
{
	double left;
	double right;

	left = parse_unary();
	if (eerr != (const char *)NULL)
		return 0.0;
	skip_ws();
	if (match('^'))
	{
		right = parse_power();
		if (eerr != (const char *)NULL)
			return 0.0;
		return call2(pow, left, right);
	}
	return left;
}

static double
parse_term(void)
{
	double left;
	double right;

	left = parse_power();
	if (eerr != (const char *)NULL)
		return 0.0;
	for (;;)
	{
		skip_ws();
		if (match('*'))
		{
			right = parse_power();
			if (eerr != (const char *)NULL)
				return 0.0;
			left = left * right;
		}
		else if (match('/'))
		{
			right = parse_power();
			if (eerr != (const char *)NULL)
				return 0.0;
			if (right == 0.0)
			{
				eerr = "division by zero";
				return 0.0;
			}
			left = left / right;
		}
		else if (match('%'))
		{
			right = parse_power();
			if (eerr != (const char *)NULL)
				return 0.0;
			left = call2(fmod, left, right);
		}
		else
			break;
	}
	return left;
}

static double
parse_expr(void)
{
	double left;
	double right;

	left = parse_term();
	if (eerr != (const char *)NULL)
		return 0.0;
	for (;;)
	{
		skip_ws();
		if (match('+'))
		{
			right = parse_term();
			if (eerr != (const char *)NULL)
				return 0.0;
			left = left + right;
		}
		else if (match('-'))
		{
			right = parse_term();
			if (eerr != (const char *)NULL)
				return 0.0;
			left = left - right;
		}
		else
			break;
	}
	return left;
}

static void
usage(void)
{
	fputs("usage: meval <expression>\n", stderr);
	fputs("  constants: pi e phi\n", stderr);
	fputs("  ops: + - * / % ^  and ( )\n", stderr);
	fputs("  funcs: sin cos tan asin acos atan atan2 sinh cosh tanh\n", stderr);
	fputs("         exp log log10 log2 sqrt cbrt fabs floor ceil trunc round\n", stderr);
	fputs("         expm1 log1p pow hypot fmod fmin fmax\n", stderr);
}

int
main(int argc, char **argv)
{
	double result;
	size_t len;
	int i;
	char *expr;
	char *p;

	if (argc < 2)
	{
		usage();
		return 1;
	}

	/* Join argv[1..] with spaces so `meval 2 + 2` works. */
	len = 0;
	i = 1;
	while (i < argc)
	{
		len += strlen(argv[i]) + 1;
		i++;
	}
	expr = (char *)malloc(len + 1);
	if (expr == (char *)NULL)
	{
		fputs("meval: out of memory\n", stderr);
		return 1;
	}
	p = expr;
	i = 1;
	while (i < argc)
	{
		size_t n;

		n = strlen(argv[i]);
		memcpy(p, argv[i], n);
		p += n;
		if (i + 1 < argc)
		{
			*p = ' ';
			p++;
		}
		i++;
	}
	*p = '\0';

	ep = expr;
	eerr = (const char *)NULL;
	result = parse_expr();
	skip_ws();
	if (eerr == (const char *)NULL && *ep != '\0')
		eerr = "trailing junk";

	if (eerr != (const char *)NULL)
	{
		fprintf(stderr, "meval: %s\n", eerr);
		free(expr);
		return 1;
	}

	/* %g via CClib + mathieeedoubbas (CRT _math stub). */
	printf("%.17g\n", result);
	free(expr);
	return 0;
}
