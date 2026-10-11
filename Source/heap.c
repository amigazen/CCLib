/*
 * heap.c - memory allocation LVOs for cclib.library
 *
 * Layout for malloc-family blocks:
 *   [cc_MemBlk][ULONG nbytes][user bytes...]
 * _alloc returns the address after cc_MemBlk.  malloc stores the
 * requested (8-rounded) size in the ULONG and returns the address
 * after that header.
 *
 * Backing store is exec CreatePool / AllocPooled / FreePooled
 * (NDK 3.2R4) on the per-session ud->heap_pool from SetSTDIO.
 */

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* Round byte count up to a multiple of 8 (historic malloc_size). */
static ULONG cc_round8(ULONG n)
{
	return (n + 7UL) & ~7UL;
}

/* Link a MemBlk at the head of ud->RAM. */
static void cc_ram_link(cc_task_UserData *ud, cc_MemBlk *blk)
{
	cc_node *n;

	n = &blk->n;
	n->prev = (cc_node *)NULL;
	n->next = ud->RAM.first;
	if (ud->RAM.first != (cc_node *)NULL)
		ud->RAM.first->prev = n;
	else
		ud->RAM.last = n;
	ud->RAM.first = n;
	ud->RAM.length++;
}

/* Unlink a MemBlk from ud->RAM. */
static void cc_ram_unlink(cc_task_UserData *ud, cc_MemBlk *blk)
{
	cc_node *n;

	n = &blk->n;
	if (n->prev != (cc_node *)NULL)
		n->prev->next = n->next;
	else
		ud->RAM.first = n->next;
	if (n->next != (cc_node *)NULL)
		n->next->prev = n->prev;
	else
		ud->RAM.last = n->prev;
	ud->RAM.length--;
	n->prev = (cc_node *)NULL;
	n->next = (cc_node *)NULL;
}

/* Reset K&R heap headers after all arenas are gone. */
static void cc_heap_reset(cc_task_UserData *ud)
{
	ud->base.s.ptr = &ud->base;
	ud->base.s.size = 0;
	ud->allocp = &ud->base;
	ud->lastfree.ptr = (APTR)NULL;
	ud->lastfree.size = 0;
}

/*
 * Lowest-level allocator: AllocPooled(size + sizeof(MemBlk)) from the
 * session pool, bookkeep on ud->RAM, return payload after MemBlk.
 * flags is retained for ABI. MEMF_CLEAR is applied with memset
 * (longword stores, CopyMemQuick doubling). AllocPooled has no clear flag.
 */
APTR __ASM__ __SAVE_DS__ _alloc(
	__REG__(a0, APTR ud),
	__REG__(d0, LONG size),
	__REG__(d1, LONG flags))
{
	cc_task_UserData *u;
	cc_MemBlk *blk;
	ULONG total;
	cc_check_break();

	u = (cc_task_UserData *)ud;
	if (u == (cc_task_UserData *)NULL || size <= 0)
		return (APTR)NULL;
	if (u->heap_pool == (APTR)NULL)
		return (APTR)NULL;

	total = (ULONG)size + (ULONG)sizeof(cc_MemBlk);
	blk = (cc_MemBlk *)AllocPooled(u->heap_pool, total);
	if (blk == (cc_MemBlk *)NULL)
		return (APTR)NULL;

	if ((flags & (LONG)MEMF_CLEAR) != 0)
		(void)memset((APTR)blk, 0UL, (LONG)total);

	blk->n.prev = (cc_node *)NULL;
	blk->n.next = (cc_node *)NULL;
	blk->size = (LONG)total;
	cc_ram_link(u, blk);

	return (APTR)(blk + 1);
}

/*
 * True when blk is the header of a node currently linked on ud->RAM.
 * Address compare only: do not read blk->size until this returns true.
 */
static LONG
cc_ram_has(cc_task_UserData *ud, cc_MemBlk *blk)
{
	cc_node *n;

	n = ud->RAM.first;
	while (n != (cc_node *)NULL)
	{
		if (n == &blk->n)
			return 1L;
		n = n->next;
	}
	return 0L;
}

