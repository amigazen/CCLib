/* SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2026
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED.
 *
 * string.c - string LVOs for cclib.library
 *
 * Target: latest AmigaOS 3.x utility.library (V47+, opened in LibInit).
 * Prefer OS APIs per LIBC_TO_AMIGA.md and
 * https://developer.amigaos3.net/autodocs/utility.library/
 *
 *   Stricmp  -> strcasecmp
 *   ToUpper  -> strupr (and toupper in conv.c)
 *   ToLower  -> tolower in conv.c
 *   Strncpy  -> strcpy / strcat (unbounded C copies; size = ULONG_MAX)
 *   Strnicmp -> no CClib LVO (would map to strncasecmp, not strncmp)
 *
 * Strncpy/Strncat take a destination *buffer size*, not C's "max chars
 * from src", so strncpy/strncat LVOs stay Pure C to keep C/CClib ABI.
 * Search helpers (strchr, strstr, ...) have no utility equivalent.
 * Memory copies live in mem.c (exec CopyMem / memmove when overlapping).
 */

#include <proto/exec.h>
#include <proto/utility.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* ---- shared Pure C helpers (one implementation for aliases) --------- */

static STRPTR cc_strchr(STRPTR s, LONG c)
{
	UBYTE *p;
	UBYTE ch;

	p = (UBYTE *)s;
	ch = (UBYTE)c;
	if (p == (UBYTE *)NULL)
		return (STRPTR)NULL;
	for (;;)
	{
		if (*p == ch)
			return (STRPTR)p;
		if (*p == 0)
			return (STRPTR)NULL;
		p++;
	}
}

static STRPTR cc_strrchr(STRPTR s, LONG c)
{
	UBYTE *p;
	UBYTE *last;
	UBYTE ch;

	p = (UBYTE *)s;
	ch = (UBYTE)c;
	last = (UBYTE *)NULL;
	if (p == (UBYTE *)NULL)
		return (STRPTR)NULL;
	for (;;)
	{
		if (*p == ch)
			last = p;
		if (*p == 0)
			return (STRPTR)last;
		p++;
	}
}

/* ---- LVOs ----------------------------------------------------------- */

STRPTR __ASM__ __SAVE_DS__ index(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c))
{
	/* BSD name; same as strchr. */
	return cc_strchr(s, c);
}

STRPTR __ASM__ __SAVE_DS__ rindex(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c))
{
	/* BSD name; same as strrchr. */
	return cc_strrchr(s, c);
}

STRPTR __ASM__ __SAVE_DS__ strupr(
	__REG__(a0, STRPTR s))
{
	UBYTE *p;
	cc_task_UserData *ud;

	p = (UBYTE *)s;
	if (p == (UBYTE *)NULL)
		return s;

	/* Case fold via cc_toupper_ch (utility today; locale.library later). */
	ud = cc_get_ud();
	while (*p)
	{
		*p = (UBYTE)cc_toupper_ch(ud, (LONG)*p);
		p++;
	}
	return s;
}

STRPTR __ASM__ __SAVE_DS__ stpcrlf(
	__REG__(a0, STRPTR s))
{
	UBYTE *p;

	/* CClib extension: truncate at first CR or LF (Pure C). */
	p = (UBYTE *)s;
	if (p == (UBYTE *)NULL)
		return s;
	while (*p)
	{
		if (*p == (UBYTE)'\r' || *p == (UBYTE)'\n')
		{
			*p = 0;
			break;
		}
		p++;
	}
	return s;
}

STRPTR __ASM__ __SAVE_DS__ strcat(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src))
{
	UBYTE *d;

	/* Append via Strncpy at the existing NUL (same unlimited assumption). */
	d = (UBYTE *)dst;
	if (d == (UBYTE *)NULL)
		return dst;
	while (*d)
		d++;
	(void)Strncpy(d, (UBYTE *)src, (ULONG)~0UL);
	return dst;
}

STRPTR __ASM__ __SAVE_DS__ strchr(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c))
{
	return cc_strchr(s, c);
}

