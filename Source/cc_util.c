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
 * cc_util.c - task userdata helpers and ctype classification table
 */

#include <proto/exec.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

/* ctype bit flags — must match archive ctype.h macros (type[(X)+1] & mask) */
#define CC_U 0x01 /* upper */
#define CC_L 0x02 /* lower */
#define CC_D 0x04 /* digit */
#define CC_X 0x08 /* xdigit */
#define CC_S 0x10 /* space */
#define CC_C 0x20 /* cntrl */
#define CC_P 0x40 /* punct */

/*
 * Index style: tab[0] is EOF sentinel; tab[c+1] classifies unsigned char c.
 * SetupSTDIO may hand apps a pointer to this table as the ctype "type" array.
 * Shared library resident — READ-ONLY for all tasks (CClib.Doc §4).
 */
UBYTE cc_ctype_tab[257] = {
	0, /* EOF / index 0 */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /*   0..7   */
	0x20, 0xB0, 0x30, 0x30, 0x30, 0x30, 0x20, 0x20, /*   8..15  */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /*  16..23  */
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, /*  24..31  */
	0x90, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /*  32..39  */
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /*  40..47  */
	0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, /*  48..55  */
	0x0C, 0x0C, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, /*  56..63  */
	0x40, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x01, /*  64..71  */
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /*  72..79  */
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /*  80..87  */
	0x01, 0x01, 0x01, 0x40, 0x40, 0x40, 0x40, 0x40, /*  88..95  */
	0x40, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x02, /*  96..103 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 104..111 */
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* 112..119 */
	0x02, 0x02, 0x02, 0x40, 0x40, 0x40, 0x40, 0x20, /* 120..127 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 128..135 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 136..143 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 144..151 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 152..159 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 160..167 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 168..175 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 176..183 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 184..191 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 192..199 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 200..207 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 208..215 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 216..223 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 224..231 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 232..239 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 240..247 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 248..255 */
};

cc_task_UserData *cc_get_ud(void)
{
	struct Task *t;

	t = FindTask(NULL);
	if (!t)
		return NULL;
	return (cc_task_UserData *)t->tc_UserData;
}

void cc_set_errno(LONG err)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud && ud->errno_p)
		*ud->errno_p = err;
}

LONG cc_get_errno(void)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud && ud->errno_p)
		return *ud->errno_p;
	return 0;
}

/* True if data payload pointer is a node on list (add_head layout). */
static int
cc_list_has_data(cc_list *list, APTR data)
{
	cc_node *n;

	if (list == (cc_list *)NULL || data == (APTR)NULL)
		return 0;
	n = list->first;
	while (n != (cc_node *)NULL)
	{
		if ((APTR)(n + 1) == data)
			return 1;
		n = n->next;
	}
	return 0;
}

/*
 * Resolve an open() fd (pointer-as-capability) against OpenFiles.
 * Does not call AmigaOS; sets EBADF on failure.
 */
cc_FileDesc *
cc_fd_from_list(cc_task_UserData *ud, LONG fd)
{
	cc_FileDesc *desc;

	if (ud == (cc_task_UserData *)NULL || fd == 0L || fd == -1L)
	{
		cc_set_errno(CC_EBADF);
		return (cc_FileDesc *)NULL;
	}
	desc = (cc_FileDesc *)fd;
	if (!cc_list_has_data(&ud->OpenFiles, (APTR)desc))
	{
		cc_set_errno(CC_EBADF);
		return (cc_FileDesc *)NULL;
	}
	if (desc->magic != CC_FD_MAGIC)
	{
		cc_set_errno(CC_EBADF);
		return (cc_FileDesc *)NULL;
	}
	return desc;
}

/*
 * Resolve a FILE* against StreamFiles.  Sets EINVAL for NULL stream,
 * EBADF if not a live session stream.
 */
cc_FILE *
cc_file_from_list(cc_task_UserData *ud, APTR stream)
{
	cc_FILE *fp;

	if (ud == (cc_task_UserData *)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (cc_FILE *)NULL;
	}
	if (stream == (APTR)NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (cc_FILE *)NULL;
	}
	fp = (cc_FILE *)stream;
	if (!cc_list_has_data(&ud->StreamFiles, (APTR)fp))
	{
		cc_set_errno(CC_EBADF);
		return (cc_FILE *)NULL;
	}
	if (fp->magic != CC_FP_MAGIC)
	{
		cc_set_errno(CC_EBADF);
		return (cc_FILE *)NULL;
	}
	return fp;
}

/*
 * malloc user pointer -> MemBlk if that arena is on ud->RAM.
 * Layout: [cc_MemBlk][ULONG nbytes][user...].
 */
cc_MemBlk *
cc_blk_from_malloc_ptr(cc_task_UserData *ud, APTR ptr)
{
	ULONG *hdr;
	cc_MemBlk *blk;
	cc_node *n;

	if (ud == (cc_task_UserData *)NULL || ptr == (APTR)NULL)
		return (cc_MemBlk *)NULL;

	hdr = ((ULONG *)ptr) - 1;
	blk = ((cc_MemBlk *)hdr) - 1;
	n = ud->RAM.first;
	while (n != (cc_node *)NULL)
	{
		if (n == &blk->n)
			return blk;
		n = n->next;
	}
	return (cc_MemBlk *)NULL;
}
