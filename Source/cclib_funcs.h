/*
 * cclib_funcs.h - LVO declarations for cclib.library sources
 *
 * Register layout matches SDK/SFD/cclib_lib.sfd and FuncTab[] in StartUp.c.
 */

#ifndef CCLIB_FUNCS_H
#define CCLIB_FUNCS_H

#include <exec/types.h>
#include <dos/dos.h>
#include <sys/types.h>
#include <clib/compiler-specific.h>

struct Library *__ASM__ __SAVE_DS__ LibInit(
	__REG__(a6, struct ExecBase *sysbase),
	__REG__(a0, APTR seglist),
	__REG__(d0, struct Library *base));
struct Library *__ASM__ __SAVE_DS__ LibOpen(
	__REG__(a6, struct Library *base));
APTR __ASM__ __SAVE_DS__ LibClose(
	__REG__(a6, struct Library *base));
APTR __ASM__ __SAVE_DS__ LibExpunge(
	__REG__(a6, struct Library *base));
LONG __ASM__ LibReserved(void);

LONG __ASM__ __SAVE_DS__ SetSTDIO(void);

APTR __ASM__ __SAVE_DS__ GetSTDIO(void);

void __ASM__ __SAVE_DS__ ClearSTDIO(void);

void __ASM__ __SAVE_DS__ closeall(void);

void __ASM__ __SAVE_DS__ SetAbortFunc(
	__REG__(a0, APTR abortfunc));

void __ASM__ __SAVE_DS__ ClearAbortFunc(void);

LONG __ASM__ __SAVE_DS__ SetupSTDIO(
	__REG__(a0, APTR stdin_p),
	__REG__(a1, APTR stdout_p),
	__REG__(a2, APTR stderr_p),
	__REG__(a3, APTR errno_p),
	__REG__(a4, APTR blocksize_p),
	__REG__(a5, APTR type_p),
	__REG__(d0, APTR mathbase),
	__REG__(d1, LONG alen),
	__REG__(d2, STRPTR aptr),
	__REG__(d3, APTR wbm),
	__REG__(d4, APTR exitfunc));

STRPTR __ASM__ __SAVE_DS__ strerror(
	__REG__(d0, LONG err));

LONG __ASM__ __SAVE_DS__ open(
	__REG__(a0, STRPTR name),
	__REG__(d0, ULONG mode),
	__REG__(d1, ULONG perms));

LONG __ASM__ __SAVE_DS__ close(
	__REG__(d0, LONG fd));

LONG __ASM__ __SAVE_DS__ remove(
	__REG__(a0, STRPTR name));

LONG __ASM__ __SAVE_DS__ stat(
	__REG__(a0, STRPTR name),
	__REG__(a1, APTR buf));

LONG __ASM__ __SAVE_DS__ rename(
	__REG__(a0, STRPTR oldname),
	__REG__(a1, STRPTR newname));

LONG __ASM__ __SAVE_DS__ read(
	__REG__(d0, LONG fd),
	__REG__(a0, APTR buf),
	__REG__(d1, LONG len));

LONG __ASM__ __SAVE_DS__ write(
	__REG__(d0, LONG fd),
	__REG__(a0, APTR buf),
	__REG__(d1, LONG len));

LONG __ASM__ __SAVE_DS__ creat(
	__REG__(a0, STRPTR name),
	__REG__(d0, ULONG perms));

LONG __ASM__ __SAVE_DS__ lseek(
	__REG__(d0, LONG fd),
	__REG__(d1, LONG offset),
	__REG__(d2, ULONG whence));

LONG __ASM__ __SAVE_DS__ isatty(
	__REG__(d0, LONG fd));

LONG __ASM__ __SAVE_DS__ access(
	__REG__(a0, STRPTR name),
	__REG__(d0, LONG mode));

APTR __ASM__ __SAVE_DS__ fopen(
	__REG__(a0, STRPTR name),
	__REG__(a1, STRPTR mode));

APTR __ASM__ __SAVE_DS__ freopen(
	__REG__(a0, STRPTR name),
	__REG__(a1, STRPTR mode),
	__REG__(a2, APTR stream));

LONG __ASM__ __SAVE_DS__ fclose(
	__REG__(a0, APTR stream));

APTR __ASM__ __SAVE_DS__ fdopen(
	__REG__(d0, LONG fd),
	__REG__(a0, STRPTR mode));