LONG __ASM__ __SAVE_DS__ strcmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2))
{
	UBYTE *p1;
	UBYTE *p2;

	/* Pure C (case-sensitive; Stricmp is case-insensitive only). */
	p1 = (UBYTE *)s1;
	p2 = (UBYTE *)s2;
	if (p1 == p2)
		return 0;
	if (p1 == (UBYTE *)NULL)
		return -1;
	if (p2 == (UBYTE *)NULL)
		return 1;
	while (*p1 && *p1 == *p2)
	{
		p1++;
		p2++;
	}
	return (LONG)*p1 - (LONG)*p2;
}

STRPTR __ASM__ __SAVE_DS__ strcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src))
{
	/*
	 * utility.library/Strncpy (V47): length-limited copy.  C strcpy has
	 * no buffer size, so pass ULONG_MAX (caller must supply room).
	 * Return dst for C semantics (Strncpy returns end-of-string or NULL).
	 */
	(void)Strncpy((UBYTE *)dst, (UBYTE *)src, (ULONG)~0UL);
	return dst;
}

LONG __ASM__ __SAVE_DS__ strcspn(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR reject))
{
	UBYTE *p;
	UBYTE *r;
	LONG n;
	UBYTE found;

	p = (UBYTE *)s;
	n = 0;
	if (p == (UBYTE *)NULL)
		return 0;
	while (*p)
	{
		found = 0;
		r = (UBYTE *)reject;
		if (r != (UBYTE *)NULL)
		{
			while (*r)
			{
				if (*p == *r)
				{
					found = 1;
					break;
				}
				r++;
			}
		}
		if (found)
			break;
		n++;
		p++;
	}
	return n;
}

LONG __ASM__ __SAVE_DS__ strlen(
	__REG__(a0, STRPTR s))
{
	UBYTE *p;
	LONG n;

	/* Pure C — no strlen in AmigaOS 3.x utility.library. */
	p = (UBYTE *)s;
	n = 0;
	if (p == (UBYTE *)NULL)
		return 0;
	while (*p++)
		n++;
	return n;
}

STRPTR __ASM__ __SAVE_DS__ strncat(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, LONG n))
{
	UBYTE *d;
	UBYTE *s;
	LONG i;

	/*
	 * Pure C.  utility Strncat (V47) takes the full destination buffer
	 * size, not C's "append at most n chars from src".
	 * See https://developer.amigaos3.net/autodocs/utility.library/Strncat.html
	 */
	d = (UBYTE *)dst;
	s = (UBYTE *)src;
	while (*d)
		d++;
	i = 0;
	while (i < n && s != (UBYTE *)NULL && s[i])
	{
		d[i] = s[i];
		i++;
	}
	d[i] = 0;
	return dst;
}

LONG __ASM__ __SAVE_DS__ strcasecmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2))
{
	/* Case-insensitive compare via helper (Stricmp / locale ctype later). */
	return cc_str_casecmp(cc_get_ud(), s1, s2);
}

LONG __ASM__ __SAVE_DS__ strncasecmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2),
	__REG__(d0, LONG n))
{
	return cc_str_ncasecmp(cc_get_ud(), s1, s2, n);
}

STRPTR __ASM__ __SAVE_DS__ strdup(
	__REG__(a0, STRPTR s))
{
	LONG len;
	STRPTR d;

	if (s == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}
	len = strlen(s);
	d = (STRPTR)malloc((ULONG)(len + 1L));
	if (d == (STRPTR)NULL)
		return (STRPTR)NULL;
	(void)strcpy(d, s);
	return d;
}

STRPTR __ASM__ __SAVE_DS__ strndup(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG n))
{
	LONG i;
	LONG len;
	STRPTR d;

	if (s == (STRPTR)NULL || n < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}
	len = 0L;
	while (len < n && s[len] != '\0')
		len++;
	d = (STRPTR)malloc((ULONG)(len + 1L));
	if (d == (STRPTR)NULL)
		return (STRPTR)NULL;
	i = 0L;
	while (i < len)
	{
		d[i] = s[i];
		i++;
	}
	d[len] = '\0';
	return d;
}

LONG __ASM__ __SAVE_DS__ strnlen(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG maxlen))
{
	LONG i;

	if (s == (STRPTR)NULL || maxlen <= 0L)
		return 0L;
	i = 0L;
	while (i < maxlen && s[i] != '\0')
		i++;
	return i;
}

