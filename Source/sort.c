/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * sort.c - quicksort variants for cclib.library
 *
 * vquicksort / quicksort operate on abstract indices via caller-supplied
 * compare and swap hooks. qsort sorts a contiguous array of width-byte
 * elements with a C-callable compar(APTR, APTR).
 */

#include <exec/types.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

typedef LONG (*cc_vq_cmp)(ULONG a, ULONG b);
typedef void (*cc_vq_swap)(ULONG a, ULONG b);

typedef LONG (*cc_qs_cmp)(ULONG a, ULONG b, APTR arg);
typedef void (*cc_qs_swap)(ULONG a, ULONG b, APTR arg);

typedef LONG (*cc_qsort_cmp)(APTR a, APTR b);

/* Median-of-three index selection for abstract quicksort */
static ULONG cc_med3_idx(ULONG a, ULONG b, ULONG c, cc_vq_cmp cmp)
{
	if ((*cmp)(a, b) < 0L)
	{
		if ((*cmp)(b, c) < 0L)
			return b;
		if ((*cmp)(a, c) < 0L)
			return c;
		return a;
	}
	if ((*cmp)(b, c) > 0L)
		return b;
	if ((*cmp)(a, c) > 0L)
		return c;
	return a;
}

static void cc_vq_sort(ULONG left, ULONG right, cc_vq_cmp cmp, cc_vq_swap swap)
{
	ULONG i;
	ULONG j;
	ULONG pivot;

	if (left >= right)
		return;

	/* Small partitions: insertion-style via pairwise swaps */
	if (right - left < 8UL)
	{
		for (i = left + 1UL; i <= right; i++)
		{
			j = i;
			while (j > left && (*cmp)(j - 1UL, j) > 0L)
			{
				(*swap)(j - 1UL, j);
				j--;
			}
		}
		return;
	}

	pivot = cc_med3_idx(left, left + (right - left) / 2UL, right, cmp);
	(*swap)(left, pivot);
	pivot = left;

	i = left;
	j = right + 1UL;
	for (;;)
	{
		do
		{
			i++;
		} while (i <= right && (*cmp)(i, pivot) < 0L);

		do
		{
			j--;
		} while ((*cmp)(j, pivot) > 0L);

		if (i >= j)
			break;
		(*swap)(i, j);
	}

	(*swap)(left, j);
	if (j > 0UL && left < j - 1UL)
		cc_vq_sort(left, j - 1UL, cmp, swap);
	if (j + 1UL < right)
		cc_vq_sort(j + 1UL, right, cmp, swap);
}

void __ASM__ __SAVE_DS__ vquicksort(
	__REG__(d0, ULONG n),
	__REG__(a0, APTR cmp),
	__REG__(a1, APTR swap))
{
	cc_check_break();
	if (n < 2UL || cmp == NULL || swap == NULL)
		return;
	cc_vq_sort(0UL, n - 1UL, (cc_vq_cmp)cmp, (cc_vq_swap)swap);
}

static ULONG cc_med3_arg(ULONG a, ULONG b, ULONG c, cc_qs_cmp cmp, APTR arg)
{
	if ((*cmp)(a, b, arg) < 0L)
	{
		if ((*cmp)(b, c, arg) < 0L)
			return b;
		if ((*cmp)(a, c, arg) < 0L)
			return c;
		return a;
	}
	if ((*cmp)(b, c, arg) > 0L)
		return b;
	if ((*cmp)(a, c, arg) > 0L)
		return c;
	return a;
}

static void cc_qs_sort(ULONG left, ULONG right, cc_qs_cmp cmp, cc_qs_swap swap, APTR arg)
{
	ULONG i;
	ULONG j;
	ULONG pivot;

	if (left >= right)
		return;

	if (right - left < 8UL)
	{
		for (i = left + 1UL; i <= right; i++)
		{
			j = i;
			while (j > left && (*cmp)(j - 1UL, j, arg) > 0L)
			{
				(*swap)(j - 1UL, j, arg);
				j--;
			}
		}
		return;
	}

	pivot = cc_med3_arg(left, left + (right - left) / 2UL, right, cmp, arg);
	(*swap)(left, pivot, arg);
	pivot = left;

	i = left;
	j = right + 1UL;
	for (;;)
	{
		do
		{
			i++;
		} while (i <= right && (*cmp)(i, pivot, arg) < 0L);

		do
		{
			j--;
		} while ((*cmp)(j, pivot, arg) > 0L);

		if (i >= j)
			break;
		(*swap)(i, j, arg);
	}

	(*swap)(left, j, arg);
	if (j > 0UL && left < j - 1UL)
		cc_qs_sort(left, j - 1UL, cmp, swap, arg);
	if (j + 1UL < right)
		cc_qs_sort(j + 1UL, right, cmp, swap, arg);
}