void __ASM__ __SAVE_DS__ getbuff(
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ setvbuf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR buf),
	__REG__(d0, LONG mode),
	__REG__(d1, ULONG size));

STRPTR __ASM__ __SAVE_DS__ mktemp(
	__REG__(a0, STRPTR template));

STRPTR __ASM__ __SAVE_DS__ tmpnam(
	__REG__(a0, STRPTR s));

APTR __ASM__ __SAVE_DS__ tmpfile(void);

LONG __ASM__ __SAVE_DS__ putc(
	__REG__(d0, LONG c),
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ aputc(
	__REG__(d0, LONG c),
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ getc(
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ agetc(
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ getchar(void);

LONG __ASM__ __SAVE_DS__ ungetc(
	__REG__(d0, LONG c),
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ putw(
	__REG__(d0, ULONG w),
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ puts(
	__REG__(a0, STRPTR s));

LONG __ASM__ __SAVE_DS__ puterr(
	__REG__(d0, LONG c));

ULONG __ASM__ __SAVE_DS__ fwrite(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size),
	__REG__(d1, ULONG nitems),
	__REG__(a1, APTR stream));

void __ASM__ __SAVE_DS__ perror(
	__REG__(a0, STRPTR s));

LONG __ASM__ __SAVE_DS__ fseek(
	__REG__(a0, APTR stream),
	__REG__(d0, LONG offset),
	__REG__(d1, LONG whence));

LONG __ASM__ __SAVE_DS__ ftell(
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ fflush(
	__REG__(a0, APTR stream));

LONG __ASM__ __SAVE_DS__ getw(
	__REG__(a0, APTR stream));

ULONG __ASM__ __SAVE_DS__ fread(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size),
	__REG__(d1, ULONG nitems),
	__REG__(a1, APTR stream));

STRPTR __ASM__ __SAVE_DS__ gets(
	__REG__(a0, STRPTR s));

STRPTR __ASM__ __SAVE_DS__ fgets(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG n),
	__REG__(a1, APTR stream));

LONG __ASM__ __SAVE_DS__ fputs(
	__REG__(a0, STRPTR s),
	__REG__(a1, APTR stream));

LONG __ASM__ __SAVE_DS__ scanf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args));

LONG __ASM__ __SAVE_DS__ printf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args));

LONG __ASM__ __SAVE_DS__ fscanf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

LONG __ASM__ __SAVE_DS__ fprintf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

LONG __ASM__ __SAVE_DS__ sscanf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

LONG __ASM__ __SAVE_DS__ sprintf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

APTR __ASM__ __SAVE_DS__ _alloc(
	__REG__(a0, APTR ud),
	__REG__(d0, LONG size),
	__REG__(d1, LONG flags));

void __ASM__ __SAVE_DS__ _fre(
	__REG__(a0, APTR ud),
	__REG__(a1, APTR ptr));

void __ASM__ __SAVE_DS__ _freall(
	__REG__(a0, APTR ud));

APTR __ASM__ __SAVE_DS__ sbrk(
	__REG__(d0, LONG incr));

APTR __ASM__ __SAVE_DS__ heap_alloc(
	__REG__(d0, ULONG size));

void __ASM__ __SAVE_DS__ heap_free(
	__REG__(a0, APTR ptr));

APTR __ASM__ __SAVE_DS__ malloc(
	__REG__(d0, ULONG size));

APTR __ASM__ __SAVE_DS__ calloc(
	__REG__(d0, ULONG nelem),
	__REG__(d1, ULONG elsize));

APTR __ASM__ __SAVE_DS__ realloc(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size));

void __ASM__ __SAVE_DS__ free(
	__REG__(a0, APTR ptr));

void __ASM__ __SAVE_DS__ be_free(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size));

ULONG __ASM__ __SAVE_DS__ malloc_size(
	__REG__(a0, APTR ptr));

void __ASM__ __SAVE_DS__ freeall(void);

APTR __ASM__ __SAVE_DS__ inst_node(
	__REG__(d0, LONG size));

void __ASM__ __SAVE_DS__ put_head(
	__REG__(a0, APTR list),
	__REG__(a1, APTR node));

APTR __ASM__ __SAVE_DS__ add_head(
	__REG__(a0, APTR list),
	__REG__(d0, LONG size));

