#ifndef STDLIB_H
#define STDLIB_H 1

/*
 * stdlib — allocation, conversion, and process control for CClib.
 * exit/abort/atexit are provided by the CRT (not library LVOs).
 */

#ifndef ANSIC
#define ANSIC 1
#endif

#define EXIT_SUCCESS 0L
#define EXIT_FAILURE 20L
#define RAND_MAX 32767

#ifndef __SIZE_T
#define __SIZE_T 1
typedef unsigned long size_t;
#endif

typedef struct {
	long quot;
	long rem;
} div_t;

typedef struct {
	long quot;
	long rem;
} ldiv_t;

#ifdef ANSIC

double atof(char *);
long atoi(char *);
long atol(char *);
long atoll(char *);
long strtol(char *, char **, long);
unsigned long strtoul(char *, char **, long);
double strtod(char *, char **);

void *malloc(unsigned long);
void *calloc(unsigned long, unsigned long);
void *realloc(void *,unsigned long);
void free(void *);

void exit(long);
void abort(void);
int atexit(void (*)(void));
extern void (*exit_fcn)(void);
void qsort(void *,unsigned long,unsigned long,long (*)(void *,void *));
void *bsearch(void *, void *, unsigned long, unsigned long,
	long (*)(void *, void *));
long setenv(char *name, char *value);
long unsetenv(char *name);
char *getenv(char *name);

void srand(unsigned long);
long rand(void);
void div(div_t *result, long numer, long denom);
void ldiv(ldiv_t *result, long numer, long denom);
long labs(long);

char *realpath(char *path, char *resolved);

#else

double atof();
short atoi();
long atol();
long atoll();
long strtol();
unsigned long strtoul();
double strtod();
void *malloc();
void *calloc();
void *realloc();
void free();
void exit();
void abort();
int atexit();
extern void (*exit_fcn)();
void qsort();
void *bsearch();
long setenv();
long unsetenv();
char *getenv();
void srand();
long rand();
void div();
void ldiv();
long labs();
char *realpath();

#endif

#define abs(X) (((X) < 0) ? -(X) : (X))
#define system(COMMAND) (Execute(COMMAND,0L,0L))

#endif
