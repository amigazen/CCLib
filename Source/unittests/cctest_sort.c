/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cctest_sort.c - qsort, quicksort, vquicksort
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include "cctest_util.h"

static LONG ct_sort_arr[16];

static LONG
ct_cmp_long(APTR a, APTR b)
{
	LONG xa;
	LONG xb;

	xa = *(LONG *)a;
	xb = *(LONG *)b;
	if (xa < xb)
		return -1L;
	if (xa > xb)
		return 1L;
	return 0L;
}

/*
 * __saveds: vquicksort callbacks use the near global ct_sort_arr; the
 * library enters with its own A4, so without saveds the sort would not
 * touch the test array (quicksort passes an absolute arg pointer instead).
 */
static LONG __saveds
ct_vq_cmp(ULONG a, ULONG b)
{
	if (ct_sort_arr[a] < ct_sort_arr[b])
		return -1L;
	if (ct_sort_arr[a] > ct_sort_arr[b])
		return 1L;
	return 0L;
}

static void __saveds
ct_vq_swap(ULONG a, ULONG b)
{
	LONG t;

	t = ct_sort_arr[a];
	ct_sort_arr[a] = ct_sort_arr[b];
	ct_sort_arr[b] = t;
}

static LONG
ct_qs_cmp(ULONG a, ULONG b, APTR arg)
{
	LONG *arr;

	arr = (LONG *)arg;
	if (arr[a] < arr[b])
		return -1L;
	if (arr[a] > arr[b])
		return 1L;
	return 0L;
}

static void
ct_qs_swap(ULONG a, ULONG b, APTR arg)
{
	LONG *arr;
	LONG t;

	arr = (LONG *)arg;
	t = arr[a];
	arr[a] = arr[b];
	arr[b] = t;
}

static void
ct_sort_fill_random(LONG *arr, ULONG n)
{
	ULONG i;

	arr[0] = 40;
	arr[1] = 10;
	arr[2] = 30;
	arr[3] = 20;
	arr[4] = 50;
	i = 5;
	while (i < n)
	{
		arr[i] = (LONG)(i * 7L + 3L);
		i++;
	}
}

static LONG
ct_sort_is_ascending(LONG *arr, ULONG n)
{
	ULONG i;

	i = 1;
	while (i < n)
	{
		if (arr[i - 1] > arr[i])
			return 0;
		i++;
	}
	return 1;
}