void __ASM__ __SAVE_DS__ put_tail(
	__REG__(a0, APTR list),
	__REG__(a1, APTR node));

APTR __ASM__ __SAVE_DS__ add_tail(
	__REG__(a0, APTR list),
	__REG__(d0, LONG size));

APTR __ASM__ __SAVE_DS__ remv_head(
	__REG__(a0, APTR list),
	__REG__(a1, APTR datap));

APTR __ASM__ __SAVE_DS__ remv_tail(
	__REG__(a0, APTR list),
	__REG__(a1, APTR datap));

void __ASM__ __SAVE_DS__ remv_list(
	__REG__(a0, APTR list));

APTR __ASM__ __SAVE_DS__ r_node(
	__REG__(a0, APTR list),
	__REG__(a1, APTR node),
	__REG__(a2, APTR datap));

APTR __ASM__ __SAVE_DS__ remv_node(
	__REG__(a0, APTR list),
	__REG__(a1, APTR data));

LONG __ASM__ __SAVE_DS__ atoi(
	__REG__(a0, STRPTR s));

LONG __ASM__ __SAVE_DS__ atol(
	__REG__(a0, STRPTR s));

DOUBLE __ASM__ __SAVE_DS__ atof(
	__REG__(a0, STRPTR s));

void __ASM__ __SAVE_DS__ ltoa(
	__REG__(d0, LONG n),
	__REG__(a0, STRPTR s));

void __ASM__ __SAVE_DS__ stoa(
	__REG__(d0, LONG n),
	__REG__(a0, STRPTR s));

LONG __ASM__ __SAVE_DS__ bcmp(
	__REG__(a0, APTR b1),
	__REG__(a1, APTR b2),
	__REG__(d0, LONG len));

void __ASM__ __SAVE_DS__ bcopy(
	__REG__(a0, APTR src),
	__REG__(a1, APTR dst),
	__REG__(d0, LONG len));

void __ASM__ __SAVE_DS__ bzero(
	__REG__(a0, APTR b),
	__REG__(d0, LONG len));

APTR __ASM__ __SAVE_DS__ memccpy(
	__REG__(a0, APTR dst),
	__REG__(a1, APTR src),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n));

APTR __ASM__ __SAVE_DS__ memchr(
	__REG__(a0, APTR b),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n));

LONG __ASM__ __SAVE_DS__ memcmp(
	__REG__(a0, APTR b1),
	__REG__(a1, APTR b2),
	__REG__(d0, LONG n));

APTR __ASM__ __SAVE_DS__ memcpy(
	__REG__(a0, APTR dst),
	__REG__(a1, APTR src),
	__REG__(d0, LONG n));

APTR __ASM__ __SAVE_DS__ memset(
	__REG__(a0, APTR b),
	__REG__(d0, ULONG c),
	__REG__(d1, LONG n));

void __ASM__ __SAVE_DS__ memswap(
	__REG__(a0, APTR b1),
	__REG__(a1, APTR b2),
	__REG__(d0, LONG n));

STRPTR __ASM__ __SAVE_DS__ index(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c));

STRPTR __ASM__ __SAVE_DS__ rindex(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c));

STRPTR __ASM__ __SAVE_DS__ strupr(
	__REG__(a0, STRPTR s));

STRPTR __ASM__ __SAVE_DS__ stpcrlf(
	__REG__(a0, STRPTR s));

STRPTR __ASM__ __SAVE_DS__ strcat(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src));

STRPTR __ASM__ __SAVE_DS__ strchr(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c));

LONG __ASM__ __SAVE_DS__ strcmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2));

STRPTR __ASM__ __SAVE_DS__ strcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src));

LONG __ASM__ __SAVE_DS__ strcspn(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR reject));

LONG __ASM__ __SAVE_DS__ strlen(
	__REG__(a0, STRPTR s));

STRPTR __ASM__ __SAVE_DS__ strncat(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, LONG n));

LONG __ASM__ __SAVE_DS__ strcasecmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2));

LONG __ASM__ __SAVE_DS__ strncmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2),
	__REG__(d0, LONG n));

STRPTR __ASM__ __SAVE_DS__ strncpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, LONG n));

STRPTR __ASM__ __SAVE_DS__ strpbrk(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR accept));

