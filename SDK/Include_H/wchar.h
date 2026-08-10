#ifndef WCHAR_H
#define WCHAR_H 1

/*
 * wchar - multibyte / wide conversion (MB_CUR_MAX==1 identity foundation).
 * Classification lives in <wctype.h>.  LVOs via proto.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef STDDEF_H
#include <stddef.h>
#endif

#ifndef WEOF
#define WEOF ((wint_t)(-1))
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* WCHAR_H */
