/*
 * fmt.c - printf/scanf family for cclib.library
 *
 * Integer/string formats match RawDoFmt-style APTR arg arrays.
 * %e/%f/%g use mathieeedoubbas via the task mathbase (CClib.Doc IEEE rule).
 * No C `double` arithmetic — DOUBLE bits only through cc_ieee helpers.
 */

#include <exec/types.h>

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "private/cc_ieee.h"
#include "cclib_funcs.h"

typedef void (*fmt_put_t)(UBYTE ch, APTR data);

typedef struct
{
	STRPTR ptr;
	LONG count;
} fmt_str_ctx;

/* Bounded string sink for snprintf / vsnprintf (C99 truncation). */
typedef struct
{
	STRPTR buf;
	ULONG size;   /* caller n; 0 means do not store */
	ULONG stored; /* payload chars written (excludes NUL) */
	LONG count;   /* would-be length excluding NUL */
} fmt_strn_ctx;

typedef struct
{
	cc_FILE *fp;
	LONG count;
	LONG err;
} fmt_file_ctx;

static void fmt_put_str(UBYTE ch, APTR data)
{
	fmt_str_ctx *ctx;

	ctx = (fmt_str_ctx *)data;
	if (ctx->ptr != (STRPTR)NULL)
	{
		*(ctx->ptr) = (char)ch;
		ctx->ptr++;
	}
	if (ch != 0)
		ctx->count++;
}

/*
 * Store at most size-1 payload bytes, always NUL-terminate when size > 0.
 * count still tracks the full untruncated length (C99 snprintf return).
 */
static void fmt_put_strn(UBYTE ch, APTR data)
{
	fmt_strn_ctx *ctx;

	ctx = (fmt_strn_ctx *)data;
	if (ch == 0)
	{
		if (ctx->size > 0UL && ctx->buf != (STRPTR)NULL)
			ctx->buf[ctx->stored] = '\0';
		return;
	}
	ctx->count++;
	if (ctx->size > 0UL && ctx->buf != (STRPTR)NULL
	    && ctx->stored + 1UL < ctx->size)
	{
		ctx->buf[ctx->stored] = (char)ch;
		ctx->stored++;
	}
}

static void fmt_put_file(UBYTE ch, APTR data)
{
	fmt_file_ctx *ctx;

	ctx = (fmt_file_ctx *)data;
	if (ch == 0)
		return;
	if (ctx->err)
		return;
	if (putc((LONG)ch, (APTR)ctx->fp) == (LONG)CC_EOF)
	{
		ctx->err = 1;
		return;
	}
	ctx->count++;
}

static void fmt_puts(fmt_put_t put, APTR data, STRPTR s)
{
	if (s == (STRPTR)NULL)
		s = (STRPTR)"(null)";
	while (*s)
	{
		put((UBYTE)*s, data);
		s++;
	}
}

static APTR fmt_arg_long(APTR *argp, LONG *out)
{
	LONG *p;

	p = (LONG *)*argp;
	*out = *p;
	*argp = (APTR)(p + 1);
	return (APTR)p;
}

static DOUBLE fmt_arg_double(APTR *argp)
{
	union {
		DOUBLE d;
		ULONG w[2];
	} u;
	ULONG *p;

	p = (ULONG *)*argp;
	u.w[0] = p[0];
	u.w[1] = p[1];
	*argp = (APTR)(p + 2);
	return u.d;
}

static void fmt_put_pad(fmt_put_t put, APTR data, LONG n, UBYTE ch)
{
	while (n > 0)
	{
		put(ch, data);
		n--;
	}
}

static void fmt_put_num(fmt_put_t put, APTR data, STRPTR digits,
	LONG width, LONG left, LONG zpad)
{
	LONG len;
	LONG pad;

	len = 0;
	while (digits[len] != '\0')
		len++;
	pad = width - len;
	if (pad < 0)
		pad = 0;
	if (!left && !zpad)
		fmt_put_pad(put, data, pad, (UBYTE)' ');
	if (!left && zpad)
		fmt_put_pad(put, data, pad, (UBYTE)'0');
	fmt_puts(put, data, digits);
	if (left)
		fmt_put_pad(put, data, pad, (UBYTE)' ');
}