STRPTR __ASM__ __SAVE_DS__ strrchr(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG c));

LONG __ASM__ __SAVE_DS__ strspn(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR accept));

STRPTR __ASM__ __SAVE_DS__ strstr(
	__REG__(a0, STRPTR haystack),
	__REG__(a1, STRPTR needle));

STRPTR __ASM__ __SAVE_DS__ strtok(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR delim));

STRPTR __ASM__ __SAVE_DS__ strrv(
	__REG__(a0, STRPTR s));

STRPTR __ASM__ __SAVE_DS__ strnrv(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG n));

STRPTR __ASM__ __SAVE_DS__ bcpl_strcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, BPTR src));

void __ASM__ __SAVE_DS__ vquicksort(
	__REG__(d0, ULONG n),
	__REG__(a0, APTR cmp),
	__REG__(a1, APTR swap));

LONG __ASM__ __SAVE_DS__ toupper(
	__REG__(d0, LONG c));

void __ASM__ __SAVE_DS__ quicksort(
	__REG__(d0, ULONG n),
	__REG__(a0, APTR cmp),
	__REG__(a1, APTR swap),
	__REG__(a2, APTR arg));

void __ASM__ __SAVE_DS__ qsort(
	__REG__(a0, APTR base),
	__REG__(d0, ULONG nel),
	__REG__(d1, ULONG width),
	__REG__(a1, APTR compar));

void __ASM__ __SAVE_DS__ cli_parse(
	__REG__(a0, APTR ud),
	__REG__(d0, LONG alen),
	__REG__(a1, STRPTR aptr));

LONG __ASM__ __SAVE_DS__ wb_parse(
	__REG__(a0, APTR ud),
	__REG__(a1, APTR wbm));

LONG __ASM__ __SAVE_DS__ vprintf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args));

LONG __ASM__ __SAVE_DS__ vfprintf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

LONG __ASM__ __SAVE_DS__ vsprintf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

/* vsnprintf LVO; snprintf is SFD ==varargs at the same bias. */
LONG __ASM__ __SAVE_DS__ cc_vsnprintf(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG n),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args));

LONG __ASM__ __SAVE_DS__ fgetpos(
	__REG__(a0, APTR stream),
	__REG__(a1, APTR pos));

LONG __ASM__ __SAVE_DS__ fsetpos(
	__REG__(a0, APTR stream),
	__REG__(a1, APTR pos));

LONG __ASM__ __SAVE_DS__ time(
	__REG__(a0, APTR tloc));

LONG __ASM__ __SAVE_DS__ clock(void);

APTR __ASM__ __SAVE_DS__ gmtime(
	__REG__(a0, APTR timer));

STRPTR __ASM__ __SAVE_DS__ asctime(
	__REG__(a0, APTR tm));

STRPTR __ASM__ __SAVE_DS__ ctime(
	__REG__(a0, APTR timer));

APTR __ASM__ __SAVE_DS__ localtime(
	__REG__(a0, APTR timer));

LONG __ASM__ __SAVE_DS__ mktime(
	__REG__(a0, APTR tm));

DOUBLE __ASM__ __SAVE_DS__ difftime(
	__REG__(d0, LONG time1),
	__REG__(d1, LONG time0));

ULONG __ASM__ __SAVE_DS__ strftime(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG maxsize),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR tm));

STRPTR __ASM__ __SAVE_DS__ scdir(
	__REG__(a0, STRPTR pattern));

void __ASM__ __SAVE_DS__ scdir_clean(void);

LONG __ASM__ __SAVE_DS__ tolower(
	__REG__(d0, LONG c));

LONG __ASM__ __SAVE_DS__ setenv(
	__REG__(a0, STRPTR name),
	__REG__(a1, STRPTR value));

STRPTR __ASM__ __SAVE_DS__ getenv(
	__REG__(a0, STRPTR name));

LONG __ASM__ __SAVE_DS__ stat_(
	__REG__(a0, STRPTR name),
	__REG__(a1, APTR buf));

LONG __ASM__ __SAVE_DS__ wb_exec(
	__REG__(a0, STRPTR filename),
	__REG__(a1, STRPTR procname),
	__REG__(a2, STRPTR toolname),
	__REG__(a3, STRPTR toolwindow),
	__REG__(a4, BPTR curdir),
	__REG__(a5, APTR rport),
	__REG__(d0, LONG stk),
	__REG__(d1, LONG pri),
	__REG__(d2, APTR child));

