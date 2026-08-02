#ifndef DIRENT_H
#define DIRENT_H 1

/*
 * dirent — POSIX directory reading for CClib (opendir/readdir/closedir).
 * Historic scdir/scdir_clean remain available for pattern scans.
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef struct cc_DIR DIR;

struct dirent {
	char d_name[108];
};

#ifdef ANSIC
DIR *opendir(char *name);
struct dirent *readdir(DIR *dir);
long closedir(DIR *dir);
#else
DIR *opendir();
struct dirent *readdir();
long closedir();
#endif

#endif /* DIRENT_H */
