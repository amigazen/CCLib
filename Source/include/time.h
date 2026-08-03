#ifndef TIME_H
#define TIME_H 1

/*
 * time — calendar / clock for CClib (types in libraries/cclib.h; LVOs via proto).
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

#define CLK_TCK 1
#define CLOCK_TCK CLK_TCK

#ifndef CCLIB_NO_PROTO
#ifndef PROTO_CCLIB_H
#include <proto/cclib.h>
#endif
#endif

#endif /* TIME_H */