static void fmt_ltoa_u(ULONG v, LONG base, LONG upper, STRPTR out)
{
	char tmp[34];
	LONG i;
	LONG d;
	char *digs;
	static char digs_u[] = "0123456789ABCDEF";
	static char digs_l[] = "0123456789abcdef";

	digs = upper ? digs_u : digs_l;
	if (v == 0)
	{
		out[0] = '0';
		out[1] = '\0';
		return;
	}
	i = 0;
	while (v != 0)
	{
		d = (LONG)(v % (ULONG)base);
		tmp[i++] = digs[d];
		v = v / (ULONG)base;
	}
	while (i > 0)
	{
		i--;
		*out++ = tmp[i];
	}
	*out = '\0';
}

static void fmt_ltoa_s(LONG v, STRPTR out)
{
	ULONG u;

	if (v < 0)
	{
		*out++ = '-';
		if (v == (LONG)0x80000000)
		{
			fmt_ltoa_u(0x80000000UL, 10, 0, out);
			return;
		}
		u = (ULONG)(-v);
	}
	else
		u = (ULONG)v;
	fmt_ltoa_u(u, 10, 0, out);
}

static LONG cc_dofmt(fmt_put_t put, APTR data, STRPTR format, APTR args)
{
	STRPTR fmt;
	APTR argp;
	LONG width;
	LONG prec;
	LONG left;
	LONG zpad;
	LONG islong;
	LONG c;
	LONG n;
	LONG val;
	STRPTR sp;
	char numbuf[64];
	char fbuf[64];
	APTR math;
	DOUBLE dv;

	fmt = format;
	argp = args;
	math = cc_get_mathbase();

	while (*fmt)
	{
		if (*fmt != '%')
		{
			put((UBYTE)*fmt, data);
			fmt++;
			continue;
		}
		fmt++;
		if (*fmt == '%')
		{
			put((UBYTE)'%', data);
			fmt++;
			continue;
		}

		left = 0;
		zpad = 0;
		width = 0;
		prec = -1;
		islong = 0;

		while (*fmt == '-' || *fmt == '0' || *fmt == '+' || *fmt == ' ' || *fmt == '#')
		{
			if (*fmt == '-')
				left = 1;
			if (*fmt == '0')
				zpad = 1;
			fmt++;
		}

		if (*fmt == '*')
		{
			fmt_arg_long(&argp, &width);
			fmt++;
		}
		else
		{
			while (*fmt >= '0' && *fmt <= '9')
			{
				width = width * 10 + (LONG)(*fmt - '0');
				fmt++;
			}
		}

		if (*fmt == '.')
		{
			fmt++;
			prec = 0;
			if (*fmt == '*')
			{
				fmt_arg_long(&argp, &prec);
				fmt++;
			}
			else
			{
				while (*fmt >= '0' && *fmt <= '9')
				{
					prec = prec * 10 + (LONG)(*fmt - '0');
					fmt++;
				}
			}
		}

		if (*fmt == 'l' || *fmt == 'L')
		{
			islong = 1;
			fmt++;
		}
		else if (*fmt == 'h')
		{
			islong = 0;
			fmt++;
		}

		c = (LONG)(UBYTE)*fmt;
		if (c == 0)
			break;
		fmt++;

		if (c == (LONG)'c')
		{
			fmt_arg_long(&argp, &val);
			put((UBYTE)val, data);
			continue;
		}

		if (c == (LONG)'s')
		{
			fmt_arg_long(&argp, &val);
			sp = (STRPTR)val;
			if (sp == (STRPTR)NULL)
				sp = (STRPTR)"(null)";
			n = 0;
			while (sp[n] != '\0' && (prec < 0 || n < prec))
				n++;
			{
				LONG pad;
				LONG i;

				pad = width - n;
				if (pad < 0)
					pad = 0;
				if (!left)
					fmt_put_pad(put, data, pad, (UBYTE)' ');
				i = 0;
				while (i < n)
				{
					put((UBYTE)sp[i], data);
					i++;
				}
				if (left)
					fmt_put_pad(put, data, pad, (UBYTE)' ');
			}
			continue;
		}

		if (c == (LONG)'d' || c == (LONG)'i')
		{
			fmt_arg_long(&argp, &val);
			fmt_ltoa_s(val, numbuf);
			fmt_put_num(put, data, numbuf, width, left, zpad);
			continue;
		}

		if (c == (LONG)'u')
		{
			fmt_arg_long(&argp, &val);
			fmt_ltoa_u((ULONG)val, 10, 0, numbuf);
			fmt_put_num(put, data, numbuf, width, left, zpad);
			continue;
		}

		if (c == (LONG)'x' || c == (LONG)'X')
		{
			fmt_arg_long(&argp, &val);
			fmt_ltoa_u((ULONG)val, 16, c == (LONG)'X', numbuf);
			fmt_put_num(put, data, numbuf, width, left, zpad);
			continue;
		}

		if (c == (LONG)'o')
		{
			fmt_arg_long(&argp, &val);
			fmt_ltoa_u((ULONG)val, 8, 0, numbuf);
			fmt_put_num(put, data, numbuf, width, left, zpad);
			continue;
		}

		if (c == (LONG)'p')
		{
			fmt_arg_long(&argp, &val);
			numbuf[0] = '0';
			numbuf[1] = 'x';
			fmt_ltoa_u((ULONG)val, 16, 0, numbuf + 2);
			fmt_put_num(put, data, numbuf, width, left, 0);
			continue;
		}

		if (c == (LONG)'f' || c == (LONG)'e' || c == (LONG)'E' ||
		    c == (LONG)'g' || c == (LONG)'G')
		{
			dv = fmt_arg_double(&argp);
			(void)islong;
			if (math != (APTR)NULL)
				cc_dtoa_ieee(math, dv, fbuf, prec, c);
			else
			{
				/* No mathbase: emit 0 */
				fbuf[0] = '0';
				fbuf[1] = '\0';
			}
			fmt_put_num(put, data, fbuf, width, left, 0);
			continue;
		}

		/* Unknown conversion — emit literally */
		put((UBYTE)'%', data);
		put((UBYTE)c, data);
	}

	put((UBYTE)0, data); /* NUL for string path; file path ignores */
	return 0;
}

