/*
 * wchar.c - multibyte / wchar foundation (MB_CUR_MAX==1) and wctype LVOs.
 *
 * Single-byte identity conversion for the "C" / 8-bit codeset model.
 * Values outside 0..255 are encoding errors (EILSEQ).  isw* / tow* map
 * through the existing ctype/locale helpers.
 */

#include <exec/types.h>

#include "private/cc_build.h"
#include "private/cc_internal.h"
#include "cclib_funcs.h"

#include <errno.h>

#ifndef WEOF
#define WEOF ((LONG)-1)
#endif

/* Wide code must fit in one unsigned byte for identity conversion. */
static LONG
cc_wchar_byte_ok(ULONG wc)
{
	return (wc <= 255UL) ? 1L : 0L;
}

/*
 * Map wint_t to a ctype LONG.  WEOF and out-of-range wide values are
 * rejected by callers before this is used for classification.
 */
static LONG
cc_wint_to_c(LONG c)
{
	return (LONG)(c & 0xFFL);
}

LONG __ASM__ __SAVE_DS__ mblen(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG n))
{
	cc_check_break();

	/* Null s: encoding is not state-dependent. */
	if (s == (STRPTR)NULL)
		return 0L;
	if (n == 0UL)
		return -1L;
	if (!cc_buf_mapped((APTR)s, 1UL))
		return -1L;
	if (*s == '\0')
		return 0L;
	return 1L;
}

LONG __ASM__ __SAVE_DS__ mbtowc(
	__REG__(a0, APTR pwc),
	__REG__(a1, STRPTR s),
	__REG__(d0, ULONG n))
{
	UBYTE ch;

	cc_check_break();

	if (s == (STRPTR)NULL)
		return 0L;
	if (n == 0UL)
		return -1L;
	if (!cc_buf_mapped((APTR)s, 1UL))
		return -1L;
	if (pwc != (APTR)NULL && !cc_buf_mapped(pwc, sizeof(ULONG)))
		return -1L;

	ch = (UBYTE)*s;
	if (ch == 0U)
	{
		if (pwc != (APTR)NULL)
			*((ULONG *)pwc) = 0UL;
		return 0L;
	}

	if (pwc != (APTR)NULL)
		*((ULONG *)pwc) = (ULONG)ch;
	return 1L;
}

LONG __ASM__ __SAVE_DS__ wctomb(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG wc))
{
	cc_check_break();

	/* Null s: encoding is not state-dependent. */
	if (s == (STRPTR)NULL)
		return 0L;
	if (!cc_buf_mapped((APTR)s, 1UL))
		return -1L;

	if (!cc_wchar_byte_ok(wc))
	{
		cc_set_errno((LONG)EILSEQ);
		return -1L;
	}

	*s = (char)(UBYTE)wc;
	return 1L;
}

