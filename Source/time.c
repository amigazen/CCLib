/*
 * time.c - time and date functions for cclib.library
 *
 * Epoch is 1978-01-01 (Amiga), not Unix 1970. Amiga2Date/Date2Amiga from
 * utility.library convert ClockData <-> seconds since that epoch.
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <utility/date.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include "private/cc_internal.h"
#include "private/cc_ieee.h"
#include "cclib_funcs.h"

static const char *cc_wday_abbr[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char *cc_wday_full[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday",
	"Thursday", "Friday", "Saturday"
};
static const char *cc_mon_abbr[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char *cc_mon_full[] = {
	"January", "February", "March", "April", "May", "June",
	"July", "August", "September", "October", "November", "December"
};

static const short cc_mon_days[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

static LONG cc_isleap(LONG year)
{
	/* year is full Gregorian year */
	if ((year % 4L) != 0L)
		return 0L;
	if ((year % 100L) != 0L)
		return 1L;
	if ((year % 400L) == 0L)
		return 1L;
	return 0L;
}

/* Seconds since 1978-01-01 from a DateStamp */
static LONG cc_ds_to_secs(struct DateStamp *ds)
{
	LONG secs;

	secs = ds->ds_Days * 86400L;
	secs += ds->ds_Minute * 60L;
	secs += ds->ds_Tick / 50L;
	return secs;
}

/* Fill ud->tm from Amiga-epoch seconds */
static void cc_secs_to_tm(LONG secs, cc_tm *tm)
{
	struct ClockData cd;

	Amiga2Date((ULONG)secs, &cd);

	tm->tm_sec = (short)cd.sec;
	tm->tm_min = (short)cd.min;
	tm->tm_hour = (short)cd.hour;
	tm->tm_mday = (short)cd.mday;
	tm->tm_mon = (short)(cd.month - 1);          /* 0..11 */
	tm->tm_year = (short)(cd.year - 1900);        /* years since 1900 */
	tm->tm_wday = (short)cd.wday;
	tm->tm_yday = 0;
	tm->tm_isdst = 0;
	tm->tm_hsec = 0;

	/* Compute day-of-year */
	if (cd.month >= 1 && cd.month <= 12)
	{
		tm->tm_yday = (short)(cc_mon_days[cd.month - 1] + cd.mday - 1);
		if (cd.month > 2 && cc_isleap((LONG)cd.year))
			tm->tm_yday++;
	}
}

LONG __ASM__ __SAVE_DS__ time(
	__REG__(a0, APTR tloc))
{
	struct DateStamp ds;
	LONG secs;
	cc_check_break();

	DateStamp(&ds);
	secs = cc_ds_to_secs(&ds);
	if (tloc != NULL)
		*(LONG *)tloc = secs;
	return secs;
}

LONG __ASM__ __SAVE_DS__ clock(void)
{
	cc_task_UserData *ud;
	struct DateStamp ds;
	LONG now;
	cc_check_break();

	ud = cc_get_ud();
	DateStamp(&ds);
	now = cc_ds_to_secs(&ds);

	if (ud == NULL)
		return now;
	return now - ud->start;
}

APTR __ASM__ __SAVE_DS__ gmtime(
	__REG__(a0, APTR timer))
{
	cc_task_UserData *ud;
	LONG secs;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == NULL)
		return NULL;
	if (timer == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (APTR)NULL;
	}
	if (!cc_buf_mapped(timer, sizeof(LONG)))
		return (APTR)NULL;

	secs = *(LONG *)timer;
	cc_secs_to_tm(secs, &ud->tm);
	return (APTR)&ud->tm;
}

APTR __ASM__ __SAVE_DS__ localtime(
	__REG__(a0, APTR timer))
{
	/* Amiga has no timezone database - identical to gmtime */
	cc_check_break();

	return gmtime(timer);
}