LONG __ASM__ __SAVE_DS__ vsprintf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	fmt_str_ctx ctx;
	cc_check_break();

	if (s == (STRPTR)NULL || format == (STRPTR)NULL)
		return -1L;

	ctx.ptr = s;
	ctx.count = 0L;
	cc_dofmt(fmt_put_str, (APTR)&ctx, format, args);
	return ctx.count;
}

/*
 * Client tagcall puts &first stack vararg in A2; body is identical to
 * vsprintf. Prefer vsprintf when the caller already has an APTR arg block.
 */
LONG __ASM__ __SAVE_DS__ sprintf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	cc_check_break();
	return vsprintf(s, format, args);
}

/*
 * C99 vsnprintf via the CClib formatter (not utility VSNPrintf — keep
 * %e/%f/%g and APTR-arg semantics identical to sprintf).
 * FuncTab uses the cc_* symbol names; client pragmas still call snprintf.
 */
/*
 * C99 vsnprintf LVO (CClib formatter, not utility VSNPrintf).
 * SFD ==varargs alias snprintf shares this bias: client tagcall/stub
 * passes a pointer to the first stack vararg in a2 (FPrintf pattern).
 */
LONG __ASM__ __SAVE_DS__ cc_vsnprintf(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG n),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	fmt_strn_ctx ctx;
	cc_check_break();

	if (format == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	if (n > 0UL && s == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	ctx.buf = s;
	ctx.size = n;
	ctx.stored = 0UL;
	ctx.count = 0L;
	cc_dofmt(fmt_put_strn, (APTR)&ctx, format, args);
	return ctx.count;
}

LONG __ASM__ __SAVE_DS__ vfprintf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	fmt_file_ctx ctx;
	cc_check_break();

	if (stream == (APTR)NULL || format == (STRPTR)NULL)
		return -1L;

	ctx.fp = (cc_FILE *)stream;
	ctx.count = 0L;
	ctx.err = 0L;
	cc_dofmt(fmt_put_file, (APTR)&ctx, format, args);
	if (ctx.err)
		return -1L;
	return ctx.count;
}

