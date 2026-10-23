/*
 * cctest_heap.c - malloc, calloc, realloc, free, malloc_size, heap_alloc,
 * heap_free, and be_free
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

static UBYTE ct_be_static[64];

static LONG
ct_heap_zeroed(APTR p, ULONG n)
{
	UBYTE *b;
	ULONG i;

	b = (UBYTE *)p;
	i = 0;
	while (i < n)
	{
		if (b[i] != 0)
			return 0;
		i++;
	}
	return 1;
}

static LONG
ct_heap_pattern_ok(APTR p, ULONG n, UBYTE fill)
{
	UBYTE *b;

	b = (UBYTE *)p;
	if (b[0] != fill)
		return 0;
	if (n > 1UL && b[n - 1UL] != fill)
		return 0;
	return 1;
}

void
ct_suite_heap(void)
{
	APTR p;
	APTR q;
	APTR r;
	ULONG sz;
	ULONG req;
	UBYTE *b;

	ct_suite_begin((STRPTR)"heap",
		(STRPTR)"malloc, calloc, realloc, heap_alloc, be_free");

	req = 1UL;
	p = malloc(req);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"malloc_1",
		(STRPTR)"malloc(1)",
		(STRPTR)"malloc(1) must return a usable block",
		p);
	if (p != (APTR)NULL)
	{
		sz = malloc_size(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_size_ge_1",
			(STRPTR)"malloc_size(p) for malloc(1)",
			(STRPTR)"malloc_size of a 1-byte allocation must be >= 1",
			(LONG)(sz >= 1UL));
		memset(p, 0xA1, 1);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_1_writable",
			(STRPTR)"write pattern to malloc(1) block",
			(STRPTR)"Caller must be able to write the 1-byte block",
			ct_heap_pattern_ok(p, 1UL, 0xA1));
		free(p);
	}

	req = 8UL;
	p = malloc(req);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"malloc_8",
		(STRPTR)"malloc(8)",
		(STRPTR)"malloc(8) must return a usable block",
		p);
	if (p != (APTR)NULL)
	{
		sz = malloc_size(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_size_ge_8",
			(STRPTR)"malloc_size(p) for malloc(8)",
			(STRPTR)"malloc_size of an 8-byte allocation must be >= 8",
			(LONG)(sz >= 8UL));
		memset(p, 0xB2, 8);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_8_writable",
			(STRPTR)"write pattern to malloc(8) block",
			(STRPTR)"Caller must be able to write all 8 bytes",
			ct_heap_pattern_ok(p, 8UL, 0xB2));
		free(p);
	}

	req = 64UL;
	p = malloc(req);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"malloc_64",
		(STRPTR)"malloc(64)",
		(STRPTR)"malloc(64) must return a usable block",
		p);
	if (p != (APTR)NULL)
	{
		sz = malloc_size(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_size_ge_64",
			(STRPTR)"malloc_size(p) for malloc(64)",
			(STRPTR)"malloc_size of a 64-byte allocation must be >= 64",
			(LONG)(sz >= 64UL));
		memset(p, 0x5A, 64);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_64_writable",
			(STRPTR)"write pattern to malloc(64) block",
			(STRPTR)"Caller must be able to write all 64 bytes",
			ct_heap_pattern_ok(p, 64UL, 0x5A));
		free(p);
	}

	req = 256UL;
	p = malloc(req);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"malloc_256",
		(STRPTR)"malloc(256)",
		(STRPTR)"malloc(256) must return a usable block",
		p);
	if (p != (APTR)NULL)
	{
		sz = malloc_size(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_size_ge_256",
			(STRPTR)"malloc_size(p) for malloc(256)",
			(STRPTR)"malloc_size of a 256-byte allocation must be >= 256",
			(LONG)(sz >= 256UL));
		memset(p, 0xC3, 256);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_256_writable",
			(STRPTR)"write pattern to malloc(256) block",
			(STRPTR)"Caller must be able to write first and last bytes",
			ct_heap_pattern_ok(p, 256UL, 0xC3));
		free(p);
	}

	req = 1024UL;
	p = malloc(req);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"malloc_1024",
		(STRPTR)"malloc(1024)",
		(STRPTR)"malloc(1024) must return a usable block",
		p);
	if (p != (APTR)NULL)
	{
		sz = malloc_size(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_size_ge_1024",
			(STRPTR)"malloc_size(p) for malloc(1024)",
			(STRPTR)"malloc_size of a 1024-byte allocation must be >= 1024",
			(LONG)(sz >= 1024UL));
		memset(p, 0xD4, 1024);
		ct_check_true((STRPTR)"heap", (STRPTR)"malloc_1024_writable",
			(STRPTR)"write pattern to malloc(1024) block",
			(STRPTR)"Caller must be able to write first and last bytes",
			ct_heap_pattern_ok(p, 1024UL, 0xD4));
		free(p);
	}

	p = calloc(1, 1);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"calloc_1x1",
		(STRPTR)"calloc(1,1)",
		(STRPTR)"calloc(1,1) must return a 1-byte zeroed block",
		p);
	if (p != (APTR)NULL)
	{
		ct_check_true((STRPTR)"heap", (STRPTR)"calloc_1x1_zeroed",
			(STRPTR)"inspect calloc(1,1) bytes",
			(STRPTR)"calloc(1,1) byte must be zero",
			ct_heap_zeroed(p, 1UL));
		free(p);
	}

	p = calloc(4, 2);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"calloc_4x2",
		(STRPTR)"calloc(4,2)",
		(STRPTR)"calloc(4,2) must return an 8-byte zeroed block",
		p);
	if (p != (APTR)NULL)
	{
		ct_check_true((STRPTR)"heap", (STRPTR)"calloc_4x2_zeroed",
			(STRPTR)"inspect calloc(4,2) bytes",
			(STRPTR)"All 8 bytes from calloc(4,2) must be zero",
			ct_heap_zeroed(p, 8UL));
		free(p);
	}

	p = calloc(8, 4);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"calloc_8x4",
		(STRPTR)"calloc(8,4)",
		(STRPTR)"calloc(8,4) must return a 32-byte zeroed block",
		p);
	if (p != (APTR)NULL)
	{
		ct_check_true((STRPTR)"heap", (STRPTR)"calloc_8x4_zeroed",
			(STRPTR)"inspect calloc(8,4) bytes",
			(STRPTR)"All 32 bytes from calloc(8,4) must be zero",
			ct_heap_zeroed(p, 32UL));
		free(p);
	}

	p = calloc(16, 4);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"calloc_16x4",
		(STRPTR)"calloc(16,4)",
		(STRPTR)"calloc(16,4) must return a 64-byte zeroed block",
		p);
	if (p != (APTR)NULL)
	{
		ct_check_true((STRPTR)"heap", (STRPTR)"calloc_16x4_zeroed",
			(STRPTR)"inspect calloc(16,4) bytes",
			(STRPTR)"All 64 bytes from calloc(16,4) must be zero",
			ct_heap_zeroed(p, 64UL));
		free(p);
	}

	p = malloc(16);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"realloc_base",
		(STRPTR)"malloc(16)",
		(STRPTR)"malloc(16) as base for realloc must succeed",
		p);
	if (p != (APTR)NULL)
	{
		memset(p, 'A', 16);
		q = realloc(p, 48);
		ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"realloc_grow",
			(STRPTR)"realloc(p,48)",
			(STRPTR)"realloc growing 16->48 bytes must return a block",
			q);
		if (q != (APTR)NULL)
		{
			b = (UBYTE *)q;
			ct_check_true((STRPTR)"heap", (STRPTR)"realloc_grow_preserves",
				(STRPTR)"inspect first 16 bytes after realloc grow",
				(STRPTR)"First 16 bytes must still be 'A' after grow",
				b[0] == 'A' && b[15] == 'A');
			r = realloc(q, 8);
			ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"realloc_shrink",
				(STRPTR)"realloc(q,8)",
				(STRPTR)"realloc shrinking 48->8 bytes must return a block",
				r);
			if (r != (APTR)NULL)
			{
				b = (UBYTE *)r;
				ct_check_true((STRPTR)"heap", (STRPTR)"realloc_shrink_preserves",
					(STRPTR)"inspect first 8 bytes after realloc shrink",
					(STRPTR)"First 8 bytes must still be 'A' after shrink",
					b[0] == 'A' && b[7] == 'A');
				free(r);
			}
		}
	}

	p = malloc(0);
	ct_check_true((STRPTR)"heap", (STRPTR)"malloc_zero_defined",
		(STRPTR)"malloc(0)",
		(STRPTR)"malloc(0) must either return NULL or a freeable pointer (no crash)",
		1);
	if (p != (APTR)NULL)
		free(p);

	p = heap_alloc(128);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"heap_alloc_128",
		(STRPTR)"heap_alloc(128)",
		(STRPTR)"heap_alloc(128) must return a block from the CClib heap",
		p);
	if (p != (APTR)NULL)
	{
		memset(p, 0, 128);
		heap_free(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"heap_free_ok",
			(STRPTR)"heap_free(p)",
			(STRPTR)"heap_free of a heap_alloc block must complete without fault",
			1);
	}

	memset(ct_be_static, 0xEE, 64);
	be_free((APTR)ct_be_static, 64UL);
	ct_check_true((STRPTR)"heap", (STRPTR)"be_free_ok",
		(STRPTR)"be_free(ct_be_static,64)",
		(STRPTR)"be_free of static memory must complete without fault",
		1);
	p = heap_alloc(64);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"heap_alloc_after_be_free",
		(STRPTR)"heap_alloc(64) after be_free",
		(STRPTR)"heap_alloc must still succeed after be_free donates static memory",
		p);
	if (p != (APTR)NULL)
		heap_free(p);

	/*
	 * 64-byte donation is too small for heap_alloc(64) (needs room for
	 * the size word). heap_alloc(32) consumes it; heap_free must drop
	 * the static pointer instead of panicking.
	 */
	p = heap_alloc(32);
	ct_check_ptr_nonnull((STRPTR)"heap", (STRPTR)"heap_alloc_from_be_free",
		(STRPTR)"heap_alloc(32) after be_free(64)",
		(STRPTR)"heap_alloc must reuse a be_free donation that fits",
		p);
	if (p != (APTR)NULL)
	{
		sz = malloc_size(p);
		ct_check_ulong((STRPTR)"heap", (STRPTR)"malloc_size_donated",
			(STRPTR)"malloc_size of a be_free donation",
			(STRPTR)"malloc_size of a reused be_free block must be the rounded request",
			32UL, sz);
		heap_free(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"heap_free_donated",
			(STRPTR)"heap_free of a be_free donation",
			(STRPTR)"heap_free of static donation must return without panic",
			1);
	}

	p = calloc(0xFFFFFFFFUL, 2UL);
	ct_check_ptr_null((STRPTR)"heap", (STRPTR)"calloc_overflow",
		(STRPTR)"calloc(0xFFFFFFFF, 2)",
		(STRPTR)"calloc must return NULL when nelem * elsize overflows",
		p);

	ct_install_panic_hook();
	_fre(GetSTDIO(), (APTR)&ct_be_static);
	ct_check_true((STRPTR)"heap", (STRPTR)"fre_unknown",
		(STRPTR)"_fre of a pointer that is not on RAM",
		(STRPTR)"_fre of a non-member must panic like free (hook returns)",
		ct_panic_hits == 1L);
	ct_restore_panic_hook();

	r = malloc(32);
	q = malloc(32);
	ct_check_true((STRPTR)"heap", (STRPTR)"malloc_distinct",
		(STRPTR)"malloc(32) twice",
		(STRPTR)"Two successive malloc(32) calls must return distinct pointers",
		r != (APTR)NULL && q != (APTR)NULL && r != q);
	if (r != (APTR)NULL)
		free(r);
	if (q != (APTR)NULL)
		free(q);

	p = malloc(8);
	if (p != (APTR)NULL)
	{
		free(p);
		ct_check_true((STRPTR)"heap", (STRPTR)"free_once",
			(STRPTR)"free(p)",
			(STRPTR)"free of a valid malloc block must complete without fault",
			1);
	}
}
