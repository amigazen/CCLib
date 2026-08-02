/*
 * stdio.c - buffered FILE stream I/O for cclib.library
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "private/cc_internal.h"
#include "cclib_funcs.h"

#define CC_HASBUF   1L
#define CC_NOBUFMEM 2L

/* Parse fopen-style mode string into CC_O_* flags */
static ULONG cc_parse_fopen_mode(STRPTR mode)
{
	ULONG flags;
	char *p;
	LONG plus;

	flags = 0;
	plus = 0;
	if (mode == NULL)
		return flags;

	p = mode;
	while (*p)
	{
		if (*p == '+')
			plus = 1;
		p++;
	}

	switch (mode[0])
	{
	case 'r':
		flags = plus ? (ULONG)CC_O_RDWR : (ULONG)CC_O_RDONLY;
		break;
	case 'w':
		flags = (ULONG)(CC_O_CREAT | CC_O_TRUNC);
		flags |= plus ? (ULONG)CC_O_RDWR : (ULONG)CC_O_WRONLY;
		break;
	case 'a':
		flags = (ULONG)(CC_O_CREAT | CC_O_APPEND);
		flags |= plus ? (ULONG)CC_O_RDWR : (ULONG)CC_O_WRONLY;
		break;
	default:
		flags = (ULONG)CC_O_RDONLY;
		break;
	}
	return flags;
}

/* Allocate a FILE node on StreamFiles and clear it */
static cc_FILE *cc_alloc_file(cc_task_UserData *ud)
{
	cc_FILE *fp;

	fp = (cc_FILE *)add_head((APTR)&ud->StreamFiles, (LONG)sizeof(cc_FILE));
	if (fp == NULL)
	{
		cc_set_errno(CC_ENOMEM);
		return NULL;
	}
	fp->_unit = -1L;
	fp->_bp = NULL;
	fp->_bend = NULL;
	fp->_buff = NULL;
	fp->_flags = 0;
	fp->_bytbuf = 0;
	fp->_buflen = 0;
	fp->_tmpname = NULL;
	return fp;
}

void __ASM__ __SAVE_DS__ getbuff(
	__REG__(a0, APTR stream))
{
	cc_FILE *fp;
	char *buf;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
		return;
	if (fp->_buff != NULL)
		return;

	buf = (char *)AllocMem(CC_BUFSIZ, MEMF_CLEAR);
	if (buf == NULL)
	{
		/* Fall back to the single-byte buffer */
		fp->_buff = &fp->_bytbuf;
		fp->_bp = &fp->_bytbuf;
		fp->_bend = &fp->_bytbuf;
		fp->_buflen = 1;
		fp->_flags &= ~CC_ALLBUF;
		return;
	}

	fp->_buff = buf;
	fp->_bp = buf;
	fp->_bend = buf;
	fp->_buflen = (short)CC_BUFSIZ;
	fp->_flags |= CC_ALLBUF;
}

LONG __ASM__ __SAVE_DS__ setvbuf(
	__REG__(a0, APTR stream),
	__REG__(a1, STRPTR buf),
	__REG__(d0, LONG mode),
	__REG__(d1, ULONG size))
{
	cc_FILE *fp;
	char *nb;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	if (fp->_buff != NULL && (fp->_flags & CC_ALLBUF))
	{
		/* Already has a library-owned buffer */
		return CC_HASBUF;
	}
	if (fp->_buff != NULL && fp->_buff != &fp->_bytbuf)
	{
		return CC_HASBUF;
	}

	fflush(fp);

	if (mode == CC_IONBF || size == 0UL)
	{
		fp->_buff = &fp->_bytbuf;
		fp->_bp = &fp->_bytbuf;
		fp->_bend = &fp->_bytbuf;
		fp->_buflen = 1;
		fp->_flags &= ~CC_ALLBUF;
		return 0L;
	}

	if (buf != NULL)
	{
		fp->_buff = (char *)buf;
		fp->_bp = (char *)buf;
		fp->_bend = (char *)buf;
		fp->_buflen = (short)size;
		fp->_flags &= ~CC_ALLBUF;
		return 0L;
	}

	nb = (char *)AllocMem(size, MEMF_CLEAR);
	if (nb == NULL)
		return CC_NOBUFMEM;

	fp->_buff = nb;
	fp->_bp = nb;
	fp->_bend = nb;
	fp->_buflen = (short)size;
	fp->_flags |= CC_ALLBUF;
	return 0L;
}