void
ct_suite_sort(void)
{
	LONG arr[5];
	LONG one;
	LONG empty;
	ULONG i;

	ct_suite_begin((STRPTR)"sort",
		(STRPTR)"qsort, vquicksort, quicksort");

	errno = 0;
	qsort((APTR)NULL, 5UL, sizeof(LONG), (APTR)ct_cmp_long);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_null_base",
		(STRPTR)"qsort(NULL, 5, ...)",
		(STRPTR)"NULL base with nel>=2 is EINVAL, no panic",
		(LONG)EINVAL, errno);

	arr[0] = 40;
	arr[1] = 10;
	arr[2] = 30;
	arr[3] = 20;
	arr[4] = 50;
	qsort((APTR)arr, 5, sizeof(LONG), (APTR)ct_cmp_long);

	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_0",
		(STRPTR)"arr[0] after qsort(5)",
		(STRPTR)"After qsort ascending, element [0] must be the smallest (10)",
		10, arr[0]);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_1",
		(STRPTR)"arr[1] after qsort(5)",
		(STRPTR)"After qsort ascending, element [1] must be 20",
		20, arr[1]);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_2",
		(STRPTR)"arr[2] after qsort(5)",
		(STRPTR)"After qsort ascending, element [2] must be 30",
		30, arr[2]);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_3",
		(STRPTR)"arr[3] after qsort(5)",
		(STRPTR)"After qsort ascending, element [3] must be 40",
		40, arr[3]);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_4",
		(STRPTR)"arr[4] after qsort(5)",
		(STRPTR)"After qsort ascending, element [4] must be the largest (50)",
		50, arr[4]);

	arr[0] = 1;
	arr[1] = 2;
	arr[2] = 3;
	arr[3] = 4;
	arr[4] = 5;
	qsort((APTR)arr, 5, sizeof(LONG), (APTR)ct_cmp_long);
	ct_check_true((STRPTR)"sort", (STRPTR)"qsort_already_sorted",
		(STRPTR)"qsort on already sorted array",
		(STRPTR)"qsort must leave an already sorted array in order",
		ct_sort_is_ascending(arr, 5UL));

	arr[0] = 50;
	arr[1] = 40;
	arr[2] = 30;
	arr[3] = 20;
	arr[4] = 10;
	qsort((APTR)arr, 5, sizeof(LONG), (APTR)ct_cmp_long);
	ct_check_true((STRPTR)"sort", (STRPTR)"qsort_reverse",
		(STRPTR)"qsort on reverse sorted array",
		(STRPTR)"qsort must sort a reverse sorted array ascending",
		ct_sort_is_ascending(arr, 5UL));

	arr[0] = 5;
	arr[1] = 2;
	arr[2] = 5;
	arr[3] = 2;
	arr[4] = 5;
	qsort((APTR)arr, 5, sizeof(LONG), (APTR)ct_cmp_long);
	ct_check_true((STRPTR)"sort", (STRPTR)"qsort_duplicates",
		(STRPTR)"qsort with duplicate keys",
		(STRPTR)"qsort with duplicates must produce non-decreasing order",
		ct_sort_is_ascending(arr, 5UL));

	one = 42;
	qsort((APTR)&one, 1, sizeof(LONG), (APTR)ct_cmp_long);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_single",
		(STRPTR)"qsort(nel=1)",
		(STRPTR)"qsort of a single element must leave it unchanged",
		42, one);

	empty = 99;
	qsort((APTR)&empty, 0, sizeof(LONG), (APTR)ct_cmp_long);
	ct_check_long((STRPTR)"sort", (STRPTR)"qsort_empty",
		(STRPTR)"qsort(nel=0)",
		(STRPTR)"qsort with nel=0 must not modify the array (no crash)",
		99, empty);

	ct_sort_fill_random(ct_sort_arr, 5UL);
	vquicksort(5UL, (APTR)ct_vq_cmp, (APTR)ct_vq_swap);
	ct_check_true((STRPTR)"sort", (STRPTR)"vquicksort_5",
		(STRPTR)"vquicksort(5, cmp, swap)",
		(STRPTR)"vquicksort must sort five elements ascending",
		ct_sort_is_ascending(ct_sort_arr, 5UL));
	ct_check_long((STRPTR)"sort", (STRPTR)"vquicksort_5_first",
		(STRPTR)"ct_sort_arr[0] after vquicksort(5)",
		(STRPTR)"Smallest element after vquicksort must be 10",
		10, ct_sort_arr[0]);

	ct_sort_fill_random(ct_sort_arr, 8UL);
	vquicksort(8UL, (APTR)ct_vq_cmp, (APTR)ct_vq_swap);
	ct_check_true((STRPTR)"sort", (STRPTR)"vquicksort_8",
		(STRPTR)"vquicksort(8, cmp, swap)",
		(STRPTR)"vquicksort must sort eight elements ascending",
		ct_sort_is_ascending(ct_sort_arr, 8UL));

	ct_sort_arr[0] = 1;
	ct_sort_arr[1] = 1;
	ct_sort_arr[2] = 1;
	vquicksort(3UL, (APTR)ct_vq_cmp, (APTR)ct_vq_swap);
	ct_check_true((STRPTR)"sort", (STRPTR)"vquicksort_equal",
		(STRPTR)"vquicksort with equal keys",
		(STRPTR)"vquicksort of equal keys must leave them equal",
		ct_sort_arr[0] == 1 && ct_sort_arr[1] == 1 && ct_sort_arr[2] == 1);

	ct_sort_fill_random(ct_sort_arr, 5UL);
	quicksort(5UL, (APTR)ct_qs_cmp, (APTR)ct_qs_swap, (APTR)ct_sort_arr);
	ct_check_true((STRPTR)"sort", (STRPTR)"quicksort_5",
		(STRPTR)"quicksort(5, cmp, swap, arr)",
		(STRPTR)"quicksort with arg must sort five elements ascending",
		ct_sort_is_ascending(ct_sort_arr, 5UL));

	ct_sort_fill_random(ct_sort_arr, 6UL);
	quicksort(6UL, (APTR)ct_qs_cmp, (APTR)ct_qs_swap, (APTR)ct_sort_arr);
	ct_check_true((STRPTR)"sort", (STRPTR)"quicksort_6",
		(STRPTR)"quicksort(6, cmp, swap, arr)",
		(STRPTR)"quicksort must sort six elements ascending",
		ct_sort_is_ascending(ct_sort_arr, 6UL));

	ct_sort_arr[0] = 7;
	vquicksort(1UL, (APTR)ct_vq_cmp, (APTR)ct_vq_swap);
	ct_check_long((STRPTR)"sort", (STRPTR)"vquicksort_single",
		(STRPTR)"vquicksort(n=1)",
		(STRPTR)"vquicksort of one element must leave it unchanged",
		7, ct_sort_arr[0]);

	ct_sort_arr[0] = 11;
	vquicksort(0UL, (APTR)ct_vq_cmp, (APTR)ct_vq_swap);
	ct_check_long((STRPTR)"sort", (STRPTR)"vquicksort_empty",
		(STRPTR)"vquicksort(n=0)",
		(STRPTR)"vquicksort with n=0 must not modify the array (no crash)",
		11, ct_sort_arr[0]);

	i = 0;
	while (i < 5UL)
	{
		ct_sort_arr[i] = (LONG)(5L - (LONG)i);
		i++;
	}
	quicksort(0UL, (APTR)ct_qs_cmp, (APTR)ct_qs_swap, (APTR)ct_sort_arr);
	ct_check_long((STRPTR)"sort", (STRPTR)"quicksort_empty",
		(STRPTR)"quicksort(n=0)",
		(STRPTR)"quicksort with n=0 must not modify the array (no crash)",
		5, ct_sort_arr[0]);

	/* --- bsearch --- */
	{
		LONG key;
		APTR found;

		ct_sort_arr[0] = 1;
		ct_sort_arr[1] = 3;
		ct_sort_arr[2] = 5;
		ct_sort_arr[3] = 7;
		ct_sort_arr[4] = 9;
		key = 7;
		found = bsearch((APTR)&key, (APTR)ct_sort_arr, 5UL, sizeof(LONG),
			(APTR)ct_cmp_long);
		ct_check_ptr_nonnull((STRPTR)"sort", (STRPTR)"bsearch_hit",
			(STRPTR)"bsearch for 7 in {1,3,5,7,9}",
			(STRPTR)"bsearch must find an existing key",
			found);
		if (found != (APTR)NULL)
			ct_check_long((STRPTR)"sort", (STRPTR)"bsearch_val",
				(STRPTR)"*found after bsearch(7)",
				(STRPTR)"bsearch result must point at the matching element",
				7, *(LONG *)found);

		key = 4;
		found = bsearch((APTR)&key, (APTR)ct_sort_arr, 5UL, sizeof(LONG),
			(APTR)ct_cmp_long);
		ct_check_ptr_null((STRPTR)"sort", (STRPTR)"bsearch_miss",
			(STRPTR)"bsearch for 4 in odd set",
			(STRPTR)"bsearch must return NULL when the key is absent",
			found);
	}
}