STRPTR __ASM__ __SAVE_DS__ stpcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src))
{
	UBYTE *d;
	UBYTE *s;

	/* Like strcpy but returns pointer to the terminating NUL. */
	if (dst == (STRPTR)NULL || src == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}
	d = (UBYTE *)dst;
	s = (UBYTE *)src;
	while (*s != 0)
	{
		*d = *s;
		d++;
		s++;
	}
	*d = 0;
	return (STRPTR)d;
}

STRPTR __ASM__ __SAVE_DS__ stpncpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, LONG n))
{
	UBYTE *d;
	UBYTE *s;
	LONG i;

	/*
	 * Copy at most n bytes; pad with NULs like strncpy when src is
	 * short.  Return pointer to the last written byte (NUL if padded
	 * or if a terminator was stored within n).
	 */
	if (dst == (STRPTR)NULL || src == (STRPTR)NULL || n < 0L)
	{
		cc_set_errno(CC_EINVAL);
		return (STRPTR)NULL;
	}
	d = (UBYTE *)dst;
	s = (UBYTE *)src;
	i = 0L;
	while (i < n && s[i] != 0)
	{
		d[i] = s[i];
		i++;
	}
	while (i < n)
	{
		d[i] = 0;
		i++;
	}
	if (n <= 0L)
		return dst;
	/* Point at the last byte written (index n-1). */
	return (STRPTR)(d + (n - 1L));
}

LONG __ASM__ __SAVE_DS__ strncmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2),
	__REG__(d0, LONG n))
{
	UBYTE *p1;
	UBYTE *p2;
	LONG i;

	/*
	 * Pure C for case-sensitive compare.  utility Strnicmp is
	 * case-insensitive (maps to strncasecmp, not strncmp).
	 */
	p1 = (UBYTE *)s1;
	p2 = (UBYTE *)s2;
	if (n <= 0 || p1 == p2)
		return 0;
	if (p1 == (UBYTE *)NULL)
		return -1;
	if (p2 == (UBYTE *)NULL)
		return 1;
	for (i = 0; i < n; i++)
	{
		if (p1[i] != p2[i])
			return (LONG)p1[i] - (LONG)p2[i];
		if (p1[i] == 0)
			return 0;
	}
	return 0;
}

STRPTR __ASM__ __SAVE_DS__ strncpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, LONG n))
{
	UBYTE *d;
	UBYTE *s;
	LONG i;

	/*
	 * Pure C (ANSI strncpy + CClib n==0).  Do not call utility Strncpy:
	 * that API's third argument is destination buffer size and always
	 * NUL-terminates inside the buffer; C strncpy copies exactly n bytes
	 * (no forced NUL when src is longer) and zero-pads when src is shorter.
	 * See https://developer.amigaos3.net/autodocs/utility.library/Strncpy.html
	 */
	d = (UBYTE *)dst;
	s = (UBYTE *)src;
	if (n <= 0)
	{
		/* Historic CClib: n==0 still writes a terminating NUL. */
		if (n == 0 && d != (UBYTE *)NULL)
			d[0] = 0;
		return dst;
	}
	i = 0;
	while (i < n && s != (UBYTE *)NULL && s[i])
	{
		d[i] = s[i];
		i++;
	}
	while (i < n)
	{
		d[i] = 0;
		i++;
	}
	return dst;
}

STRPTR __ASM__ __SAVE_DS__ strpbrk(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR accept))
{
	UBYTE *p;
	UBYTE *a;

	p = (UBYTE *)s;
	if (p == (UBYTE *)NULL || accept == (STRPTR)NULL)
		return (STRPTR)NULL;
	while (*p)
	{
		a = (UBYTE *)accept;
		while (*a)
		{
			if (*p == *a)
				return (STRPTR)p;
			a++;
		}
		p++;
	}
	return (STRPTR)NULL;
}

STRPTR __ASM__ __SAVE_DS__ strrchr(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c))
{
	return cc_strrchr(s, c);
}

LONG __ASM__ __SAVE_DS__ strspn(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR accept))
{
	UBYTE *p;
	UBYTE *a;
	LONG n;
	UBYTE found;

	p = (UBYTE *)s;
	n = 0;
	if (p == (UBYTE *)NULL)
		return 0;
	while (*p)
	{
		found = 0;
		a = (UBYTE *)accept;
		if (a != (UBYTE *)NULL)
		{
			while (*a)
			{
				if (*p == *a)
				{
					found = 1;
					break;
				}
				a++;
			}
		}
		if (!found)
			break;
		n++;
		p++;
	}
	return n;
}

