#ifndef WCTYPE_H
#define WCTYPE_H 1

/*
 * wctype - wide character classification / case (single-byte wchar).
 * LVOs via proto (locale-aware through ctype helpers).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef STDDEF_H
#include <stddef.h>
#endif

#ifndef WCHAR_H
#include <wchar.h>
#endif

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* WCTYPE_H */
