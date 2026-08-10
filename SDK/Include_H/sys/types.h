#ifndef SYS_TYPES_H
#define SYS_TYPES_H 1

/*
 * sys/types.h - POSIX-ish and OS4-style scalars for CClib (Amiga 32-bit).
 *
 * Shared with Matcha public headers (<matcha/types.h> includes this).
 * LibDescConverter SFD parsing only accepts single-token type names, so
 * multi-word C types are spelled here as typedef identifiers (u_long for
 * unsigned long).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

/*---- POSIX / CRT --------------------------------------------------------*/

#ifndef __SIZE_T
#define __SIZE_T 1
typedef unsigned long size_t;
#endif

#ifndef __SSIZE_T
#define __SSIZE_T 1
typedef long ssize_t;
#endif

#ifndef __MODE_T
#define __MODE_T 1
typedef int mode_t;
#endif

#ifndef __OFF_T
#define __OFF_T 1
typedef long off_t;
#endif

#ifndef __USECONDS_T
#define __USECONDS_T 1
typedef unsigned long useconds_t;
#endif

#ifndef __U_LONG
#define __U_LONG 1
/* Single-token spelling of C unsigned long (for SFD / clib_protos). */
typedef unsigned long u_long;
#endif

#ifndef __DIV_T
#define __DIV_T 1
typedef struct {
	int quot;
	int rem;
} div_t;
#endif

#ifndef __LDIV_T
#define __LDIV_T 1
typedef struct {
	long quot;
	long rem;
} ldiv_t;
#endif

/*---- OS4-style fixed widths (Matcha + new CClib APIs) -------------------*/
/*
 * SAS/C / classic 68k: long and unsigned long are 32-bit.
 * Do not use classic LONG/ULONG names in new public APIs.
 */

#ifndef __INT8
#define __INT8 1
typedef signed char int8;
#endif

#ifndef __UINT8
#define __UINT8 1
typedef unsigned char uint8;
#endif

#ifndef __INT16
#define __INT16 1
typedef short int16;
#endif

#ifndef __UINT16
#define __UINT16 1
typedef unsigned short uint16;
#endif

#ifndef __INT32
#define __INT32 1
typedef long int32;
#endif

#ifndef __UINT32
#define __UINT32 1
typedef unsigned long uint32;
#endif

#ifndef __INT64
#define __INT64 1
#if defined(__GNUC__) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
typedef long long int64;
#else
typedef struct { uint32 hi, lo; } int64;
#endif
#endif

#ifndef __UINT64
#define __UINT64 1
#if defined(__GNUC__) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
typedef unsigned long long uint64;
#else
typedef struct { uint32 hi, lo; } uint64;
#endif
#endif

/* Pointer-sized integers (OS4 IPTR / SIPTR; same role as uintptr_t / intptr_t) */
#ifndef __IPTR
#define __IPTR 1
typedef unsigned long IPTR;
#endif

#ifndef __SIPTR
#define __SIPTR 1
typedef long SIPTR;
#endif

#endif /* SYS_TYPES_H */
