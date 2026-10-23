/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

#ifndef ERRNO_H
#define ERRNO_H 1

/*
 * errno - CClib error codes (application errno lives in CRT _main).
 */

extern int errno;

/* Open() can't open the file */
#define ENOENT 1
/* invalid file handle passed to close() */
#define EBADF 2
/* not enough memory to allocate another file handle */
#define ENOMEM 3
/* file already exists */
#define EEXIST 4
/* invalid function number */
#define EINVAL 5
/* no file handles left (should never occur) */
#define EMFILE 6
/* not a console device (should never occur) */
#define ENOTTY 7
/* invalid mode used in open() */
#define EACCES 8
/* math errors (not used by library) */
#define ERANGE 9
#define EDOM 10
#define EINTR 11
/* Resource busy (pthread mutex/cond/sem) */
#define EBUSY 12
/* Try again / would block */
#define EAGAIN 13
/* No such process / thread */
#define ESRCH 14
/* Resource deadlock would occur */
#define EDEADLK 15
/* Connection timed out / timed wait expired */
#define ETIMEDOUT 16
/* Function not implemented */
#define ENOSYS 17
/* Operation not permitted */
#define EPERM 18
/* Value too large for defined data type */
#define EOVERFLOW 19
/* Illegal byte sequence (multibyte) */
#define EILSEQ 20

#endif