ULONG __ASM__ __SAVE_DS__ mbstowcs(
	__REG__(a0, APTR pwcs),
	__REG__(a1, STRPTR s),
	__REG__(d0, ULONG n))
{
	ULONG i;
	ULONG *dst;
	STRPTR src;

	cc_check_break();

	if (s == (STRPTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return 0UL;
	}
	if (!cc_str_mapped(s))
		return 0UL;
	if (pwcs != (APTR)NULL && n > 0UL
	    && !cc_buf_mapped(pwcs, n * sizeof(ULONG)))
		return 0UL;

	dst = (ULONG *)pwcs;
	src = s;
	i = 0UL;

	/*
	 * When pwcs is NULL, only count wide characters (ISO).  Stop at
	 * NUL; do not write.  When pwcs is non-NULL, convert at most n
	 * wide chars (NUL may be written if room).
	 */
	if (dst == (ULONG *)NULL)
	{
		while (*src != '\0')
		{
			i++;
			src++;
		}
		return i;
	}

	while (i < n)
	{
		if (*src == '\0')
		{
			dst[i] = 0UL;
			return i;
		}
		dst[i] = (ULONG)(UBYTE)*src;
		src++;
		i++;
	}
	return i;
}

ULONG __ASM__ __SAVE_DS__ wcstombs(
	__REG__(a0, STRPTR s),
	__REG__(a1, APTR pwcs),
	__REG__(d0, ULONG n))
{
	ULONG i;
	ULONG wc;
	ULONG *src;
	STRPTR dst;
	ULONG probe;

	cc_check_break();

	if (pwcs == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return 0UL;
	}
	if (!cc_buf_mapped(pwcs, sizeof(ULONG)))
		return 0UL;
	if (s != (STRPTR)NULL && n > 0UL && !cc_buf_mapped((APTR)s, n))
		return (ULONG)-1L;

	src = (ULONG *)pwcs;
	dst = s;
	i = 0UL;

	if (dst == (STRPTR)NULL)
	{
		probe = 0UL;
		while (probe < (ULONG)CC_STR_PROBE && src[probe] != 0UL)
			probe++;
		if (probe == (ULONG)CC_STR_PROBE && src[probe] != 0UL)
		{
			cc_set_errno(CC_EINVAL);
			cc_panic(20L);
			return (ULONG)-1L;
		}
		while (src[i] != 0UL)
		{
			if (!cc_wchar_byte_ok(src[i]))
			{
				cc_set_errno((LONG)EILSEQ);
				return (ULONG)-1L;
			}
			i++;
		}
		return i;
	}

	while (i < n)
	{
		wc = src[i];
		if (wc == 0UL)
		{
			dst[i] = '\0';
			return i;
		}
		if (!cc_wchar_byte_ok(wc))
		{
			cc_set_errno((LONG)EILSEQ);
			return (ULONG)-1L;
		}
		dst[i] = (char)(UBYTE)wc;
		i++;
	}
	return i;
}

static LONG
cc_isw_dispatch(LONG c, LONG (*fn)(cc_task_UserData *, LONG))
{
	cc_task_UserData *ud;

	if (c == WEOF)
		return 0L;
	if (c < 0L || c > 255L)
		return 0L;

	ud = cc_get_ud();
	return (*fn)(ud, cc_wint_to_c(c));
}

LONG __ASM__ __SAVE_DS__ iswalnum(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isalnum_ch);
}

LONG __ASM__ __SAVE_DS__ iswalpha(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isalpha_ch);
}

LONG __ASM__ __SAVE_DS__ iswcntrl(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_iscntrl_ch);
}

LONG __ASM__ __SAVE_DS__ iswdigit(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isdigit_ch);
}

LONG __ASM__ __SAVE_DS__ iswgraph(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isgraph_ch);
}

LONG __ASM__ __SAVE_DS__ iswlower(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_islower_ch);
}

LONG __ASM__ __SAVE_DS__ iswprint(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isprint_ch);
}

LONG __ASM__ __SAVE_DS__ iswpunct(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_ispunct_ch);
}

LONG __ASM__ __SAVE_DS__ iswspace(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isspace_ch);
}

LONG __ASM__ __SAVE_DS__ iswupper(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isupper_ch);
}

LONG __ASM__ __SAVE_DS__ iswxdigit(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isxdigit_ch);
}

LONG __ASM__ __SAVE_DS__ iswblank(
	__REG__(d0, LONG c))
{
	cc_check_break();
	return cc_isw_dispatch(c, cc_isblank_ch);
}

LONG __ASM__ __SAVE_DS__ iswascii(
	__REG__(d0, LONG c))
{
	cc_check_break();
	if (c == WEOF)
		return 0L;
	if (c < 0L || c > 255L)
		return 0L;
	return cc_isascii_ch(cc_wint_to_c(c));
}

LONG __ASM__ __SAVE_DS__ towlower(
	__REG__(d0, LONG c))
{
	cc_task_UserData *ud;

	cc_check_break();
	if (c == WEOF)
		return WEOF;
	if (c < 0L || c > 255L)
		return c;
	ud = cc_get_ud();
	return cc_tolower_ch(ud, cc_wint_to_c(c));
}

LONG __ASM__ __SAVE_DS__ towupper(
	__REG__(d0, LONG c))
{
	cc_task_UserData *ud;

	cc_check_break();
	if (c == WEOF)
		return WEOF;
	if (c < 0L || c > 255L)
		return c;
	ud = cc_get_ud();
	return cc_toupper_ch(ud, cc_wint_to_c(c));
}
