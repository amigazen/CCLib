#ifndef UNISTD_H
#define UNISTD_H 1

/*
 * unistd - path and sleep helpers (CClib LVOs via proto).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* UNISTD_H */
