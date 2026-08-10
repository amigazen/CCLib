#ifndef IOLIB_H
#define IOLIB_H 1

/*
 * iolib.h - historic name for CClib per-task userdata and related types.
 *
 * task_UserData, FileDesc, MemBlk, Child, ABORT_FUNC, FILE, tm, etc. are
 * defined in <libraries/cclib.h>. Prefer that header; this file remains for
 * source compatibility with older includes.
 */

#ifndef LIBRARIES_CCLIB_H
#include <libraries/cclib.h>
#endif

#ifndef REGS
#define REGS register
#endif

#endif
