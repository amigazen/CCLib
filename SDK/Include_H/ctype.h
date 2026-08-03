#ifndef CTYPE_H
#define CTYPE_H 1

/*
 * ctype — character classification for cclib.library
 *
 * SetupSTDIO's type pointer is the shared read-only "C" table (cc_ctype_tab).
 * Historic table macros below always use that table (fast ASCII path).
 *
 * Locale-aware classification (Waves 5–6): the is* / isblank / isascii LVOs
 * use locale.library Is* when LC_CTYPE is non-"C".  Define CC_CTYPE_FUNCS
 * before including this header to skip the macros and call those LVOs
 * (via <proto/cclib.h>).  toupper/tolower LVOs are always locale-aware.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

/* this baby lives in cclib.library so don't mess with it */
extern char *type;

#ifndef CC_CTYPE_FUNCS
/* C type macros a-la K&R (ASCII / "C" table only) */
#define isalpha(X)  (type[(X)+1] & 0x03)
#define isupper(X)  (type[(X)+1] & 0x01)
#define islower(X)  (type[(X)+1] & 0x02)
#define isdigit(X)  (type[(X)+1] & 0x04)
#define isxdigit(X) (type[(X)+1] & 0x08)
#define isalnum(X)  (type[(X)+1] & 0x07)
#define isspace(X)  (type[(X)+1] & 0x10)
#define ispunct(X)  (type[(X)+1] & 0x40)
#define iscntrl(X)  (type[(X)+1] & 0x20)
#define isprint(X)  (type[(X)+1] & 0xc7)
#define isgraph(X)  (type[(X)+1] & 0x47)
#define isascii(X)  (!((X) & 0x80))
#define isblank(X)  (((X) == ' ') || ((X) == '\t'))
#endif

#define toascii(X)  ((X) & 127)
#define _tolower(X) ((X) | 0x20)
#define _toupper(X) ((X) & 0x5f)

#ifdef ANSIC
long toupper(long c);
long tolower(long c);
long isalnum(long c);
long isalpha(long c);
long iscntrl(long c);
long isdigit(long c);
long isgraph(long c);
long islower(long c);
long isprint(long c);
long ispunct(long c);
long isspace(long c);
long isupper(long c);
long isxdigit(long c);
long isblank(long c);
long isascii(long c);
#else
long toupper();
long tolower();
long isalnum();
long isalpha();
long iscntrl();
long isdigit();
long isgraph();
long islower();
long isprint();
long ispunct();
long isspace();
long isupper();
long isxdigit();
long isblank();
long isascii();
#endif

#endif