STRPTR __ASM__ __SAVE_DS__ strstr(
	__REG__(a0, STRPTR haystack),
	__REG__(a1, STRPTR needle))
{
	UBYTE *h;
	UBYTE *n;
	UBYTE *p;
	UBYTE *q;

	h = (UBYTE *)haystack;
	n = (UBYTE *)needle;
	if (h == (UBYTE *)NULL)
		return (STRPTR)NULL;
	if (n == (UBYTE *)NULL || *n == 0)
		return haystack;
	while (*h)
	{
		p = h;
		q = n;
		while (*p && *q && *p == *q)
		{
			p++;
			q++;
		}
		if (*q == 0)
			return (STRPTR)h;
		h++;
	}
	return (STRPTR)NULL;
}

STRPTR __ASM__ __SAVE_DS__ strtok(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR delim))
{
	cc_task_UserData *ud;
	UBYTE *p;
	UBYTE *start;
	UBYTE *d;
	UBYTE found;

	/* Pure C; continuation pointer lives in per-task task_UserData. */
	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return (STRPTR)NULL;

	if (s != (STRPTR)NULL)
		ud->scanpoint = (char *)s;
	p = (UBYTE *)ud->scanpoint;
	if (p == (UBYTE *)NULL || *p == 0)
		return (STRPTR)NULL;

	while (*p)
	{
		found = 0;
		d = (UBYTE *)delim;
		if (d != (UBYTE *)NULL)
		{
			while (*d)
			{
				if (*p == *d)
				{
					found = 1;
					break;
				}
				d++;
			}
		}
		if (!found)
			break;
		p++;
	}
	if (*p == 0)
	{
		ud->scanpoint = (char *)p;
		return (STRPTR)NULL;
	}

	start = p;
	while (*p)
	{
		found = 0;
		d = (UBYTE *)delim;
		if (d != (UBYTE *)NULL)
		{
			while (*d)
			{
				if (*p == *d)
				{
					found = 1;
					break;
				}
				d++;
			}
		}
		if (found)
		{
			*p = 0;
			ud->scanpoint = (char *)(p + 1);
			return (STRPTR)start;
		}
		p++;
	}
	ud->scanpoint = (char *)p;
	return (STRPTR)start;
}

STRPTR __ASM__ __SAVE_DS__ strrv(
	__REG__(a0, STRPTR s))
{
	UBYTE *left;
	UBYTE *right;
	UBYTE tmp;

	/* CClib extension (Pure C). */
	if (s == (STRPTR)NULL || *s == 0)
		return s;
	left = (UBYTE *)s;
	right = left;
	while (right[1])
		right++;
	while (left < right)
	{
		tmp = *left;
		*left = *right;
		*right = tmp;
		left++;
		right--;
	}
	return s;
}

STRPTR __ASM__ __SAVE_DS__ strnrv(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG n))
{
	UBYTE *left;
	UBYTE *right;
	UBYTE tmp;
	LONG len;

	/* CClib extension: reverse first n bytes (Pure C). */
	if (s == (STRPTR)NULL || n <= 0)
		return s;
	len = 0;
	while (s[len])
		len++;
	if (n > len)
		n = len;
	if (n <= 1)
		return s;
	left = (UBYTE *)s;
	right = left + n - 1;
	while (left < right)
	{
		tmp = *left;
		*left = *right;
		*right = tmp;
		left++;
		right--;
	}
	return s;
}

STRPTR __ASM__ __SAVE_DS__ bcpl_strcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, BPTR src))
{
	UBYTE *d;
	UBYTE *b;
	UBYTE len;
	UBYTE i;

	/* BCPL BSTR (BPTR) to C string via exec BADDR (Pure C over OS types). */
	d = (UBYTE *)dst;
	if (d == (UBYTE *)NULL)
		return dst;
	if (src == (BPTR)NULL)
	{
		*d = 0;
		return dst;
	}
	b = (UBYTE *)BADDR(src);
	len = b[0];
	for (i = 0; i < len; i++)
		d[i] = b[i + 1];
	d[len] = 0;
	return dst;
}
