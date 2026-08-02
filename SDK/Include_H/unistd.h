#ifndef UNISTD_H
#define UNISTD_H 1

/*
 * unistd — path, process, and sleep helpers (CClib LVOs).
 * sleep/usleep/nanosleep use timer.device (SendIO / AbortIO).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifdef ANSIC
char *getcwd(char *buf, long size);
long chdir(char *path);
unsigned long sleep(unsigned long seconds);
long usleep(unsigned long usec);
long mkdir(char *path, long mode);
long rmdir(char *path);
long symlink(char *target, char *linkpath);
long readlink(char *path, char *buf, long bufsiz);
#else
char *getcwd();
long chdir();
unsigned long sleep();
long usleep();
long mkdir();
long rmdir();
long symlink();
long readlink();
#endif

#endif /* UNISTD_H */
