/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef STAT_H
#define STAT_H 1

/*
 * stat - file status (struct stat in libraries/cclib.h; LVOs via proto).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef LIBRARIES_CCLIB_H
#include <libraries/cclib.h>
#endif

#ifndef STDDEF_H
#include <stddef.h>
#endif

#define ST_DELETE (1L<<0)
#define ST_EXECUTE (1L<<1)
#define ST_WRITE (1L<<2)
#define ST_READ (1L<<3)
#define ST_ARCHIVE (1L<<4)

#define S_IFMT	0x16
#define S_IFDIR 0x04
#define S_IFCHR 0x02
#define S_IFBLK 0x06
#define S_IFREG 0x10

/*
 * NOTE: st_mode / st_nlink / st_atime / st_ctime are only filled by stat_(),
 * for compatibility with older cclib.library versions.
 */

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* STAT_H */