LONG __ASM__ __SAVE_DS__ fflush(
	__REG__(a0, APTR stream))
{
	cc_FILE *fp;
	LONG n;
	LONG len;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
		return 0L;

	if (!(fp->_flags & CC_DIRTY))
	{
		/* Discard any unread buffered input by seeking back */
		if (fp->_buff != NULL && fp->_bp < fp->_bend)
		{
			len = (LONG)(fp->_bend - fp->_bp);
			lseek(fp->_unit, -len, (ULONG)CC_SEEK_CUR);
			fp->_bp = fp->_buff;
			fp->_bend = fp->_buff;
		}
		return 0L;
	}

	if (fp->_buff == NULL || fp->_bp <= fp->_buff)
	{
		fp->_flags &= ~CC_DIRTY;
		return 0L;
	}

	len = (LONG)(fp->_bp - fp->_buff);
	n = write(fp->_unit, (APTR)fp->_buff, len);
	fp->_bp = fp->_buff;
	fp->_bend = fp->_buff;
	fp->_flags &= ~CC_DIRTY;

	if (n != len)
	{
		fp->_flags |= CC_IOERR;
		return (LONG)CC_EOF;
	}
	return 0L;
}

APTR __ASM__ __SAVE_DS__ fdopen(
	__REG__(d0, LONG fd),
	__REG__(a0, STRPTR mode))
{
	cc_task_UserData *ud;
	cc_FILE *fp;

	(void)mode;

	ud = cc_get_ud();
	if (ud == NULL || fd == 0L || fd == -1L)
	{
		cc_set_errno(CC_EINVAL);
		return NULL;
	}

	fp = cc_alloc_file(ud);
	if (fp == NULL)
		return NULL;

	fp->_unit = fd;
	return (APTR)fp;
}

APTR __ASM__ __SAVE_DS__ fopen(
	__REG__(a0, STRPTR name),
	__REG__(a1, STRPTR mode))
{
	ULONG flags;
	LONG fd;
	APTR fp;

	if (name == NULL || mode == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return NULL;
	}

	flags = cc_parse_fopen_mode(mode);
	fd = open(name, flags, 0UL);
	if (fd == -1L)
		return NULL;

	fp = fdopen(fd, mode);
	if (fp == NULL)
	{
		close(fd);
		return NULL;
	}
	return fp;
}

APTR __ASM__ __SAVE_DS__ freopen(
	__REG__(a0, STRPTR name),
	__REG__(a1, STRPTR mode),
	__REG__(a2, APTR stream))
{
	cc_FILE *fp;
	ULONG flags;
	LONG fd;
	char *oldtmp;

	fp = (cc_FILE *)stream;
	if (fp == NULL || name == NULL || mode == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return NULL;
	}

	oldtmp = fp->_tmpname;
	fp->_tmpname = NULL;

	fflush(fp);
	if (fp->_unit != -1L && fp->_unit != 0L)
		close(fp->_unit);

	if ((fp->_flags & CC_ALLBUF) && fp->_buff != NULL && fp->_buff != &fp->_bytbuf)
		FreeMem(fp->_buff, (ULONG)fp->_buflen);

	if (oldtmp != NULL)
	{
		DeleteFile(oldtmp);
		FreeMem(oldtmp, 32UL);
	}

	fp->_buff = NULL;
	fp->_bp = NULL;
	fp->_bend = NULL;
	fp->_flags = 0;
	fp->_buflen = 0;
	fp->_unit = -1L;

	flags = cc_parse_fopen_mode(mode);
	fd = open(name, flags, 0UL);
	if (fd == -1L)
		return NULL;

	fp->_unit = fd;
	return (APTR)fp;
}