LONG __ASM__ __SAVE_DS__ fprintf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	/* Client tagcall: A2 = &first stack vararg. */
	cc_check_break();
	return vfprintf(stream, format, args);
}

LONG __ASM__ __SAVE_DS__ vprintf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args))
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL
	    || ud->stdout_p == (cc_FILE **)NULL
	    || *ud->stdout_p == (cc_FILE *)NULL)
		return -1L;
	return vfprintf((APTR)*ud->stdout_p, format, args);
}

LONG __ASM__ __SAVE_DS__ printf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args))
{
	/* Client tagcall: A1 = &first stack vararg. */
	cc_check_break();
	return vprintf(format, args);
}

/* ---- scanf family --------------------------------------------------- */

typedef struct
{
	STRPTR sp;
	cc_FILE *fp;
	LONG chars;
	LONG ungot;
} scan_ctx;

static LONG scan_getc(scan_ctx *sc)
{
	LONG c;

	if (sc->ungot >= 0L)
	{
		c = sc->ungot;
		sc->ungot = -1L;
		sc->chars++;
		return c;
	}

	if (sc->sp != (STRPTR)NULL)
	{
		c = (LONG)(UBYTE)*sc->sp;
		if (c == 0L)
			return (LONG)CC_EOF;
		sc->sp++;
		sc->chars++;
		return c;
	}

	if (sc->fp == (cc_FILE *)NULL)
		return (LONG)CC_EOF;
	c = getc((APTR)sc->fp);
	if (c != (LONG)CC_EOF)
		sc->chars++;
	return c;
}

static void scan_ungetc(scan_ctx *sc, LONG c)
{
	if (c == (LONG)CC_EOF)
		return;
	sc->ungot = c;
	if (sc->chars > 0L)
		sc->chars--;
}

static LONG scan_isspace(LONG c)
{
	if (c == (LONG)' ' || c == (LONG)'\t' || c == (LONG)'\n' ||
	    c == (LONG)'\r' || c == (LONG)'\f' || c == (LONG)'\v')
		return 1L;
	return 0L;
}

static void scan_skip_ws(scan_ctx *sc)
{
	LONG c;

	for (;;)
	{
		c = scan_getc(sc);
		if (c == (LONG)CC_EOF)
			return;
		if (!scan_isspace(c))
		{
			scan_ungetc(sc, c);
			return;
		}
	}
}

static APTR scan_arg(APTR *argp)
{
	APTR *p;
	APTR v;

	p = (APTR *)*argp;
	v = *p;
	*argp = (APTR)(p + 1);
	return v;
}