STRPTR __ASM__ __SAVE_DS__ asctime(
	__REG__(a0, APTR tm))
{
	cc_task_UserData *ud;
	cc_tm *t;
	LONG year;
	LONG wday;
	LONG mon;
	APTR args[7];
	cc_check_break();

	ud = cc_get_ud();
	t = (cc_tm *)tm;
	if (ud == NULL)
		return (STRPTR)NULL;
	if (t == (cc_tm *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}
	if (!cc_buf_mapped((APTR)t, sizeof(cc_tm)))
		return (STRPTR)NULL;

	wday = t->tm_wday;
	mon = t->tm_mon;
	if (wday < 0 || wday > 6)
		wday = 0;
	if (mon < 0 || mon > 11)
		mon = 0;
	year = (LONG)t->tm_year + 1900L;

	/* "Sun Jan 1 12:00:00 1989\n" - APTR arg block via vsprintf (not tagcall) */
	args[0] = (APTR)cc_wday_abbr[wday];
	args[1] = (APTR)cc_mon_abbr[mon];
	args[2] = (APTR)(LONG)t->tm_mday;
	args[3] = (APTR)(LONG)t->tm_hour;
	args[4] = (APTR)(LONG)t->tm_min;
	args[5] = (APTR)(LONG)t->tm_sec;
	args[6] = (APTR)year;
	vsprintf(ud->buffer, (STRPTR)"%s %s %2ld %02ld:%02ld:%02ld %ld\n", (APTR)args);
	return ud->buffer;
}

STRPTR __ASM__ __SAVE_DS__ ctime(
	__REG__(a0, APTR timer))
{
	APTR tm;
	cc_check_break();

	tm = localtime(timer);
	if (tm == NULL)
		return NULL;
	return asctime(tm);
}

LONG __ASM__ __SAVE_DS__ mktime(
	__REG__(a0, APTR tm))
{
	cc_tm *t;
	struct ClockData cd;
	ULONG secs;
	LONG year;
	LONG yday;
	LONG mon;
	cc_check_break();

	t = (cc_tm *)tm;
	if (t == (cc_tm *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	if (!cc_buf_mapped((APTR)t, sizeof(cc_tm)))
		return -1L;

	year = (LONG)t->tm_year + 1900L;

	/*
	 * Historic CClib builds from tm_year, tm_yday, tm_hour, tm_min, tm_sec.
	 * If tm_yday is unset (0) but mday/mon are present, derive yday.
	 */
	yday = (LONG)t->tm_yday;
	mon = (LONG)t->tm_mon;
	if (yday == 0L && t->tm_mday > 0 && mon >= 0 && mon <= 11)
	{
		yday = (LONG)cc_mon_days[mon] + (LONG)t->tm_mday - 1L;
		if (mon >= 2 && cc_isleap(year))
			yday++;
	}

	cd.sec = (UWORD)t->tm_sec;
	cd.min = (UWORD)t->tm_min;
	cd.hour = (UWORD)t->tm_hour;
	cd.mday = (UWORD)t->tm_mday;
	cd.month = (UWORD)(mon + 1);
	cd.year = (UWORD)year;
	cd.wday = 0;

	/* Prefer Date2Amiga when month/mday are valid */
	if (cd.month >= 1 && cd.month <= 12 && cd.mday >= 1)
	{
		secs = Date2Amiga(&cd);
		return (LONG)secs;
	}

	/* Fallback: days since 1978-01-01 from year + yday */
	{
		LONG days;
		LONG y;

		days = 0L;
		for (y = 1978L; y < year; y++)
			days += cc_isleap(y) ? 366L : 365L;
		days += yday;
		secs = (ULONG)(days * 86400L + (LONG)t->tm_hour * 3600L +
			(LONG)t->tm_min * 60L + (LONG)t->tm_sec);
		return (LONG)secs;
	}
}

DOUBLE __ASM__ __SAVE_DS__ difftime(
	__REG__(d0, LONG time1),
	__REG__(d1, LONG time0))
{
	LONG delta;
	APTR base;
	cc_check_break();

	delta = time1 - time0;
	base = cc_get_mathbase();
	if (base != (APTR)NULL)
		return cc_ieeedp_flt(base, delta);
	/* No mathbase (non-CRT): return +0.0 - callers should SetupSTDIO. */
	return cc_ieeedp_zero();
}

/* Append one character to strftime output if room remains */
static LONG sf_putc(STRPTR s, ULONG *pos, ULONG maxsize, char ch)
{
	if (*pos + 1UL >= maxsize)
		return 0L;
	s[*pos] = ch;
	(*pos)++;
	return 1L;
}

static LONG sf_puts(STRPTR s, ULONG *pos, ULONG maxsize, const char *src)
{
	while (*src)
	{
		if (!sf_putc(s, pos, maxsize, *src))
			return 0L;
		src++;
	}
	return 1L;
}

static LONG sf_putnum(STRPTR s, ULONG *pos, ULONG maxsize, LONG val, LONG width)
{
	char buf[16];
	LONG i;
	LONG n;
	LONG v;
	LONG neg;

	neg = 0L;
	v = val;
	if (v < 0L)
	{
		neg = 1L;
		v = -v;
	}

	i = 0L;
	if (v == 0L)
		buf[i++] = '0';
	else
	{
		while (v > 0L && i < 15L)
		{
			buf[i++] = (char)('0' + (v % 10L));
			v /= 10L;
		}
	}

	n = i;
	if (neg)
		n++;
	while (n < width)
	{
		if (!sf_putc(s, pos, maxsize, '0'))
			return 0L;
		n++;
	}
	if (neg)
	{
		if (!sf_putc(s, pos, maxsize, '-'))
			return 0L;
	}
	while (i > 0L)
	{
		i--;
		if (!sf_putc(s, pos, maxsize, buf[i]))
			return 0L;
	}
	return 1L;
}

ULONG __ASM__ __SAVE_DS__ strftime(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG maxsize),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR tm))
{
	cc_tm *t;
	ULONG pos;
	STRPTR fmt;
	LONG wday;
	LONG mon;
	LONG year;
	LONG hour12;
	char ch;
	cc_task_UserData *ud;
	ULONG loc_len;
	cc_check_break();

	t = (cc_tm *)tm;
	if (s == (STRPTR)NULL || format == (STRPTR)NULL
	    || t == (cc_tm *)NULL || maxsize == 0UL)
	{
		cc_set_errno(CC_EINVAL);
		return 0UL;
	}
	if (!cc_buf_mapped((APTR)s, maxsize) || !cc_str_mapped(format)
	    || !cc_buf_mapped((APTR)t, sizeof(cc_tm)))
		return 0UL;

	/*
	 * Non-"C" LC_TIME: locale.library/FormatDate (localized names and
	 * preferred %c/%x/%X templates).  Fall back to Pure C on failure.
	 */
	ud = cc_get_ud();
	if (cc_strftime_locale(ud, s, maxsize, format, tm, &loc_len))
		return loc_len;

	pos = 0UL;
	fmt = format;
	wday = t->tm_wday;
	mon = t->tm_mon;
	year = (LONG)t->tm_year + 1900L;
	if (wday < 0 || wday > 6)
		wday = 0;
	if (mon < 0 || mon > 11)
		mon = 0;

	while (*fmt)
	{
		if (*fmt != '%')
		{
			if (!sf_putc(s, &pos, maxsize, *fmt))
				return 0UL;
			fmt++;
			continue;
		}
		fmt++;
		ch = *fmt;
		if (ch == '\0')
			break;
		fmt++;

		switch (ch)
		{
		case 'a':
			if (!sf_puts(s, &pos, maxsize, cc_wday_abbr[wday]))
				return 0UL;
			break;
		case 'A':
			if (!sf_puts(s, &pos, maxsize, cc_wday_full[wday]))
				return 0UL;
			break;
		case 'b':
		case 'h':
			if (!sf_puts(s, &pos, maxsize, cc_mon_abbr[mon]))
				return 0UL;
			break;
		case 'B':
			if (!sf_puts(s, &pos, maxsize, cc_mon_full[mon]))
				return 0UL;
			break;
		case 'c':
			if (!sf_puts(s, &pos, maxsize, cc_wday_abbr[wday]))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ' '))
				return 0UL;
			if (!sf_puts(s, &pos, maxsize, cc_mon_abbr[mon]))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ' '))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ' '))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_hour, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_min, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_sec, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ' '))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, year, 4L))
				return 0UL;
			break;
		case 'C':
			if (!sf_putnum(s, &pos, maxsize, year / 100L, 2L))
				return 0UL;
			break;
		case 'd':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 2L))
				return 0UL;
			break;
		case 'D':
			/* %m/%d/%y */
			if (!sf_putnum(s, &pos, maxsize, (LONG)mon + 1L, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, '/'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, '/'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, year % 100L, 2L))
				return 0UL;
			break;
		case 'e':
			/* day with leading space ( 1-31) */
			if (t->tm_mday < 10)
			{
				if (!sf_putc(s, &pos, maxsize, ' '))
					return 0UL;
				if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 1L))
					return 0UL;
			}
			else if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 2L))
				return 0UL;
			break;
		case 'F':
			/* ISO 8601 %Y-%m-%d */
			if (!sf_putnum(s, &pos, maxsize, year, 4L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, '-'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)mon + 1L, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, '-'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 2L))
				return 0UL;
			break;
		case 'H':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_hour, 2L))
				return 0UL;
			break;
		case 'I':
			hour12 = (LONG)t->tm_hour % 12L;
			if (hour12 == 0L)
				hour12 = 12L;
			if (!sf_putnum(s, &pos, maxsize, hour12, 2L))
				return 0UL;
			break;
		case 'j':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_yday + 1L, 3L))
				return 0UL;
			break;
		case 'm':
			if (!sf_putnum(s, &pos, maxsize, (LONG)mon + 1L, 2L))
				return 0UL;
			break;
		case 'M':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_min, 2L))
				return 0UL;
			break;
		case 'n':
			if (!sf_putc(s, &pos, maxsize, '\n'))
				return 0UL;
			break;
		case 'p':
			if (!sf_puts(s, &pos, maxsize, (t->tm_hour >= 12) ? "PM" : "AM"))
				return 0UL;
			break;
		case 'r':
			/* %I:%M:%S %p */
			hour12 = (LONG)t->tm_hour % 12L;
			if (hour12 == 0L)
				hour12 = 12L;
			if (!sf_putnum(s, &pos, maxsize, hour12, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_min, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_sec, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ' '))
				return 0UL;
			if (!sf_puts(s, &pos, maxsize, (t->tm_hour >= 12) ? "PM" : "AM"))
				return 0UL;
			break;
		case 'R':
			/* %H:%M */
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_hour, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_min, 2L))
				return 0UL;
			break;
		case 'S':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_sec, 2L))
				return 0UL;
			break;
		case 't':
			if (!sf_putc(s, &pos, maxsize, '\t'))
				return 0UL;
			break;
		case 'T':
			/* %H:%M:%S */
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_hour, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_min, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_sec, 2L))
				return 0UL;
			break;
		case 'U':
		case 'W':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_yday / 7L, 2L))
				return 0UL;
			break;
		case 'w':
			if (!sf_putnum(s, &pos, maxsize, (LONG)wday, 1L))
				return 0UL;
			break;
		case 'x':
			if (!sf_putnum(s, &pos, maxsize, (LONG)mon + 1L, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, '/'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_mday, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, '/'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, year % 100L, 2L))
				return 0UL;
			break;
		case 'X':
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_hour, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_min, 2L))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ':'))
				return 0UL;
			if (!sf_putnum(s, &pos, maxsize, (LONG)t->tm_sec, 2L))
				return 0UL;
			break;
		case 'y':
			if (!sf_putnum(s, &pos, maxsize, year % 100L, 2L))
				return 0UL;
			break;
		case 'Y':
			if (!sf_putnum(s, &pos, maxsize, year, 4L))
				return 0UL;
			break;
		case 'Z':
			if (!sf_puts(s, &pos, maxsize, "Local"))
				return 0UL;
			break;
		case '%':
			if (!sf_putc(s, &pos, maxsize, '%'))
				return 0UL;
			break;
		default:
			if (!sf_putc(s, &pos, maxsize, '%'))
				return 0UL;
			if (!sf_putc(s, &pos, maxsize, ch))
				return 0UL;
			break;
		}
	}

	if (pos >= maxsize)
		return 0UL;
	s[pos] = '\0';
	return pos;
}