/* Free a block previously returned by _alloc / sbrk. */
void __ASM__ __SAVE_DS__ _fre(
	__REG__(a0, APTR ud),
	__REG__(a1, APTR ptr))
{
	cc_task_UserData *u;
	cc_MemBlk *blk;
	LONG total;
	cc_check_break();

	u = (cc_task_UserData *)ud;
	if (u == (cc_task_UserData *)NULL || ptr == (APTR)NULL)
		return;
	if (u->heap_pool == (APTR)NULL)
		return;

	blk = ((cc_MemBlk *)ptr) - 1;
	if (!cc_ram_has(u, blk))
	{
		cc_set_errno(CC_EINVAL);
		return;
	}
	total = blk->size;
	cc_ram_unlink(u, blk);
	FreePooled(u->heap_pool, blk, (ULONG)total);
}

/* Free every MemBlk on ud->RAM via FreePooled. Does not DeletePool. */
void __ASM__ __SAVE_DS__ _freall(
	__REG__(a0, APTR ud))
{
	cc_task_UserData *u;
	cc_node *n;
	cc_node *next;
	cc_MemBlk *blk;
	LONG total;
	cc_check_break();

	u = (cc_task_UserData *)ud;
	if (u == (cc_task_UserData *)NULL)
		return;

	n = u->RAM.first;
	while (n != (cc_node *)NULL) {
		next = n->next;
		blk = (cc_MemBlk *)n;
		total = blk->size;
		if (u->heap_pool != (APTR)NULL)
			FreePooled(u->heap_pool, blk, (ULONG)total);
		n = next;
	}
	u->RAM.first = (cc_node *)NULL;
	u->RAM.last = (cc_node *)NULL;
	u->RAM.length = 0;
}

/*
 * Historic sbrk: allocate via _alloc(MEMF_ANY). Failure returns (APTR)-1.
 */
APTR __ASM__ __SAVE_DS__ sbrk(
	__REG__(d0, LONG incr))
{
	cc_task_UserData *ud;
	APTR p;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL || incr <= 0)
		return (APTR)-1L;

	p = _alloc(ud, incr, (LONG)MEMF_ANY);
	if (p == (APTR)NULL)
		return (APTR)-1L;
	return p;
}

/*
 * Heap manager entry used by historic malloc path.  Prefer a be_free
 * donation on lastfree when it is large enough; otherwise malloc.
 */
APTR __ASM__ __SAVE_DS__ heap_alloc(
	__REG__(d0, ULONG size))
{
	cc_task_UserData *ud;
	ULONG need;
	APTR p;
	ULONG *hdr;
	cc_check_break();

	ud = cc_get_ud();
	need = cc_round8(size);

	/*
	 * A fresh donation has a non-zero size. size 0 with a non-NULL
	 * pointer is the user address already handed out; free() drops it.
	 */
	if (ud != (cc_task_UserData *)NULL
	    && ud->lastfree.ptr != (APTR)NULL
	    && ud->lastfree.size >= need + (ULONG)sizeof(ULONG)) {
		p = ud->lastfree.ptr;
		ud->lastfree.ptr = (APTR)NULL;
		ud->lastfree.size = 0;
		hdr = (ULONG *)p;
		*hdr = need;
		ud->lastfree.ptr = (APTR)(hdr + 1);
		return (APTR)(hdr + 1);
	}

	return malloc(size);
}

void __ASM__ __SAVE_DS__ heap_free(
	__REG__(a0, APTR ptr))
{
	cc_check_break();
	free(ptr);
}

APTR __ASM__ __SAVE_DS__ malloc(
	__REG__(d0, ULONG size))
{
	cc_task_UserData *ud;
	APTR raw;
	ULONG need;
	ULONG *hdr;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return (APTR)NULL;

	need = cc_round8(size);
	raw = _alloc(ud, (LONG)(need + (ULONG)sizeof(ULONG)), (LONG)MEMF_ANY);
	if (raw == (APTR)NULL) {
		cc_set_errno(CC_ENOMEM);
		return (APTR)NULL;
	}

	hdr = (ULONG *)raw;
	*hdr = need;
	return (APTR)(hdr + 1);
}

