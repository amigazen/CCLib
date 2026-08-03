/*
 * ccpanic_heap.c - forged / stale malloc pointers and NULL free
 */

#include <exec/types.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include <errno.h>
#include "cctest_util.h"

void
ct_suite_panic_heap(void)
{
	APTR p;
	APTR q;
	ULONG sz;
	LONG hits;

	ct_suite_begin((STRPTR)"panic_heap",
		(STRPTR)"malloc validation / abort_func");

	/* free(NULL) is defined and must not panic. */
	ct_install_panic_hook();
	free((APTR)NULL);
	ct_check_true((STRPTR)"panic_heap", (STRPTR)"free_null",
		(STRPTR)"free(NULL)",
		(STRPTR)"free(NULL) must complete without calling abort_func",
		ct_panic_hits == 0L);
	ct_restore_panic_hook();

	ct_install_panic_hook();
	sz = malloc_size((APTR)NULL);
	ct_check_true((STRPTR)"panic_heap", (STRPTR)"malloc_size_null",
		(STRPTR)"malloc_size(NULL)",
		(STRPTR)"malloc_size(NULL) must return 0 without panic",
		sz == 0UL && ct_panic_hits == 0L);
	ct_restore_panic_hook();

	/* Forged pointer: panic + fail the LVO. */
	errno = 0;
	ct_install_panic_hook();
	sz = malloc_size((APTR)0x12345678L);
	ct_check_true((STRPTR)"panic_heap", (STRPTR)"malloc_size_forged",
		(STRPTR)"malloc_size(forged)",
		(STRPTR)"malloc_size of a non-heap pointer must panic and return 0",
		sz == 0UL && ct_panic_hits == 1L);
	ct_check_long((STRPTR)"panic_heap", (STRPTR)"malloc_size_forged_errno",
		(STRPTR)"errno after malloc_size(forged)",
		(STRPTR)"forged malloc_size must set errno to EINVAL",
		(LONG)EINVAL, errno);

	errno = 0;
	free((APTR)0x12345678L);
	ct_check_true((STRPTR)"panic_heap", (STRPTR)"free_forged_safe",
		(STRPTR)"free(forged)",
		(STRPTR)"free of a forged pointer must panic and not touch OS memory",
		ct_panic_hits == 2L);
	ct_check_long((STRPTR)"panic_heap", (STRPTR)"free_forged_errno",
		(STRPTR)"errno after free(forged)",
		(STRPTR)"forged free must set errno to EINVAL",
		(LONG)EINVAL, errno);
	ct_restore_panic_hook();

	/* Stale pointer after a real free. */
	p = malloc(16);
	ct_check_ptr_nonnull((STRPTR)"panic_heap", (STRPTR)"malloc_for_stale",
		(STRPTR)"malloc(16)",
		(STRPTR)"need a live block to free before stale checks",
		p);
	if (p != (APTR)NULL)
	{
		free(p);
		errno = 0;
		ct_install_panic_hook();
		sz = malloc_size(p);
		ct_check_true((STRPTR)"panic_heap", (STRPTR)"malloc_size_after_free",
			(STRPTR)"malloc_size after free",
			(STRPTR)"malloc_size of a freed block must panic and return 0",
			sz == 0UL && ct_panic_hits == 1L);
		ct_check_long((STRPTR)"panic_heap", (STRPTR)"malloc_size_stale_errno",
			(STRPTR)"errno after stale malloc_size",
			(STRPTR)"stale malloc_size must set errno to EINVAL",
			(LONG)EINVAL, errno);

		errno = 0;
		hits = ct_panic_hits;
		free(p);
		ct_check_true((STRPTR)"panic_heap", (STRPTR)"free_twice",
			(STRPTR)"free(stale)",
			(STRPTR)"second free of the same pointer must panic again",
			ct_panic_hits == hits + 1L);
		ct_check_long((STRPTR)"panic_heap", (STRPTR)"free_twice_errno",
			(STRPTR)"errno after double free",
			(STRPTR)"double free must set errno to EINVAL",
			(LONG)EINVAL, errno);
		ct_restore_panic_hook();
	}

	/* realloc of forged: panics in malloc_size (and again in free). */
	ct_install_panic_hook();
	q = realloc((APTR)0x12345678L, 32UL);
	ct_check_true((STRPTR)"panic_heap", (STRPTR)"realloc_forged_panic",
		(STRPTR)"realloc(forged,32)",
		(STRPTR)"realloc of a forged pointer must invoke abort_func",
		ct_panic_hits >= 1L);
	/* malloc_size returned 0; realloc may still have allocated a replacement. */
	if (q != (APTR)NULL)
		free(q);
	ct_restore_panic_hook();
}
