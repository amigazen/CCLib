#ifndef TIME_H
#define TIME_H 1

/*
 * time — calendar / clock routines for CClib.
 * struct tm, time_t, and clock_t come from <libraries/cclib.h>.
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

#ifdef ANSIC

clock_t clock(void);
time_t time(time_t *);
long difftime(time_t,time_t);
time_t mktime(struct tm *);
char *asctime(struct tm *);
char *ctime(time_t *);
struct tm *gmtime(time_t *);
struct tm *localtime(time_t *);
size_t strftime(char *,size_t,char *,struct tm *);

#else

clock_t clock();
time_t time();
long difftime();
time_t mktime();
char *asctime();
char *ctime();
struct tm *gmtime();
struct tm *localtime();
size_t strftime();

#endif

#endif