LONG __ASM__ __SAVE_DS__ fclose(
	__REG__(a0, APTR stream))
{
	cc_task_UserData *ud;
	cc_FILE *fp;
	LONG rc;
	cc_node *node;
	ULONG *hp;

	ud = cc_get_ud();
	fp = (cc_FILE *)stream;
	if (ud == NULL || fp == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	rc = fflush(fp);

	if (fp->_unit != -1L && fp->_unit != 0L)
	{
		if (close(fp->_unit) != 0L)
			rc = -1L;
	}
	fp->_unit = -1L;

	if ((fp->_flags & CC_ALLBUF) && fp->_buff != NULL && fp->_buff != &fp->_bytbuf)
		FreeMem(fp->_buff, (ULONG)fp->_buflen);

	if (fp->_tmpname != NULL)
	{
		if (fp->_flags & CC_TEMP)
			DeleteFile(fp->_tmpname);
		FreeMem(fp->_tmpname, 32UL);
		fp->_tmpname = NULL;
	}

	node = (cc_node *)remv_node((APTR)&ud->StreamFiles, (APTR)fp);
	if (node != NULL)
	{
		hp = ((ULONG *)node) - 1;
		FreeMem(hp, *hp);
	}
	return rc;
}

LONG __ASM__ __SAVE_DS__ getc(
	__REG__(a0, APTR stream))
{
	cc_FILE *fp;
	LONG n;
	UBYTE ch;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (LONG)CC_EOF;
	}

	if (fp->_flags & CC_EOFBIT)
		return (LONG)CC_EOF;

	if (fp->_flags & CC_DIRTY)
	{
		if (fflush(fp) == (LONG)CC_EOF)
			return (LONG)CC_EOF;
	}

	if (fp->_buff == NULL)
		getbuff(fp);

	if (fp->_bp < fp->_bend)
	{
		ch = (UBYTE)*fp->_bp;
		fp->_bp++;
		return (LONG)ch;
	}

	/* Refill */
	n = read(fp->_unit, (APTR)fp->_buff, (LONG)fp->_buflen);
	if (n <= 0L)
	{
		fp->_flags |= CC_EOFBIT;
		if (n < 0L)
			fp->_flags |= CC_IOERR;
		fp->_bp = fp->_buff;
		fp->_bend = fp->_buff;
		return (LONG)CC_EOF;
	}

	fp->_bp = fp->_buff;
	fp->_bend = fp->_buff + n;
	ch = (UBYTE)*fp->_bp;
	fp->_bp++;
	return (LONG)ch;
}

LONG __ASM__ __SAVE_DS__ putc(
	__REG__(d0, LONG c),
	__REG__(a0, APTR stream))
{
	cc_FILE *fp;
	LONG leftover;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return (LONG)CC_EOF;
	}

	if (fp->_buff == NULL)
		getbuff(fp);

	/* Switching from read to write: rewind unused lookahead */
	if (!(fp->_flags & CC_DIRTY) && fp->_bp < fp->_bend)
	{
		leftover = (LONG)(fp->_bend - fp->_bp);
		lseek(fp->_unit, -leftover, (ULONG)CC_SEEK_CUR);
		fp->_bp = fp->_buff;
		fp->_bend = fp->_buff;
	}

	if (fp->_bp >= fp->_buff + fp->_buflen)
	{
		if (fflush(fp) == (LONG)CC_EOF)
			return (LONG)CC_EOF;
	}

	*fp->_bp = (char)c;
	fp->_bp++;
	fp->_flags |= CC_DIRTY;
	fp->_flags &= ~CC_EOFBIT;

	/* Line-buffered interactive streams flush on newline */
	if ((char)c == '\n')
	{
		if (isatty(fp->_unit))
		{
			if (fflush(fp) == (LONG)CC_EOF)
				return (LONG)CC_EOF;
		}
	}

	/* Unbuffered: flush every character */
	if (fp->_buflen <= 1)
	{
		if (fflush(fp) == (LONG)CC_EOF)
			return (LONG)CC_EOF;
	}

	return c & 0xFFL;
}

LONG __ASM__ __SAVE_DS__ aputc(
	__REG__(d0, LONG c),
	__REG__(a0, APTR stream))
{
	/* Lattice name; same as putc. */
	return putc(c, stream);
}

