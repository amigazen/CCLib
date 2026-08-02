/*
 * cctest_list.c - linked-list helpers
 */

#include <exec/types.h>
#include <proto/exec.h>
#include <proto/cclib.h>
#include <libraries/cclib.h>
#include "cctest_util.h"

typedef struct
{
	APTR first;
	APTR last;
	LONG length;
} ct_list;

#define CT_LIST_DATAP(node) ((APTR)((UBYTE *)(node) + 8))

static void
ct_list_freenode(APTR node)
{
	ULONG *hp;

	if (node == (APTR)NULL)
		return;
	hp = ((ULONG *)node) - 1;
	FreeMem(hp, *hp);
}

void
ct_suite_list(void)
{
	ct_list list;
	ct_list *hlist;
	APTR n1;
	APTR n2;
	APTR n3;
	APTR data;
	APTR got;

	ct_suite_begin((STRPTR)"list",
		(STRPTR)"inst_node, add/put, remv_*, r_node");

	list.first = (APTR)NULL;
	list.last = (APTR)NULL;
	list.length = 0;

	n1 = inst_node(16);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"inst_node_16",
		(STRPTR)"inst_node(16)",
		(STRPTR)"inst_node(16) must allocate a node with 16 bytes of payload room",
		n1);
	if (n1 != (APTR)NULL)
		ct_list_freenode(n1);

	n2 = add_tail((APTR)&list, 8);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"add_tail_first",
		(STRPTR)"add_tail(&list,8)",
		(STRPTR)"add_tail on an empty list must create and link a node",
		n2);
	ct_check_long((STRPTR)"list", (STRPTR)"list_len_1",
		(STRPTR)"list.length after add_tail",
		(STRPTR)"List length must be 1 after the first add_tail",
		1, list.length);

	n3 = add_head((APTR)&list, 8);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"add_head",
		(STRPTR)"add_head(&list,8)",
		(STRPTR)"add_head must create a node and place it at the front",
		n3);
	ct_check_long((STRPTR)"list", (STRPTR)"list_len_2",
		(STRPTR)"list.length after add_head",
		(STRPTR)"List length must be 2 after add_head",
		2, list.length);
	ct_check_ptr_eq((STRPTR)"list", (STRPTR)"add_head_is_first_data",
		(STRPTR)"list.first data pointer",
		(STRPTR)"add_head node data must be at list.first payload",
		n3, CT_LIST_DATAP(list.first));

	data = (APTR)NULL;
	got = remv_head((APTR)&list, (APTR)&data);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"remv_head",
		(STRPTR)"remv_head(&list,&data)",
		(STRPTR)"remv_head must return the removed node from a non-empty list",
		got);
	ct_check_ptr_eq((STRPTR)"list", (STRPTR)"remv_head_data",
		(STRPTR)"data from remv_head",
		(STRPTR)"remv_head must store the node data pointer in *datap",
		n3, data);
	ct_check_long((STRPTR)"list", (STRPTR)"list_len_after_remv_head",
		(STRPTR)"list.length after remv_head",
		(STRPTR)"List length must be 1 after remv_head",
		1, list.length);
	if (got != (APTR)NULL)
		ct_list_freenode(got);

	data = (APTR)NULL;
	got = remv_tail((APTR)&list, (APTR)&data);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"remv_tail",
		(STRPTR)"remv_tail(&list,&data)",
		(STRPTR)"remv_tail must return the remaining node",
		got);
	ct_check_ptr_eq((STRPTR)"list", (STRPTR)"remv_tail_data",
		(STRPTR)"data from remv_tail",
		(STRPTR)"remv_tail must store the node data pointer in *datap",
		n2, data);
	ct_check_long((STRPTR)"list", (STRPTR)"list_len_empty",
		(STRPTR)"list.length after remv_tail",
		(STRPTR)"List length must be 0 after removing the last node",
		0, list.length);
	if (got != (APTR)NULL)
		ct_list_freenode(got);

	got = remv_head((APTR)&list, (APTR)NULL);
	ct_check_ptr_null((STRPTR)"list", (STRPTR)"remv_head_empty",
		(STRPTR)"remv_head(&empty_list,NULL)",
		(STRPTR)"remv_head on an empty list must return NULL",
		got);
	got = remv_tail((APTR)&list, (APTR)NULL);
	ct_check_ptr_null((STRPTR)"list", (STRPTR)"remv_tail_empty",
		(STRPTR)"remv_tail(&empty_list,NULL)",
		(STRPTR)"remv_tail on an empty list must return NULL",
		got);

	n1 = add_tail((APTR)&list, 4);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"add_tail_payload",
		(STRPTR)"add_tail(&list,4)",
		(STRPTR)"add_tail must return a writable data pointer",
		n1);
	if (n1 != (APTR)NULL)
	{
		*(LONG *)n1 = 0x12345678L;
		ct_check_long((STRPTR)"list", (STRPTR)"add_tail_write",
			(STRPTR)"*(LONG*)add_tail_data",
			(STRPTR)"Payload written through add_tail data pointer must persist",
			0x12345678L, *(LONG *)n1);
		got = remv_tail((APTR)&list, (APTR)NULL);
		if (got != (APTR)NULL)
			ct_list_freenode(got);
	}

	n1 = inst_node(4);
	n2 = inst_node(4);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"inst_for_put",
		(STRPTR)"inst_node(4) twice",
		(STRPTR)"inst_node for put_head/put_tail must succeed",
		(n1 != (APTR)NULL && n2 != (APTR)NULL) ? n1 : (APTR)NULL);
	if (n1 != (APTR)NULL && n2 != (APTR)NULL)
	{
		put_head((APTR)&list, n1);
		put_tail((APTR)&list, n2);
		ct_check_long((STRPTR)"list", (STRPTR)"put_head_tail_len",
			(STRPTR)"list.length after put_head+put_tail",
			(STRPTR)"After put_head then put_tail, length must be 2",
			2, list.length);

		data = CT_LIST_DATAP(n1);
		got = remv_node((APTR)&list, data);
		ct_check_ptr_eq((STRPTR)"list", (STRPTR)"remv_node",
			(STRPTR)"remv_node(&list,n1_data)",
			(STRPTR)"remv_node must unlink the node matching the data pointer",
			n1, got);
		ct_check_long((STRPTR)"list", (STRPTR)"list_len_after_remv_node",
			(STRPTR)"list.length after remv_node",
			(STRPTR)"List length must be 1 after remv_node",
			1, list.length);
		if (got != (APTR)NULL)
			ct_list_freenode(got);

		data = (APTR)NULL;
		got = r_node((APTR)&list, n2, (APTR)&data);
		ct_check_ptr_eq((STRPTR)"list", (STRPTR)"r_node",
			(STRPTR)"r_node(&list,n2,&data)",
			(STRPTR)"r_node must remove the given node and return it",
			n2, got);
		ct_check_ptr_eq((STRPTR)"list", (STRPTR)"r_node_data",
			(STRPTR)"data from r_node",
			(STRPTR)"r_node must store the node data pointer in *datap",
			CT_LIST_DATAP(n2), data);
		ct_check_long((STRPTR)"list", (STRPTR)"list_len_after_r_node",
			(STRPTR)"list.length after r_node",
			(STRPTR)"List length must be 0 after r_node removes the last node",
			0, list.length);
		if (got != (APTR)NULL)
			ct_list_freenode(got);
	}

	hlist = (ct_list *)AllocMem(sizeof(ct_list), MEMF_CLEAR);
	ct_check_ptr_nonnull((STRPTR)"list", (STRPTR)"heap_list_alloc",
		(STRPTR)"AllocMem(sizeof(ct_list))",
		(STRPTR)"remv_list test requires a heap-allocated list header",
		(APTR)hlist);
	if (hlist != (ct_list *)NULL)
	{
		n1 = inst_node(4);
		n2 = inst_node(4);
		if (n1 != (APTR)NULL && n2 != (APTR)NULL)
		{
			put_head((APTR)hlist, n1);
			put_tail((APTR)hlist, n2);
			ct_check_long((STRPTR)"list", (STRPTR)"heap_list_len_2",
				(STRPTR)"hlist->length before remv_list",
				(STRPTR)"Heap list length must be 2 before remv_list",
				2, hlist->length);
			remv_list((APTR)hlist);
			ct_check_true((STRPTR)"list", (STRPTR)"remv_list_done",
				(STRPTR)"remv_list(hlist)",
				(STRPTR)"remv_list must free all nodes and the list header without fault",
				1);
		}
		else
		{
			if (n1 != (APTR)NULL)
				ct_list_freenode(n1);
			if (n2 != (APTR)NULL)
				ct_list_freenode(n2);
			FreeMem(hlist, sizeof(ct_list));
		}
	}

	n1 = inst_node(8);
	n2 = inst_node(8);
	if (n1 != (APTR)NULL && n2 != (APTR)NULL)
	{
		put_tail((APTR)&list, n1);
		put_tail((APTR)&list, n2);
		ct_check_long((STRPTR)"list", (STRPTR)"two_put_tail_len",
			(STRPTR)"list.length after two put_tail",
			(STRPTR)"List length must be 2 after two put_tail calls",
			2, list.length);
		got = remv_head((APTR)&list, (APTR)NULL);
		if (got != (APTR)NULL)
			ct_list_freenode(got);
		got = remv_tail((APTR)&list, (APTR)NULL);
		if (got != (APTR)NULL)
			ct_list_freenode(got);
		ct_check_long((STRPTR)"list", (STRPTR)"manual_drain_empty",
			(STRPTR)"list.length after manual drain",
			(STRPTR)"List length must be 0 after draining with remv_head/remv_tail",
			0, list.length);
	}
}