static LONG cc_doscan(scan_ctx *sc, STRPTR format, APTR args)
{
	STRPTR fmt;
	APTR argp;
	LONG assigned;
	LONG c;
	LONG fc;
	LONG width;
	LONG isshort;
	LONG suppress;
	LONG sign;
	LONG val;
	LONG digits;
	LONG base;
	LONG digv;
	char *dp;
	APTR dest;
	char nbuf[64];
	LONG ni;
	DOUBLE dv;
	APTR math;

	fmt = format;
	argp = args;
	assigned = 0L;
	math = cc_get_mathbase();

	while (*fmt)
	{
		fc = (LONG)(UBYTE)*fmt;

		if (scan_isspace(fc))
		{
			while (scan_isspace((LONG)(UBYTE)*fmt))
				fmt++;
			scan_skip_ws(sc);
			continue;
		}

		if (fc != (LONG)'%')
		{
			c = scan_getc(sc);
			if (c != fc)
			{
				scan_ungetc(sc, c);
				return assigned;
			}
			fmt++;
			continue;
		}

		fmt++;
		suppress = 0L;
		width = 0L;
		/* Amiga int is 32-bit: default %d/%u/%x store LONG; %h* stores short */
		isshort = 0L;

		if (*fmt == '*')
		{
			suppress = 1L;
			fmt++;
		}

		while (*fmt >= '0' && *fmt <= '9')
		{
			width = width * 10L + (LONG)(*fmt - '0');
			fmt++;
		}

		if (*fmt == 'l' || *fmt == 'L')
		{
			isshort = 0L;
			fmt++;
		}
		else if (*fmt == 'h')
		{
			isshort = 1L;
			fmt++;
		}

		fc = (LONG)(UBYTE)*fmt;
		if (fc == 0L)
			break;
		fmt++;

		if (fc == (LONG)'%')
		{
			c = scan_getc(sc);
			if (c != (LONG)'%')
			{
				scan_ungetc(sc, c);
				return assigned;
			}
			continue;
		}

		if (fc == (LONG)'n')
		{
			if (!suppress)
			{
				dest = scan_arg(&argp);
				if (isshort)
					*(short *)dest = (short)sc->chars;
				else
					*(LONG *)dest = sc->chars;
			}
			continue;
		}

		if (fc == (LONG)'c')
		{
			if (width <= 0L)
				width = 1L;
			if (!suppress)
				dest = scan_arg(&argp);
			else
				dest = (APTR)NULL;
			dp = (char *)dest;
			digits = 0L;
			while (digits < width)
			{
				c = scan_getc(sc);
				if (c == (LONG)CC_EOF)
					break;
				if (dp != (char *)NULL)
					*dp++ = (char)c;
				digits++;
			}
			if (digits == 0L)
				return assigned > 0L ? assigned : (LONG)CC_EOF;
			if (!suppress)
				assigned++;
			continue;
		}

		if (fc == (LONG)'s')
		{
			scan_skip_ws(sc);
			if (!suppress)
				dest = scan_arg(&argp);
			else
				dest = (APTR)NULL;
			dp = (char *)dest;
			digits = 0L;
			for (;;)
			{
				if (width > 0L && digits >= width)
					break;
				c = scan_getc(sc);
				if (c == (LONG)CC_EOF || scan_isspace(c))
				{
					scan_ungetc(sc, c);
					break;
				}
				if (dp != (char *)NULL)
					*dp++ = (char)c;
				digits++;
			}
			if (dp != (char *)NULL)
				*dp = '\0';
			if (digits == 0L)
				return assigned > 0L ? assigned : (LONG)CC_EOF;
			if (!suppress)
				assigned++;
			continue;
		}

		if (fc == (LONG)'d' || fc == (LONG)'i' || fc == (LONG)'u' ||
		    fc == (LONG)'x' || fc == (LONG)'X' || fc == (LONG)'o' ||
		    fc == (LONG)'p')
		{
			base = 10L;
			if (fc == (LONG)'o')
				base = 8L;
			else if (fc == (LONG)'x' || fc == (LONG)'X' || fc == (LONG)'p')
				base = 16L;

			scan_skip_ws(sc);
			sign = 1L;
			c = scan_getc(sc);
			if (c == (LONG)'+' || c == (LONG)'-')
			{
				if (c == (LONG)'-')
					sign = -1L;
				c = scan_getc(sc);
			}
			/* Optional 0x / 0X prefix for hex */
			if (base == 16L && c == (LONG)'0')
			{
				c = scan_getc(sc);
				if (c == (LONG)'x' || c == (LONG)'X')
					c = scan_getc(sc);
				else
				{
					scan_ungetc(sc, c);
					c = (LONG)'0';
				}
			}
			val = 0L;
			digits = 0L;
			for (;;)
			{
				if (c >= (LONG)'0' && c <= (LONG)'9')
					digv = c - (LONG)'0';
				else if (c >= (LONG)'a' && c <= (LONG)'f')
					digv = c - (LONG)'a' + 10L;
				else if (c >= (LONG)'A' && c <= (LONG)'F')
					digv = c - (LONG)'A' + 10L;
				else
					digv = -1L;
				if (digv < 0L || digv >= base)
					break;
				if (width > 0L && digits >= width)
					break;
				val = val * base + digv;
				digits++;
				c = scan_getc(sc);
			}
			scan_ungetc(sc, c);
			if (digits == 0L)
			{
				/* Matching failure vs input failure (EOF) */
				if (c == (LONG)CC_EOF)
					return assigned > 0L ? assigned : (LONG)CC_EOF;
				return assigned;
			}
			val *= sign;
			if (!suppress)
			{
				dest = scan_arg(&argp);
				if (isshort)
					*(short *)dest = (short)val;
				else
					*(LONG *)dest = val;
				assigned++;
			}
			continue;
		}

		if (fc == (LONG)'f' || fc == (LONG)'e' || fc == (LONG)'g' ||
		    fc == (LONG)'E' || fc == (LONG)'G')
		{
			scan_skip_ws(sc);
			ni = 0;
			c = scan_getc(sc);
			if (c == (LONG)'+' || c == (LONG)'-')
			{
				if (ni < 63)
					nbuf[ni++] = (char)c;
				c = scan_getc(sc);
			}
			digits = 0L;
			while (c >= (LONG)'0' && c <= (LONG)'9')
			{
				if (width > 0L && digits >= width)
					break;
				if (ni < 63)
					nbuf[ni++] = (char)c;
				digits++;
				c = scan_getc(sc);
			}
			if (c == (LONG)'.')
			{
				if (ni < 63)
					nbuf[ni++] = '.';
				digits++;
				c = scan_getc(sc);
				while (c >= (LONG)'0' && c <= (LONG)'9')
				{
					if (width > 0L && digits >= width)
						break;
					if (ni < 63)
						nbuf[ni++] = (char)c;
					digits++;
					c = scan_getc(sc);
				}
			}
			if (c == (LONG)'e' || c == (LONG)'E')
			{
				if (ni < 63)
					nbuf[ni++] = (char)c;
				digits++;
				c = scan_getc(sc);
				if (c == (LONG)'+' || c == (LONG)'-')
				{
					if (ni < 63)
						nbuf[ni++] = (char)c;
					c = scan_getc(sc);
				}
				while (c >= (LONG)'0' && c <= (LONG)'9')
				{
					if (width > 0L && digits >= width)
						break;
					if (ni < 63)
						nbuf[ni++] = (char)c;
					digits++;
					c = scan_getc(sc);
				}
			}
			scan_ungetc(sc, c);
			nbuf[ni] = '\0';
			if (digits == 0L)
			{
				if (c == (LONG)CC_EOF)
					return assigned > 0L ? assigned : (LONG)CC_EOF;
				return assigned;
			}
			if (!suppress)
			{
				dest = scan_arg(&argp);
				if (math != (APTR)NULL)
					dv = cc_atof_ieee(math, nbuf);
				else
					dv = atof(nbuf);
				*(DOUBLE *)dest = dv;
				assigned++;
			}
			continue;
		}

		return assigned;
	}

	return assigned;
}