/* ---- strptime ------------------------------------------------------- */

static LONG sp_isdigit(LONG c)
{
	return (c >= '0' && c <= '9') ? 1L : 0L;
}

static LONG sp_getnum(STRPTR *pp, LONG width, LONG *out)
{
	STRPTR p;
	LONG n;
	LONG d;
	LONG got;

	p = *pp;
	n = 0L;
	got = 0L;
	while (width > 0L && sp_isdigit((LONG)(UBYTE)*p))
	{
		d = (LONG)(*p - '0');
		n = n * 10L + d;
		p++;
		width--;
		got = 1L;
	}
	if (!got)
		return 0L;
	*out = n;
	*pp = p;
	return 1L;
}

static LONG sp_match_str(STRPTR *pp, const char **tab, LONG ntab, LONG *out)
{
	LONG i;
	LONG j;
	STRPTR p;

	i = 0L;
	while (i < ntab)
	{
		p = *pp;
		j = 0L;
		while (tab[i][j] != '\0' && p[j] != '\0')
		{
			if (cc_tolower_ch((cc_task_UserData *)NULL, (LONG)(UBYTE)p[j])
			    != cc_tolower_ch((cc_task_UserData *)NULL, (LONG)(UBYTE)tab[i][j]))
				break;
			j++;
		}
		if (tab[i][j] == '\0')
		{
			*pp = p + j;
			*out = i;
			return 1L;
		}
		i++;
	}
	return 0L;
}