LONG __ASM__ __SAVE_DS__ agetc(
	__REG__(a0, APTR stream))
{
	/* Lattice name; same as getc. */
	return getc(stream);
}

LONG __ASM__ __SAVE_DS__ getchar(void)
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud == NULL || ud->stdin_p == NULL || *ud->stdin_p == NULL)
		return (LONG)CC_EOF;
	return getc((APTR)*ud->stdin_p);
}

LONG __ASM__ __SAVE_DS__ ungetc(
	__REG__(d0, LONG c),
	__REG__(a0, APTR stream))
{
	cc_FILE *fp;

	fp = (cc_FILE *)stream;
	if (fp == NULL || c == (LONG)CC_EOF)
		return (LONG)CC_EOF;

	if (fp->_buff == NULL)
		getbuff(fp);

	/*
	 * Prefer backing up one byte in the existing read buffer. After fseek
	 * the buffer is empty (_bp == _bend == _buff); plant the pushback as
	 * a one-byte lookahead so getc returns it next.
	 */
	if (fp->_bp > fp->_buff)
	{
		fp->_bp--;
		*fp->_bp = (char)c;
		fp->_flags &= ~CC_EOFBIT;
		return c & 0xFFL;
	}

	if (fp->_bp == fp->_bend && fp->_buff != NULL)
	{
		*fp->_buff = (char)c;
		fp->_bp = fp->_buff;
		fp->_bend = fp->_buff + 1;
		fp->_flags &= ~CC_EOFBIT;
		return c & 0xFFL;
	}

	/* No room to push back into a non-empty buffer at its start */
	return (LONG)CC_EOF;
}

LONG __ASM__ __SAVE_DS__ putw(
	__REG__(d0, ULONG w),
	__REG__(a0, APTR stream))
{
	UWORD word;
	LONG r0;
	LONG r1;

	word = (UWORD)w;
	r0 = putc((LONG)(word >> 8), stream);
	if (r0 == (LONG)CC_EOF)
		return (LONG)CC_EOF;
	r1 = putc((LONG)(word & 0xFF), stream);
	if (r1 == (LONG)CC_EOF)
		return (LONG)CC_EOF;
	return (LONG)word;
}

LONG __ASM__ __SAVE_DS__ getw(
	__REG__(a0, APTR stream))
{
	LONG hi;
	LONG lo;

	hi = getc(stream);
	if (hi == (LONG)CC_EOF)
		return (LONG)CC_EOF;
	lo = getc(stream);
	if (lo == (LONG)CC_EOF)
		return (LONG)CC_EOF;
	return ((hi & 0xFFL) << 8) | (lo & 0xFFL);
}

LONG __ASM__ __SAVE_DS__ puts(
	__REG__(a0, STRPTR s))
{
	cc_task_UserData *ud;
	cc_FILE *fp;
	LONG r;

	ud = cc_get_ud();
	if (ud == NULL || ud->stdout_p == NULL || *ud->stdout_p == NULL)
		return (LONG)CC_EOF;

	fp = *ud->stdout_p;
	r = fputs(s, (APTR)fp);
	if (r == (LONG)CC_EOF)
		return (LONG)CC_EOF;
	if (putc((LONG)'\n', (APTR)fp) == (LONG)CC_EOF)
		return (LONG)CC_EOF;
	return 1L;
}

LONG __ASM__ __SAVE_DS__ puterr(
	__REG__(d0, LONG c))
{
	cc_task_UserData *ud;

	ud = cc_get_ud();
	if (ud == NULL || ud->stderr_p == NULL || *ud->stderr_p == NULL)
		return (LONG)CC_EOF;
	return putc(c, (APTR)*ud->stderr_p);
}

ULONG __ASM__ __SAVE_DS__ fwrite(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size),
	__REG__(d1, ULONG nitems),
	__REG__(a1, APTR stream))
{
	UBYTE *p;
	ULONG total;
	ULONG i;
	LONG c;

	if (ptr == NULL || stream == NULL || size == 0UL || nitems == 0UL)
		return 0UL;

	p = (UBYTE *)ptr;
	total = size * nitems;
	for (i = 0UL; i < total; i++)
	{
		c = putc((LONG)p[i], stream);
		if (c == (LONG)CC_EOF)
			return i / size;
	}
	return nitems;
}

