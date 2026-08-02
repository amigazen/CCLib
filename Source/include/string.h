#ifndef STRING_H
#define STRING_H 1

/*
 * string — memory and string routines for CClib.
 * LVOs are register-based via <proto/cclib.h>; C call UX is via stubs/auto-lib.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifdef ANSIC

long bcmp(void *, void *, long);
void bcopy(void *, void *, long);
void bzero(void *, long);
void *memmove(void *, void *, long);
void *memmem(void *, long, void *, long);
void *memccpy(void *, void *, unsigned long, long);
void *memchr(void *, unsigned long, long);
long memcmp(void *, void *, long);
void *memcpy(void *, void *, long);
void *memset(void *, unsigned long, long);
void memswap(void *, void *, long);

char *index(char *, long);
char *rindex(char *, long);
char *strupr(char *);
char *stpcrlf(char *);
char *strcat(char *, char *);
char *strchr(char *, long);
long strcmp(char *, char *);
char *strcpy(char *, char *);
long strcspn(char *, char *);
long strlen(char *);
char *strncat(char *, char *, long);
long strcasecmp(char *, char *);
long strcmpa(char *, char *);
long strncasecmp(char *, char *, long);
long strncmp(char *, char *, long);
char *strncpy(char *, char *, long);
char *strdup(char *);
char *strndup(char *, long);
long strnlen(char *, long);
char *stpcpy(char *, char *);
char *stpncpy(char *, char *, long);
char *strpbrk(char *, char *);
char *strrchr(char *, long);
long strspn(char *, char *);
char *strstr(char *, char *);
char *strtok(char *, char *);
char *strtok_r(char *, char *, char **);
unsigned long strlcpy(char *, char *, unsigned long);
unsigned long strlcat(char *, char *, unsigned long);
char *strlwr(char *);
char *strrv(char *);
char *strnrv(char *, long);
char *bcpl_strcpy(char *, BPTR);
char *strerror(long);
long strcoll(char *, char *);
unsigned long strxfrm(char *, char *, unsigned long);

#else

long bcmp();
void bcopy();
void bzero();
void *memmove();
void *memmem();
void *memccpy();
void *memchr();
long memcmp();
void *memcpy();
void *memset();
void memswap();
char *index();
char *rindex();
char *strupr();
char *stpcrlf();
char *strcat();
char *strchr();
long strcmp();
char *strcpy();
long strcspn();
long strlen();
char *strncat();
long strcasecmp();
long strcmpa();
long strncasecmp();
long strncmp();
char *strncpy();
char *strdup();
char *strndup();
long strnlen();
char *stpcpy();
char *stpncpy();
char *strpbrk();
char *strrchr();
long strspn();
char *strstr();
char *strtok();
char *strtok_r();
unsigned long strlcpy();
unsigned long strlcat();
char *strlwr();
char *strrv();
char *strnrv();
char *bcpl_strcpy();
char *strerror();
long strcoll();
unsigned long strxfrm();

#endif

#endif