LONG __ASM__ __SAVE_DS__ sscanf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	scan_ctx sc;
	cc_check_break();

	if (s == (STRPTR)NULL || format == (STRPTR)NULL)
		return (LONG)CC_EOF;

	sc.sp = s;
	sc.fp = (cc_FILE *)NULL;
	sc.chars = 0L;
	sc.ungot = -1L;
	return cc_doscan(&sc, format, args);
}

LONG __ASM__ __SAVE_DS__ fscanf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	scan_ctx sc;
	cc_check_break();

	if (stream == (APTR)NULL || format == (STRPTR)NULL)
		return (LONG)CC_EOF;

	sc.sp = (STRPTR)NULL;
	sc.fp = (cc_FILE *)stream;
	sc.chars = 0L;
	sc.ungot = -1L;
	return cc_doscan(&sc, format, args);
}

LONG __ASM__ __SAVE_DS__ scanf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args))
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL
	    || ud->stdin_p == (cc_FILE **)NULL
	    || *ud->stdin_p == (cc_FILE *)NULL)
		return (LONG)CC_EOF;
	return fscanf((APTR)*ud->stdin_p, format, args);
}

LONG __ASM__ __SAVE_DS__ vscanf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args))
{
	cc_check_break();
	return scanf(format, args);
}

LONG __ASM__ __SAVE_DS__ vfscanf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	cc_check_break();
	return fscanf(stream, format, args);
}

LONG __ASM__ __SAVE_DS__ vsscanf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args))
{
	cc_check_break();
	return sscanf(s, format, args);
}