ULONG __ASM__ __SAVE_DS__ fread(
	__REG__(a0, APTR ptr),
	__REG__(d0, ULONG size),
	__REG__(d1, ULONG nitems),
	__REG__(a1, APTR stream))
{
	UBYTE *p;
	ULONG total;
	ULONG i;
	LONG c;

	if (ptr == NULL || stream == NULL || size == 0UL || nitems == 0UL)
		return 0UL;

	p = (UBYTE *)ptr;
	total = size * nitems;
	for (i = 0UL; i < total; i++)
	{
		c = getc(stream);
		if (c == (LONG)CC_EOF)
			return i / size;
		p[i] = (UBYTE)c;
	}
	return nitems;
}

LONG __ASM__ __SAVE_DS__ perror(
	__REG__(a0, STRPTR s))
{
	cc_task_UserData *ud;
	cc_FILE *err;
	STRPTR msg;
	LONG e;

	ud = cc_get_ud();
	if (ud == NULL || ud->stderr_p == NULL || *ud->stderr_p == NULL)
		return -1L;

	err = *ud->stderr_p;
	e = cc_get_errno();
	msg = strerror(e);

	if (s != NULL && s[0] != '\0')
	{
		fputs(s, (APTR)err);
		fputs((STRPTR)": ", (APTR)err);
	}
	if (msg != NULL)
		fputs(msg, (APTR)err);
	else
		fputs((STRPTR)"Unknown error", (APTR)err);
	putc((LONG)'\n', (APTR)err);
	return 0L;
}

LONG __ASM__ __SAVE_DS__ fseek(
	__REG__(a0, APTR stream),
	__REG__(d0, LONG offset),
	__REG__(d1, LONG whence))
{
	cc_FILE *fp;
	LONG pos;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	if (fflush(fp) == (LONG)CC_EOF)
		return -1L;

	fp->_bp = fp->_buff;
	fp->_bend = fp->_buff;
	fp->_flags &= ~(CC_EOFBIT | CC_DIRTY);

	pos = lseek(fp->_unit, offset, (ULONG)whence);
	if (pos < 0L)
		return -1L;
	return 0L;
}

LONG __ASM__ __SAVE_DS__ ftell(
	__REG__(a0, APTR stream))
{
	cc_FILE *fp;
	LONG pos;
	LONG adj;

	fp = (cc_FILE *)stream;
	if (fp == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}

	pos = lseek(fp->_unit, 0L, (ULONG)CC_SEEK_CUR);
	if (pos < 0L)
		return -1L;

	if (fp->_flags & CC_DIRTY)
	{
		adj = (LONG)(fp->_bp - fp->_buff);
		pos += adj;
	}
	else if (fp->_buff != NULL && fp->_bp < fp->_bend)
	{
		adj = (LONG)(fp->_bend - fp->_bp);
		pos -= adj;
	}
	return pos;
}