void __ASM__ __SAVE_DS__ quicksort(
	__REG__(d0, ULONG n),
	__REG__(a0, APTR cmp),
	__REG__(a1, APTR swap),
	__REG__(a2, APTR arg))
{
	cc_check_break();
	if (n < 2UL || cmp == NULL || swap == NULL)
		return;
	cc_qs_sort(0UL, n - 1UL, (cc_qs_cmp)cmp, (cc_qs_swap)swap, arg);
}

/* qsort helpers: compare/swap by byte-array index */
typedef struct
{
	UBYTE *base;
	ULONG width;
	cc_qsort_cmp compar;
} cc_qsort_ctx;

static LONG cc_qsort_cmp_idx(ULONG a, ULONG b, APTR arg)
{
	cc_qsort_ctx *ctx;
	UBYTE *pa;
	UBYTE *pb;

	ctx = (cc_qsort_ctx *)arg;
	pa = ctx->base + a * ctx->width;
	pb = ctx->base + b * ctx->width;
	return (*ctx->compar)((APTR)pa, (APTR)pb);
}

static void cc_qsort_swap_idx(ULONG a, ULONG b, APTR arg)
{
	cc_qsort_ctx *ctx;
	UBYTE *pa;
	UBYTE *pb;
	UBYTE tmp;
	ULONG i;

	ctx = (cc_qsort_ctx *)arg;
	if (a == b)
		return;
	pa = ctx->base + a * ctx->width;
	pb = ctx->base + b * ctx->width;
	for (i = 0UL; i < ctx->width; i++)
	{
		tmp = pa[i];
		pa[i] = pb[i];
		pb[i] = tmp;
	}
}

void __ASM__ __SAVE_DS__ qsort(
	__REG__(a0, APTR base),
	__REG__(d0, ULONG nel),
	__REG__(d1, ULONG width),
	__REG__(a1, APTR compar))
{
	cc_qsort_ctx ctx;
	cc_check_break();

	if (nel < 2UL || width == 0UL)
		return;
	if (base == (APTR)NULL || compar == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return;
	}
	if (!cc_buf_mapped(base, nel * width) || !cc_buf_mapped(compar, 1UL))
		return;

	ctx.base = (UBYTE *)base;
	ctx.width = width;
	ctx.compar = (cc_qsort_cmp)compar;
	/* Call the abstract sorter directly (same register ABI as quicksort LVO) */
	cc_qs_sort(0UL, nel - 1UL, cc_qsort_cmp_idx, cc_qsort_swap_idx, (APTR)&ctx);
}

APTR __ASM__ __SAVE_DS__ bsearch(
	__REG__(a0, APTR key),
	__REG__(a1, APTR base),
	__REG__(d0, ULONG nel),
	__REG__(d1, ULONG width),
	__REG__(a2, APTR compar))
{
	UBYTE *b;

	cc_qsort_cmp cmp;
	ULONG lo;
	ULONG hi;
	ULONG mid;
	LONG c;
	UBYTE *elem;
	cc_check_break();

	if (width == 0UL || nel == 0UL)
		return (APTR)NULL;
	if (key == (APTR)NULL || base == (APTR)NULL || compar == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (APTR)NULL;
	}
	if (!cc_buf_mapped(key, width) || !cc_buf_mapped(base, nel * width)
	    || !cc_buf_mapped(compar, 1UL))
		return (APTR)NULL;

	b = (UBYTE *)base;
	cmp = (cc_qsort_cmp)compar;
	lo = 0UL;
	hi = nel;
	while (lo < hi)
	{
		mid = lo + (hi - lo) / 2UL;
		elem = b + mid * width;
		c = (*cmp)(key, (APTR)elem);
		if (c == 0L)
			return (APTR)elem;
		if (c < 0L)
			hi = mid;
		else
			lo = mid + 1UL;
	}
	return (APTR)NULL;
}
