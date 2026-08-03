#ifndef LOCALE_H
#define LOCALE_H 1

/*
 * locale.h — POSIX locale (struct lconv in libraries/cclib.h; LVOs via proto).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef LIBRARIES_CCLIB_H
#include <libraries/cclib.h>
#endif

#define LC_ALL      0
#define LC_COLLATE  1
#define LC_CTYPE    2
#define LC_MONETARY 3
#define LC_NUMERIC  4
#define LC_TIME     5
#define LC_MESSAGES 6

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* LOCALE_H */