LONG __ASM__ __SAVE_DS__ fgetpos(
	__REG__(a0, APTR stream),
	__REG__(a1, APTR pos))
{
	LONG p;

	if (pos == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	p = ftell(stream);
	if (p < 0L)
		return -1L;
	*(LONG *)pos = p;
	return 0L;
}

LONG __ASM__ __SAVE_DS__ fsetpos(
	__REG__(a0, APTR stream),
	__REG__(a1, APTR pos))
{
	if (pos == NULL)
	{
		cc_set_errno(CC_EINVAL);
		return -1L;
	}
	return fseek(stream, *(LONG *)pos, CC_SEEK_SET);
}

STRPTR __ASM__ __SAVE_DS__ fgets(
	__REG__(a0, STRPTR s),
	__REG__(d0, LONG n),
	__REG__(a1, APTR stream))
{
	STRPTR p;
	LONG c;
	LONG left;

	if (s == NULL || stream == NULL || n <= 0L)
		return NULL;

	p = s;
	left = n - 1L;
	while (left > 0L)
	{
		c = getc(stream);
		if (c == (LONG)CC_EOF)
		{
			if (p == s)
				return NULL;
			break;
		}
		*p++ = (char)c;
		left--;
		if (c == (LONG)'\n')
			break;
	}
	*p = '\0';
	return s;
}

STRPTR __ASM__ __SAVE_DS__ gets(
	__REG__(a0, STRPTR s))
{
	cc_task_UserData *ud;
	STRPTR p;
	LONG c;

	ud = cc_get_ud();
	if (ud == NULL || ud->stdin_p == NULL || *ud->stdin_p == NULL || s == NULL)
		return NULL;

	p = s;
	for (;;)
	{
		c = getc((APTR)*ud->stdin_p);
		if (c == (LONG)CC_EOF)
		{
			if (p == s)
				return NULL;
			break;
		}
		if (c == (LONG)'\n')
			break;
		*p++ = (char)c;
	}
	*p = '\0';
	return s;
}

LONG __ASM__ __SAVE_DS__ fputs(
	__REG__(a0, STRPTR s),
	__REG__(a1, APTR stream))
{
	STRPTR p;
	LONG n;

	if (s == NULL || stream == NULL)
		return (LONG)CC_EOF;

	p = s;
	n = 0L;
	while (*p)
	{
		if (putc((LONG)(UBYTE)*p, stream) == (LONG)CC_EOF)
			return (LONG)CC_EOF;
		p++;
		n++;
	}
	return n > 0L ? n : 1L;
}

/* Replace trailing X's in a template with digits from a seed */
static void cc_fill_Xs(STRPTR template, ULONG seed)
{
	STRPTR p;
	STRPTR end;
	ULONG v;

	if (template == NULL)
		return;

	end = template;
	while (*end)
		end++;

	p = end;
	v = seed;
	while (p > template)
	{
		p--;
		if (*p == 'X')
		{
			*p = (char)('0' + (v % 10UL));
			v /= 10UL;
		}
	}
}

STRPTR __ASM__ __SAVE_DS__ mktemp(
	__REG__(a0, STRPTR template))
{
	struct Process *pr;
	struct DateStamp ds;
	ULONG seed;
	LONG i;
	BPTR lock;

	if (template == NULL)
		return NULL;

	pr = (struct Process *)FindTask(NULL);
	DateStamp(&ds);
	seed = (ULONG)(LONG)pr;
	seed ^= (ULONG)ds.ds_Days;
	seed ^= ((ULONG)ds.ds_Minute << 8);
	seed ^= (ULONG)ds.ds_Tick;

	for (i = 0L; i < 10000L; i++)
	{
		cc_fill_Xs(template, seed + (ULONG)i);
		lock = Lock(template, ACCESS_READ);
		if (lock == NULL)
			return template;
		UnLock(lock);
	}
	/* Exhausted attempts — leave a null template marker */
	template[0] = '\0';
	return template;
}

STRPTR __ASM__ __SAVE_DS__ tmpnam(
	__REG__(a0, STRPTR s))
{
	cc_task_UserData *ud;
	STRPTR dest;
	static char fallback[32];

	ud = cc_get_ud();
	if (s != NULL)
		dest = s;
	else if (ud != NULL && ud->tempname != NULL)
		dest = ud->tempname;
	else
		dest = fallback;

	strcpy(dest, (STRPTR)"T:ccXXXXXX");
	mktemp(dest);
	if (ud != NULL)
		ud->tempname = dest;
	return dest;
}

APTR __ASM__ __SAVE_DS__ tmpfile(void)
{
	cc_FILE *fp;
	char name[32];
	char *store;
	APTR stream;

	strcpy(name, (STRPTR)"T:ccXXXXXX");
	mktemp(name);
	if (name[0] == '\0')
		return NULL;

	stream = fopen(name, (STRPTR)"w+");
	if (stream == NULL)
		return NULL;

	fp = (cc_FILE *)stream;
	store = (char *)AllocMem(32UL, MEMF_CLEAR);
	if (store != NULL)
	{
		strcpy(store, name);
		fp->_tmpname = store;
		fp->_flags |= CC_TEMP;
	}
	return stream;
}
