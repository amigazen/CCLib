/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * list.c - linked-list LVOs for cclib.library
 *
 * Node allocation uses Exec AllocMem/FreeMem (not heap_alloc) so list code
 * does not recurse through unfinished heap LVOs. Layout:
 * [ULONG total_bytes][cc_node][size bytes of caller data]
 * inst_node returns &cc_node. add_head/add_tail return (&cc_node)+1 (data).
 * remv_head/remv_tail/r_node/remv_node return &cc_node (not freed).
 * To FreeMem a returned node: hp = (ULONG *)node - 1; FreeMem(hp, *hp).
 * remv_list FreeMems every node that way, then FreeMems the list header
 * as sizeof(cc_list) (header must have been AllocMem'd that size).
 */

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* Bytes from AllocMem base through end of data payload. */
#define CC_LIST_BLK_SIZE(datasize) \
	((ULONG)(sizeof(ULONG) + sizeof(cc_node) + (ULONG)(datasize)))

static void cc_list_freenode(cc_node *node)
{
	ULONG *hp;

	if (!node)
		return;
	hp = ((ULONG *)node) - 1;
	FreeMem(hp, *hp);
}

APTR __ASM__ __SAVE_DS__ inst_node(
	__REG__(d0, LONG size))
{
	ULONG total;
	ULONG *hp;

	cc_node *node;
	cc_check_break();

	if (size < 0)
		size = 0;
	total = CC_LIST_BLK_SIZE(size);
	hp = (ULONG *)AllocMem(total, MEMF_CLEAR | MEMF_PUBLIC);
	if (!hp)
		return NULL;
	*hp = total;
	node = (cc_node *)(hp + 1);
	node->prev = NULL;
	node->next = NULL;
	return (APTR)node;
}

void __ASM__ __SAVE_DS__ put_head(
	__REG__(a0, APTR list),
	__REG__(a1, APTR node))
{
	cc_list *l;
	cc_node *n;
	cc_check_break();

	l = (cc_list *)list;
	n = (cc_node *)node;
	if (!l || !n)
		return;
	n->prev = NULL;
	n->next = l->first;
	if (l->first)
		l->first->prev = n;
	else
		l->last = n;
	l->first = n;
	l->length++;
}

APTR __ASM__ __SAVE_DS__ add_head(
	__REG__(a0, APTR list),
	__REG__(d0, LONG size))
{
	cc_list *l;
	ULONG total;
	ULONG *hp;
	cc_node *node;

	/* Alloc + link in-line (same layout as inst_node / put_head). */
	cc_check_break();
	l = (cc_list *)list;
	if (!l)
		return NULL;
	if (size < 0)
		size = 0;
	total = CC_LIST_BLK_SIZE(size);
	hp = (ULONG *)AllocMem(total, MEMF_CLEAR | MEMF_PUBLIC);
	if (!hp)
		return NULL;
	*hp = total;
	node = (cc_node *)(hp + 1);
	node->prev = NULL;
	node->next = l->first;
	if (l->first)
		l->first->prev = node;
	else
		l->last = node;
	l->first = node;
	l->length++;
	return (APTR)(node + 1);
}

void __ASM__ __SAVE_DS__ put_tail(
	__REG__(a0, APTR list),
	__REG__(a1, APTR node))
{
	cc_list *l;
	cc_node *n;
	cc_check_break();

	l = (cc_list *)list;
	n = (cc_node *)node;
	if (!l || !n)
		return;
	n->next = NULL;
	n->prev = l->last;
	if (l->last)
		l->last->next = n;
	else
		l->first = n;
	l->last = n;
	l->length++;
}

APTR __ASM__ __SAVE_DS__ add_tail(
	__REG__(a0, APTR list),
	__REG__(d0, LONG size))
{
	cc_list *l;
	ULONG total;
	ULONG *hp;
	cc_node *node;
	cc_check_break();

	l = (cc_list *)list;
	if (!l)
		return NULL;
	if (size < 0)
		size = 0;
	total = CC_LIST_BLK_SIZE(size);
	hp = (ULONG *)AllocMem(total, MEMF_CLEAR | MEMF_PUBLIC);
	if (!hp)
		return NULL;
	*hp = total;
	node = (cc_node *)(hp + 1);
	node->next = NULL;
	node->prev = l->last;
	if (l->last)
		l->last->next = node;
	else
		l->first = node;
	l->last = node;
	l->length++;
	return (APTR)(node + 1);
}

APTR __ASM__ __SAVE_DS__ remv_head(
	__REG__(a0, APTR list),
	__REG__(a1, APTR datap))
{
	cc_list *l;
	cc_node *n;
	cc_check_break();

	l = (cc_list *)list;
	if (!l || !l->first)
		return NULL;
	n = l->first;
	l->first = n->next;
	if (l->first)
		l->first->prev = NULL;
	else
		l->last = NULL;
	l->length--;
	n->prev = NULL;
	n->next = NULL;
	if (datap)
		*((APTR *)datap) = (APTR)(n + 1);
	return (APTR)n;
}

APTR __ASM__ __SAVE_DS__ remv_tail(
	__REG__(a0, APTR list),
	__REG__(a1, APTR datap))
{
	cc_list *l;
	cc_node *n;
	cc_check_break();

	l = (cc_list *)list;
	if (!l || !l->last)
		return NULL;
	n = l->last;
	l->last = n->prev;
	if (l->last)
		l->last->next = NULL;
	else
		l->first = NULL;
	l->length--;
	n->prev = NULL;
	n->next = NULL;
	if (datap)
		*((APTR *)datap) = (APTR)(n + 1);
	return (APTR)n;
}

void __ASM__ __SAVE_DS__ remv_list(
	__REG__(a0, APTR list))
{
	cc_list *l;
	cc_node *n;
	cc_node *next;
	cc_check_break();

	l = (cc_list *)list;
	if (l == (cc_list *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return;
	}
	if (TypeOfMem((APTR)l) == 0UL)
	{
		cc_set_errno(CC_EINVAL);
		cc_panic(20L);
		return;
	}
	n = l->first;
	while (n) {
		next = n->next;
		cc_list_freenode(n);
		n = next;
	}
	l->first = NULL;
	l->last = NULL;
	l->length = 0;
	/* Historic frees the list header too; assume AllocMem(sizeof(cc_list)). */
	FreeMem(l, sizeof(cc_list));
}

APTR __ASM__ __SAVE_DS__ r_node(
	__REG__(a0, APTR list),
	__REG__(a1, APTR node),
	__REG__(a2, APTR datap))
{
	cc_list *l;
	cc_node *n;
	cc_check_break();

	l = (cc_list *)list;
	n = (cc_node *)node;
	if (!l || !n)
		return NULL;
	if (!l->first)
		return NULL;

	if (n->prev)
		n->prev->next = n->next;
	else
		l->first = n->next;
	if (n->next)
		n->next->prev = n->prev;
	else
		l->last = n->prev;
	l->length--;
	n->prev = NULL;
	n->next = NULL;
	if (datap)
		*((APTR *)datap) = (APTR)(n + 1);
	return (APTR)n;
}

APTR __ASM__ __SAVE_DS__ remv_node(
	__REG__(a0, APTR list),
	__REG__(a1, APTR data))
{
	cc_list *l;
	cc_node *n;
	cc_check_break();

	l = (cc_list *)list;
	if (!l || !data)
		return NULL;
	n = l->first;
	while (n) {
		if ((APTR)(n + 1) == data) {
			/* Unlink in-line (same as r_node with datap == NULL). */
			if (n->prev)
				n->prev->next = n->next;
			else
				l->first = n->next;
			if (n->next)
				n->next->prev = n->prev;
			else
				l->last = n->prev;
			l->length--;
			n->prev = NULL;
			n->next = NULL;
			return (APTR)n;
		}
		n = n->next;
	}
	return NULL;
}
