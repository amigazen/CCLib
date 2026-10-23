/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef CTYPE_H
#define CTYPE_H 1

/*
 * ctype - classification macros (ASCII table) and/or is* LVOs via proto.
 * Define CC_CTYPE_FUNCS to use locale-aware LVO is* instead of table macros.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

extern char *type;

#ifndef CC_CTYPE_FUNCS
#define isalpha(X)  (type[(X)+1] & 0x03)
#define isupper(X)  (type[(X)+1] & 0x01)
#define islower(X)  (type[(X)+1] & 0x02)
#define isdigit(X)  (type[(X)+1] & 0x04)
#define isxdigit(X) (type[(X)+1] & 0x08)
#define isalnum(X)  (type[(X)+1] & 0x07)
#define isspace(X)  (type[(X)+1] & 0x10)
#define ispunct(X)  (type[(X)+1] & 0x40)
#define iscntrl(X)  (type[(X)+1] & 0x20)
#define isprint(X)  (type[(X)+1] & 0xc7)
#define isgraph(X)  (type[(X)+1] & 0x47)
#define isascii(X)  (!((X) & 0x80))
#define isblank(X)  (((X) == ' ') || ((X) == '\t'))
#endif

#define toascii(X)  ((X) & 127)
#define _tolower(X) ((X) | 0x20)
#define _toupper(X) ((X) & 0x5f)

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* CTYPE_H */