LONG __ASM__ __SAVE_DS__ wbexec(
	__REG__(a0, STRPTR name),
	__REG__(d0, LONG stk),
	__REG__(d1, LONG pri));

APTR __ASM__ __SAVE_DS__ wbAexec(
	__REG__(a0, STRPTR name),
	__REG__(d0, LONG stk),
	__REG__(d1, LONG pri));

LONG __ASM__ __SAVE_DS__ wait_child(
	__REG__(a0, APTR child));

LONG __ASM__ __SAVE_DS__ wait_children(void);

LONG __ASM__ __SAVE_DS__ signal_child(
	__REG__(a0, APTR child),
	__REG__(d0, ULONG sig));

LONG __ASM__ __SAVE_DS__ kill(
	__REG__(a0, APTR child));

LONG __ASM__ __SAVE_DS__ kill_children(void);

LONG __ASM__ __SAVE_DS__ cliexec(
	__REG__(a0, STRPTR cmd));

LONG __ASM__ __SAVE_DS__ exec(
	__REG__(a0, STRPTR cmd));

APTR __ASM__ __SAVE_DS__ memmove(
	__REG__(a0, APTR dst),
	__REG__(a1, APTR src),
	__REG__(d0, LONG n)
);

APTR __ASM__ __SAVE_DS__ memmem(
	__REG__(a0, APTR haystack),
	__REG__(d0, LONG hlen),
	__REG__(a1, APTR needle),
	__REG__(d1, LONG nlen)
);

LONG __ASM__ __SAVE_DS__ strncasecmp(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2),
	__REG__(d0, LONG n)
);

STRPTR __ASM__ __SAVE_DS__ strdup(
	__REG__(a0, STRPTR s)
);

STRPTR __ASM__ __SAVE_DS__ strndup(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG n)
);

LONG __ASM__ __SAVE_DS__ strnlen(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG maxlen)
);

STRPTR __ASM__ __SAVE_DS__ stpcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src)
);

STRPTR __ASM__ __SAVE_DS__ stpncpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, LONG n)
);

ULONG __ASM__ __SAVE_DS__ strlcpy(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, ULONG size)
);

ULONG __ASM__ __SAVE_DS__ strlcat(
	__REG__(a0, STRPTR dst),
	__REG__(a1, STRPTR src),
	__REG__(d0, ULONG size)
);

STRPTR __ASM__ __SAVE_DS__ strtok_r(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR delim),
	__REG__(a2, APTR saveptr)
);

STRPTR __ASM__ __SAVE_DS__ strlwr(
	__REG__(a0, STRPTR s)
);

LONG __ASM__ __SAVE_DS__ unsetenv(
	__REG__(a0, STRPTR name)
);

LONG __ASM__ __SAVE_DS__ cc_vsnprintf(
	__REG__(a0, STRPTR s),
	__REG__(d0, ULONG n),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args)
);

LONG __ASM__ __SAVE_DS__ vscanf(
	__REG__(a0, STRPTR format),
	__REG__(a1, APTR args)
);

LONG __ASM__ __SAVE_DS__ vfscanf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args)
);

LONG __ASM__ __SAVE_DS__ vsscanf(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR args)
);

LONG __ASM__ __SAVE_DS__ getline(
	__REG__(a0, APTR lineptr),
	__REG__(a1, APTR n),
	__REG__(a2, APTR stream)
);

LONG __ASM__ __SAVE_DS__ getdelim(
	__REG__(a0, APTR lineptr),
	__REG__(a1, APTR n),
	__REG__(d0, LONG delim),
	__REG__(a2, APTR stream)
);

LONG __ASM__ __SAVE_DS__ strtol(
	__REG__(a0, STRPTR s),
	__REG__(a1, APTR endp),
	__REG__(d0, LONG base)
);

ULONG __ASM__ __SAVE_DS__ strtoul(
	__REG__(a0, STRPTR s),
	__REG__(a1, APTR endp),
	__REG__(d0, LONG base)
);

DOUBLE __ASM__ __SAVE_DS__ strtod(
	__REG__(a0, STRPTR s),
	__REG__(a1, APTR endp)
);

