#ifndef STAT_H
#define STAT_H 1

/*
 * stat — file status for CClib.
 * struct stat / time_t come from <libraries/cclib.h>.
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

/* st_mtime member is in seconds since Jan 1, 1978 */

/* st_attr member... */

/* file is NOT deletable */
#define ST_DELETE (1L<<0)
/* file is NOT executable */
#define ST_EXECUTE (1L<<1)
/* file is NOT writeable */
#define ST_WRITE (1L<<2)
/* file is NOT readable */
#define ST_READ (1L<<3)
/* file has been archived */
#define ST_ARCHIVE (1L<<4)

/* bits for st_mode... */

 /* all file type bits */
#define S_IFMT	0x16
 /* directory */
#define S_IFDIR 0x04
 /* character special */
#define S_IFCHR 0x02
 /* block special */
#define S_IFBLK 0x06
 /* regular */
#define S_IFREG 0x10

/*
 * NOTE: st_mode / st_nlink / st_atime / st_ctime are only filled by stat_(),
 * for compatibility with older CClib.library versions.
 */

#ifdef ANSIC
long stat(char *,struct stat *);
long stat_(char *,struct stat *);
#else
long stat();
long stat_();
#endif

#endif