APTR __ASM__ __SAVE_DS__ calloc(
	__REG__(d0, ULONG nelem),
	__REG__(d1, ULONG elsize))
{
	APTR p;
	ULONG total;
	cc_check_break();

	/*
	 * Reject a wrapping product. A short malloc would hand the caller
	 * a buffer smaller than nelem * elsize.
	 */
	if (nelem != 0UL && elsize > (~0UL / nelem))
	{
		cc_set_errno(CC_ENOMEM);
		return (APTR)NULL;
	}
	total = nelem * elsize;
	p = malloc(total);
	if (p == (APTR)NULL)
		return (APTR)NULL;

	/*
	 * Pool AllocPooled does not take MEMF_CLEAR. Zero through memset:
	 * longword stores, then CopyMemQuick doubling for large blocks.
	 * BltClear stays out of this path; the session pool is MEMF_ANY.
	 */
	if (total > 0UL)
		(void)memset(p, 0UL, (LONG)total);
	return p;
}

APTR __ASM__ __SAVE_DS__ realloc(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size))
{
	APTR np;
	ULONG oldsz;
	ULONG ncopy;
	cc_check_break();

	if (ptr == (APTR)NULL)
		return malloc(size);

	if (size == 0) {
		free(ptr);
		return (APTR)NULL;
	}

	oldsz = malloc_size(ptr);
	np = malloc(size);
	if (np == (APTR)NULL)
		return (APTR)NULL;

	ncopy = oldsz;
	if (ncopy > size)
		ncopy = size;

	if (ncopy > 0)
		CopyMem(ptr, np, ncopy);

	free(ptr);
	return np;
}

void __ASM__ __SAVE_DS__ free(
	__REG__(a0, APTR ptr))
{
	cc_task_UserData *ud;
	ULONG *hdr;
	cc_MemBlk *blk;
	cc_check_break();

	if (ptr == (APTR)NULL)
		return;

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return;

	/*
	 * A be_free donation handed out by heap_alloc is not on RAM.
	 * Drop the marker. Do not FreePooled static storage.
	 */
	blk = cc_blk_from_malloc_ptr(ud, ptr);
	if (blk == (cc_MemBlk *)NULL)
	{
		if (ud->lastfree.ptr == ptr && ud->lastfree.size == 0UL)
		{
			ud->lastfree.ptr = (APTR)NULL;
			return;
		}
		cc_set_errno(CC_EINVAL);
		cc_panic(20L);
		return;
	}

	hdr = ((ULONG *)ptr) - 1;
	_fre(ud, (APTR)hdr);
}

/*
 * Historic be_free donated static memory to the heap free list.
 * Record the block on lastfree so a later heap_alloc of fitting size
 * can reuse it once. heap_alloc then stores the user pointer with
 * size 0; free drops that marker and does not FreePooled it.
 */
void __ASM__ __SAVE_DS__ be_free(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size))
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL || ptr == (APTR)NULL || size == 0)
		return;

	ud->lastfree.ptr = ptr;
	ud->lastfree.size = cc_round8(size);
}

ULONG __ASM__ __SAVE_DS__ malloc_size(
	__REG__(a0, APTR ptr))
{
	cc_task_UserData *ud;
	ULONG *hdr;
	cc_check_break();

	if (ptr == (APTR)NULL)
		return 0;

	ud = cc_get_ud();
	if (ud != (cc_task_UserData *)NULL
	    && ud->lastfree.ptr == ptr
	    && ud->lastfree.size == 0UL)
	{
		hdr = ((ULONG *)ptr) - 1;
		return *hdr;
	}
	if (cc_blk_from_malloc_ptr(ud, ptr) == (cc_MemBlk *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		cc_panic(20L);
		return 0;
	}

	hdr = ((ULONG *)ptr) - 1;
	return *hdr;
}

/*
 * Return all pooled malloc arenas to the session pool and re-init heap
 * state.  closeall runs first so BPTRs are released while FILE structures
 * still exist.  A second closeall (ClearSTDIO already called it) is a
 * no-op on empty lists.  DeletePool happens in ClearSTDIO.
 */
void __ASM__ __SAVE_DS__ freeall(void)
{
	cc_task_UserData *ud;
	cc_check_break();

	ud = cc_get_ud();
	if (ud == (cc_task_UserData *)NULL)
		return;

	closeall();
	_freall(ud);
	cc_heap_reset(ud);

	ud->OpenFiles.first = (cc_node *)NULL;
	ud->OpenFiles.last = (cc_node *)NULL;
	ud->OpenFiles.length = 0;
	ud->StreamFiles.first = (cc_node *)NULL;
	ud->StreamFiles.last = (cc_node *)NULL;
	ud->StreamFiles.length = 0;
}