LONG __ASM__ __SAVE_DS__ atoll(
	__REG__(a0, STRPTR s)
);

APTR __ASM__ __SAVE_DS__ bsearch(
	__REG__(a0, APTR key),
	__REG__(a1, APTR base),
	__REG__(d0, ULONG nel),
	__REG__(d1, ULONG width),
	__REG__(a2, APTR compar)
);

void __ASM__ __SAVE_DS__ srand(
	__REG__(d0, ULONG seed)
);

LONG __ASM__ __SAVE_DS__ rand(void);

void __ASM__ __SAVE_DS__ div(
	__REG__(a0, APTR result),
	__REG__(d0, LONG numer),
	__REG__(d1, LONG denom)
);

void __ASM__ __SAVE_DS__ ldiv(
	__REG__(a0, APTR result),
	__REG__(d0, LONG numer),
	__REG__(d1, LONG denom)
);

LONG __ASM__ __SAVE_DS__ labs(
	__REG__(d0, LONG n)
);

LONG __ASM__ __SAVE_DS__ mkstemp(
	__REG__(a0, STRPTR template)
);

STRPTR __ASM__ __SAVE_DS__ getcwd(
	__REG__(a0, STRPTR buf),
	__REG__(d0, LONG size)
);

LONG __ASM__ __SAVE_DS__ chdir(
	__REG__(a0, STRPTR path)
);

STRPTR __ASM__ __SAVE_DS__ realpath(
	__REG__(a0, STRPTR path),
	__REG__(a1, STRPTR resolved)
);

LONG __ASM__ __SAVE_DS__ mkdir(
	__REG__(a0, STRPTR path),
	__REG__(d0, LONG mode)
);

LONG __ASM__ __SAVE_DS__ rmdir(
	__REG__(a0, STRPTR path)
);

LONG __ASM__ __SAVE_DS__ chmod(
	__REG__(a0, STRPTR path),
	__REG__(d0, LONG mode)
);

LONG __ASM__ __SAVE_DS__ utime(
	__REG__(a0, STRPTR path),
	__REG__(a1, APTR times)
);

LONG __ASM__ __SAVE_DS__ symlink(
	__REG__(a0, STRPTR target),
	__REG__(a1, STRPTR linkpath)
);

LONG __ASM__ __SAVE_DS__ readlink(
	__REG__(a0, STRPTR path),
	__REG__(a1, STRPTR buf),
	__REG__(d0, LONG bufsiz)
);

APTR __ASM__ __SAVE_DS__ opendir(
	__REG__(a0, STRPTR name)
);

APTR __ASM__ __SAVE_DS__ readdir(
	__REG__(a0, APTR dir)
);

LONG __ASM__ __SAVE_DS__ closedir(
	__REG__(a0, APTR dir)
);

ULONG __ASM__ __SAVE_DS__ sleep(
	__REG__(d0, ULONG seconds)
);

LONG __ASM__ __SAVE_DS__ usleep(
	__REG__(d0, ULONG usec)
);

LONG __ASM__ __SAVE_DS__ nanosleep(
	__REG__(a0, APTR req),
	__REG__(a1, APTR rem)
);

STRPTR __ASM__ __SAVE_DS__ setlocale(
	__REG__(d0, LONG category),
	__REG__(a0, STRPTR name)
);

APTR __ASM__ __SAVE_DS__ localeconv(void);

LONG __ASM__ __SAVE_DS__ strcoll(
	__REG__(a0, STRPTR s1),
	__REG__(a1, STRPTR s2)
);

ULONG __ASM__ __SAVE_DS__ strxfrm(
	__REG__(a0, STRPTR dest),
	__REG__(a1, STRPTR src),
	__REG__(d0, ULONG n)
);

STRPTR __ASM__ __SAVE_DS__ strptime(
	__REG__(a0, STRPTR s),
	__REG__(a1, STRPTR format),
	__REG__(a2, APTR tm)
);

LONG __ASM__ __SAVE_DS__ isalnum(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isalpha(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ iscntrl(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isdigit(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isgraph(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ islower(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isprint(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ ispunct(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isspace(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isupper(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isxdigit(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isblank(
	__REG__(d0, LONG c)
);

LONG __ASM__ __SAVE_DS__ isascii(
	__REG__(d0, LONG c)
);

#endif /* CCLIB_FUNCS_H */
