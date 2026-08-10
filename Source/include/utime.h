#ifndef UTIME_H
#define UTIME_H 1

/*
 * utime - set file modification time (dos SetFileDate; Amiga epoch).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

struct utimbuf {
	long actime;  /* access time (ignored on Amiga) */
	long modtime; /* modification time, seconds since 1978-01-01 */
};

#ifdef ANSIC
long utime(char *path, struct utimbuf *times);
long chmod(char *path, long mode);
#else
long utime();
long chmod();
#endif

#endif /* UTIME_H */