STRPTR __ASM__ __SAVE_DS__ strptime(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR tm))
{
	cc_tm *t;
	STRPTR p;
	STRPTR fmt;
	LONG v;
	char ch;
	cc_check_break();

	t = (cc_tm *)tm;
	if (s == (STRPTR)NULL || format == (STRPTR)NULL || t == (cc_tm *)NULL)
		return (STRPTR)NULL;

	p = s;
	fmt = format;
	while (*fmt)
	{
		if (*fmt != '%')
		{
			if (*fmt == ' ')
			{
				while (*p == ' ' || *p == '\t')
					p++;
				fmt++;
				continue;
			}
			if (*p != *fmt)
				return (STRPTR)NULL;
			p++;
			fmt++;
			continue;
		}
		fmt++;
		ch = *fmt;
		if (ch == '\0')
			break;
		fmt++;

		switch (ch)
		{
		case 'Y':
			if (!sp_getnum(&p, 4L, &v))
				return (STRPTR)NULL;
			t->tm_year = (short)(v - 1900L);
			break;
		case 'y':
			if (!sp_getnum(&p, 2L, &v))
				return (STRPTR)NULL;
			if (v < 69L)
				v += 100L;
			t->tm_year = (short)v;
			break;
		case 'm':
			if (!sp_getnum(&p, 2L, &v))
				return (STRPTR)NULL;
			t->tm_mon = (short)(v - 1L);
			break;
		case 'd':
			if (!sp_getnum(&p, 2L, &v))
				return (STRPTR)NULL;
			t->tm_mday = (short)v;
			break;
		case 'H':
			if (!sp_getnum(&p, 2L, &v))
				return (STRPTR)NULL;
			t->tm_hour = (short)v;
			break;
		case 'M':
			if (!sp_getnum(&p, 2L, &v))
				return (STRPTR)NULL;
			t->tm_min = (short)v;
			break;
		case 'S':
			if (!sp_getnum(&p, 2L, &v))
				return (STRPTR)NULL;
			t->tm_sec = (short)v;
			break;
		case 'b':
		case 'B':
			if (!sp_match_str(&p, cc_mon_abbr, 12L, &v)
			    && !sp_match_str(&p, cc_mon_full, 12L, &v))
				return (STRPTR)NULL;
			t->tm_mon = (short)v;
			break;
		case 'a':
		case 'A':
			if (!sp_match_str(&p, cc_wday_abbr, 7L, &v)
			    && !sp_match_str(&p, cc_wday_full, 7L, &v))
				return (STRPTR)NULL;
			t->tm_wday = (short)v;
			break;
		case '%':
			if (*p != '%')
				return (STRPTR)NULL;
			p++;
			break;
		default:
			return (STRPTR)NULL;
		}
	}
	return p;
}
