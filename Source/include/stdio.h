/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef STDIO_H
#define STDIO_H 1

/*
 * stdio - stream I/O for cclib.library clients.
 *
 * Standard C/POSIX types (int, size_t, FILE *, const char *), unified with
 * the SFD / clib_protos face. This header includes <proto/cclib.h> for LVO
 * pragmas and prototypes. Macros that wrap getc/putc come after that include.
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

#ifndef NULL
#define NULL ((void *)0)
#endif
#define EOF -1

#define BUFSIZ 1024L

#define _BUSY (1<<0)
#define _ALLBUF (1<<1)
#define _DIRTY (1<<2)
#define _EOF (1<<3)
#define _IOERR (1<<4)
#define _TEMP (1<<5)

#define _IOFBF 1L
#define _IOLBF 2L
#define _IONBF 3L
#define HASBUF 1L
#define NOBUFMEM 2L

#define SEEK_SET 0L
#define SEEK_CUR 1L
#define SEEK_END 2L

extern FILE *stdin, *stdout, *stderr;

#ifndef CCLIBNAME
#define CCLIBNAME "cclib.library"
#endif

#define FOPEN_MAX 2147483647L
#define L_tmpnam 30
#define TMP_MAX FOPEN_MAX
#define FILENAME_MAX 107

#ifndef __SYS_ERRLIST
#define __SYS_ERRLIST 1
extern char *sys_errlist[];
#define sys_nerr 9
#endif

#ifdef ANSIC
#ifndef STDARG_H
#include <stdarg.h>
#endif
#endif

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

/* Macros after proto - must not precede fputc/fgetc LVO declarations. */
#ifndef NOMACROS
#define getchar() getc(stdin)
#define putchar(c) putc(c, stdout)
#define feof(STREAM) (((FILE *)(STREAM))->_flags & _EOF)
#define ferror(STREAM) (((FILE *)(STREAM))->_flags & _IOERR)
#define clearerr(STREAM) (((FILE *)(STREAM))->_flags &= ~(_IOERR|_EOF))
#define fileno(STREAM) (((FILE *)(STREAM))->_unit)
#define rewind(STREAM) (fseek((FILE *)(STREAM), 0L, SEEK_SET))
#define setbuf(STREAM,BUFFER) \
	(setvbuf(STREAM, BUFFER, BUFFER ? _IOFBF : _IONBF, BUFSIZ))
#define fgetc(STREAM) ((int)getc(STREAM))
#define fputc(CH,STREAM) ((int)putc((int)(CH), STREAM))
#endif

#endif /* STDIO_H */
