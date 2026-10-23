/*
 * crt/cc_trace.c - panic backtrace from the process seglist
 *
 * LoadSeg discards HUNK_DEBUG, so file:line is read from the executable
 * once at SetupSTDIO (AllocMem, not malloc) only when the file actually
 * contains HUNK_DEBUG or HUNK_SYMBOL.  Otherwise init probes and stops.
 * Panic only raw-Writes.  No SegTracker / FindHit / mapfile.
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include "cc_trace.h"

extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct Library *CCLibBase;
extern struct WBStartup *WBenchMsg;
void __stdargs __saveds exit(LONG code);

ULONG __stdargs cc_trace_sp(void);

#define CC_TRACE_MAXSEG   16
#define CC_TRACE_MAXLINE  4096
#define CC_TRACE_MAXSTR   32768
#define CC_TRACE_MAXFRAME 8
#define CC_TRACE_MAXCAND  40
#define CC_TRACE_MAXFILE  64
#define CC_TRACE_MAXWORD  256
#define CC_TRACE_MAXOMIT  4
#define CC_TRACE_MAXDBG   16384L
#define CC_TRACE_LINE_WIN 1024UL
#define CC_TRACE_SYM_WIN  8192UL

#define CC_HUNK_CODE          1001L
#define CC_HUNK_DATA          1002L
#define CC_HUNK_BSS           1003L
#define CC_HUNK_RELOC32       1004L
#define CC_HUNK_SYMBOL        1008L
#define CC_HUNK_DEBUG         1009L
#define CC_HUNK_END           1010L
#define CC_HUNK_HEADER        1011L
#define CC_HUNK_OVERLAY       1013L
#define CC_HUNK_BREAK         1014L
#define CC_HUNK_DREL32        1015L
#define CC_HUNK_RELOC32SHORT  1020L
#define CC_LINE_MAGIC         0x4C494E45UL

typedef struct
{
	APTR base;
	ULONG size;
	ULONG hunk;
	ULONG is_code;
} cc_trace_seg;

typedef struct
{
	ULONG hunk;
	ULONG offset;
	ULONG line;
	STRPTR name;
} cc_trace_line;

typedef struct
{
	STRPTR leaf;
	ULONG hunk;
	ULONG mino;
	ULONG maxo;
} cc_trace_span;

static cc_trace_seg cc_segs[CC_TRACE_MAXSEG];
static ULONG cc_nseg;
static ULONG cc_code_bits;
static ULONG cc_have_parse;

static cc_trace_line *cc_lines;
static ULONG cc_nline;
static APTR cc_block;
static ULONG cc_blocksz;
static cc_trace_span cc_files[CC_TRACE_MAXFILE];
static ULONG cc_nfile;

static LONG cc_dumping;
static ULONG cc_omit[CC_TRACE_MAXOMIT];
static ULONG cc_nomit;

static void cc_trace_add_line(ULONG hunk, ULONG offset, ULONG line,
	STRPTR name, char *pool, ULONG *poolused);
static STRPTR cc_trace_leaf(STRPTR name);
static LONG cc_trace_name_is(STRPTR name, STRPTR want);

static void
cc_trace_msg(STRPTR msg)
{
	BPTR out;
	LONG n;

	if (DOSBase == (struct DosLibrary *)NULL || msg == (STRPTR)NULL)
		return;
	n = 0L;
	while (msg[n] != '\0')
		n++;
	out = Output();
	if (out != (BPTR)NULL)
		(void)Write(out, msg, n);
}

static void
cc_trace_hex8(ULONG v, char *b)
{
	static const char *d = "0123456789ABCDEF";
	LONG i;

	i = 8;
	while (i > 0)
	{
		i--;
		b[i] = d[(int)(v & 15UL)];
		v >>= 4;
	}
	b[8] = '\0';
}

static void
cc_trace_dec(ULONG v, char *b)
{
	char tmp[12];
	LONG n;
	LONG i;

	if (v == 0UL)
	{
		b[0] = '0';
		b[1] = '\0';
		return;
	}
	n = 0L;
	while (v != 0UL)
	{
		tmp[n] = (char)('0' + (char)(v % 10UL));
		n++;
		v /= 10UL;
	}
	i = 0L;
	while (n > 0L)
	{
		n--;
		b[i] = tmp[n];
		i++;
	}
	b[i] = '\0';
}

static LONG
cc_trace_read32(BPTR fh, ULONG *out)
{
	if (Read(fh, (APTR)out, 4L) != 4L)
		return 0L;
	return 1L;
}

static LONG
cc_trace_read16(BPTR fh, UWORD *out)
{
	if (Read(fh, (APTR)out, 2L) != 2L)
		return 0L;
	return 1L;
}

static LONG
cc_trace_skip(BPTR fh, LONG n)
{
	if (n <= 0L)
		return 1L;
	if (Seek(fh, n, OFFSET_CURRENT) < 0)
		return 0L;
	return 1L;
}

static ULONG
cc_trace_hunkid(ULONG w)
{
	return w & 0x3FFFFFFFUL;
}

static void
cc_trace_walk_seg(BPTR seg)
{
	ULONG *node;
	ULONG totalsz;
	ULONG n;

	n = 0UL;
	while (seg != (BPTR)NULL && n < (ULONG)CC_TRACE_MAXSEG)
	{
		node = (ULONG *)BADDR(seg);
		if (node == (ULONG *)NULL)
			break;
		totalsz = node[-1];
		if (totalsz < 8UL)
			break;
		cc_segs[n].base = (APTR)(node + 1);
		cc_segs[n].size = totalsz - 4UL;
		cc_segs[n].hunk = n;
		cc_segs[n].is_code = 1UL;
		n++;
		seg = (BPTR)node[0];
	}
	cc_nseg = n;
}

static void
cc_trace_snapshot(void)
{
	struct Process *pr;
	struct CommandLineInterface *cli;
	BPTR *sl;

	cc_nseg = 0UL;
	pr = (struct Process *)FindTask((STRPTR)NULL);
	if (pr == (struct Process *)NULL)
		return;

	if (pr->pr_CLI != (BPTR)NULL)
	{
		cli = (struct CommandLineInterface *)BADDR(pr->pr_CLI);
		if (cli != (struct CommandLineInterface *)NULL
		    && cli->cli_Module != (BPTR)NULL)
		{
			cc_trace_walk_seg(cli->cli_Module);
			return;
		}
	}

	if (pr->pr_SegList != (BPTR)NULL)
	{
		sl = (BPTR *)BADDR(pr->pr_SegList);
		if (sl != (BPTR *)NULL && sl[3] != (BPTR)NULL)
			cc_trace_walk_seg(sl[3]);
	}
}

static BPTR
cc_trace_open_exe(void)
{
	char name[108];
	char path[120];
	BPTR fh;
	BPTR dir;
	BPTR old;
	STRPTR leaf;
	struct WBArg *wa;
	LONG i;

	fh = (BPTR)NULL;
	name[0] = '\0';

	if (WBenchMsg != (struct WBStartup *)NULL
	    && WBenchMsg->sm_NumArgs > 0
	    && WBenchMsg->sm_ArgList != (struct WBArg *)NULL)
	{
		wa = WBenchMsg->sm_ArgList;
		if (wa->wa_Lock != (BPTR)NULL && wa->wa_Name != (STRPTR)NULL)
		{
			old = CurrentDir(wa->wa_Lock);
			fh = Open(wa->wa_Name, MODE_OLDFILE);
			(void)CurrentDir(old);
			if (fh != (BPTR)NULL)
				return fh;
		}
	}

	if (DOSBase->dl_lib.lib_Version >= 36)
	{
		(void)GetProgramName(name, (LONG)sizeof(name));
		dir = GetProgramDir();
		leaf = FilePart(name);
		if (dir != (BPTR)NULL && leaf != (STRPTR)NULL && leaf[0] != '\0')
		{
			old = CurrentDir(dir);
			fh = Open(leaf, MODE_OLDFILE);
			if (fh == (BPTR)NULL && name[0] != '\0')
				fh = Open(name, MODE_OLDFILE);
			(void)CurrentDir(old);
			if (fh != (BPTR)NULL)
				return fh;
		}
	}

	if (name[0] != '\0')
	{
		fh = Open(name, MODE_OLDFILE);
		if (fh != (BPTR)NULL)
			return fh;
		leaf = FilePart(name);
		if (leaf != (STRPTR)NULL && leaf[0] != '\0')
		{
			path[0] = 'P';
			path[1] = 'R';
			path[2] = 'O';
			path[3] = 'G';
			path[4] = 'D';
			path[5] = 'I';
			path[6] = 'R';
			path[7] = ':';
			i = 0L;
			while (leaf[i] != '\0' && i < 100L)
			{
				path[8 + i] = leaf[i];
				i++;
			}
			path[8 + i] = '\0';
			fh = Open(path, MODE_OLDFILE);
		}
	}
	return fh;
}

static LONG
cc_trace_skip_reloc32(BPTR fh)
{
	ULONG n;
	ULONG dummy;

	for (;;)
	{
		if (!cc_trace_read32(fh, &n))
			return 0L;
		if (n == 0UL)
			return 1L;
		if (!cc_trace_read32(fh, &dummy))
			return 0L;
		if (n > 0x100000UL)
			return 0L;
		if (!cc_trace_skip(fh, (LONG)(n * 4UL)))
			return 0L;
	}
}

static LONG
cc_trace_skip_reloc16(BPTR fh)
{
	UWORD n;
	UWORD dummy;
	LONG words;

	words = 0L;
	for (;;)
	{
		if (!cc_trace_read16(fh, &n))
			return 0L;
		words++;
		if (n == 0)
		{
			if ((words & 1L) != 0L)
			{
				if (!cc_trace_read16(fh, &dummy))
					return 0L;
			}
			return 1L;
		}
		if (!cc_trace_read16(fh, &dummy))
			return 0L;
		words++;
		if (!cc_trace_skip(fh, (LONG)n * 2L))
			return 0L;
		words += (LONG)n;
	}
}

/* Walk a HUNK_SYMBOL stream without storing names or values. */
static LONG
cc_trace_skip_symbol(BPTR fh)
{
	ULONG n;

	if (!cc_trace_read32(fh, &n))
		return 0L;
	n &= 0x00FFFFFFUL;
	if (n == 0UL)
		return 1L;
	if (n > 32UL)
		return cc_trace_skip(fh, (LONG)(n * 4UL));
	for (;;)
	{
		if (!cc_trace_skip(fh, (LONG)(n * 4UL + 4UL)))
			return 0L;
		if (!cc_trace_read32(fh, &n))
			return 0L;
		n &= 0x00FFFFFFUL;
		if (n == 0UL)
			return 1L;
		if (n > 32UL)
			return cc_trace_skip(fh, (LONG)(n * 4UL + 4UL));
	}
}

static LONG
cc_trace_read_sym(BPTR fh, ULONG n, char *name, ULONG *valp)
{
	ULONG nbytes;
	LONG got;

	nbytes = n * 4UL;
	if (nbytes == 0UL)
		name[0] = '\0';
	else if (nbytes < 256UL)
	{
		got = Read(fh, (APTR)name, (LONG)nbytes);
		if (got != (LONG)nbytes)
			return 0L;
		name[nbytes] = '\0';
	}
	else
	{
		got = Read(fh, (APTR)name, 255L);
		if (got != 255L)
			return 0L;
		name[255] = '\0';
		if (!cc_trace_skip(fh, (LONG)(nbytes - 255UL)))
			return 0L;
	}
	return cc_trace_read32(fh, valp);
}

static LONG
cc_trace_parse_symbol(BPTR fh, ULONG hunk, char *pool, ULONG *poolused)
{
	ULONG n;
	ULONG val;
	ULONG remain;
	char name[256];

	if (!cc_trace_read32(fh, &n))
		return 0L;
	n &= 0x00FFFFFFUL;
	if (n == 0UL)
		return 1L;
	/*
	 * Standard: first long is namelen (1..32).  Some slink builds
	 * prefix a payload size in longs; treat that as a bounded table.
	 */
	if (n > 32UL)
	{
		remain = n;
		while (remain > 0UL)
		{
			if (!cc_trace_read32(fh, &n))
				return 0L;
			remain--;
			n &= 0x00FFFFFFUL;
			if (n == 0UL)
			{
				if (remain > 0UL)
					return cc_trace_skip(fh, (LONG)(remain * 4UL));
				return 1L;
			}
			if (n > 32UL || n + 1UL > remain)
				return cc_trace_skip(fh, (LONG)(remain * 4UL));
			if (!cc_trace_read_sym(fh, n, name, &val))
				return 0L;
			remain -= n + 1UL;
			cc_trace_add_line(hunk, val, 0UL, name, pool, poolused);
		}
		return 1L;
	}

	for (;;)
	{
		if (n == 0UL)
			return 1L;
		if (n > 32UL)
		{
			if (!cc_trace_skip(fh, (LONG)(n * 4UL + 4UL)))
				return 0L;
		}
		else
		{
			if (!cc_trace_read_sym(fh, n, name, &val))
				return 0L;
			cc_trace_add_line(hunk, val, 0UL, name, pool, poolused);
		}
		if (!cc_trace_read32(fh, &n))
			return 0L;
		n &= 0x00FFFFFFUL;
	}
}

static LONG
cc_trace_skip_header(BPTR fh)
{
	ULONG n;
	ULONG first;
	ULONG last;
	ULONG nh;

	if (!cc_trace_read32(fh, &n))
		return 0L;
	if (!cc_trace_skip(fh, (LONG)(n * 4UL)))
		return 0L;
	if (!cc_trace_read32(fh, &n))
		return 0L;
	if (!cc_trace_read32(fh, &first))
		return 0L;
	if (!cc_trace_read32(fh, &last))
		return 0L;
	if (last < first || (last - first) > 256UL)
		return 0L;
	nh = last - first + 1UL;
	return cc_trace_skip(fh, (LONG)(nh * 4UL));
}

static ULONG cc_count;
static ULONG cc_strneed;
static char cc_prevname[256];

static LONG
cc_trace_eq(STRPTR a, STRPTR b)
{
	if (a == (STRPTR)NULL || b == (STRPTR)NULL)
		return (a == b) ? 1L : 0L;
	while (*a != '\0' && *b != '\0' && *a == *b)
	{
		a++;
		b++;
	}
	return (*a == *b) ? 1L : 0L;
}

static void
cc_trace_add_line(ULONG hunk, ULONG offset, ULONG line, STRPTR name,
	char *pool, ULONG *poolused)
{
	ULONG nlen;
	ULONG i;
	STRPTR dst;
	LONG reuse;

	nlen = 0UL;
	reuse = 0L;
	if (name != (STRPTR)NULL)
	{
		while (name[nlen] != '\0')
			nlen++;
		if (cc_trace_eq(name, cc_prevname))
			reuse = 1L;
	}
	if (cc_lines == (cc_trace_line *)NULL)
	{
		if (cc_count < (ULONG)CC_TRACE_MAXLINE)
			cc_count++;
		if (reuse == 0L && nlen > 0UL
		    && cc_strneed + nlen + 1UL <= (ULONG)CC_TRACE_MAXSTR)
		{
			cc_strneed += nlen + 1UL;
			i = 0UL;
			while (i < nlen && i < 255UL)
			{
				cc_prevname[i] = name[i];
				i++;
			}
			cc_prevname[i] = '\0';
		}
		return;
	}
	if (cc_nline >= (ULONG)CC_TRACE_MAXLINE)
		return;
	if (reuse != 0L && cc_nline > 0UL)
		name = cc_lines[cc_nline - 1UL].name;
	else
	{
		i = 0UL;
		while (i < cc_nline)
		{
			if (cc_trace_eq(cc_lines[i].name, name))
			{
				name = cc_lines[i].name;
				reuse = 1L;
				break;
			}
			i++;
		}
		if (reuse == 0L)
		{
			if (*poolused + nlen + 1UL > (ULONG)CC_TRACE_MAXSTR)
				name = (STRPTR)NULL;
			if (name != (STRPTR)NULL && pool != (char *)NULL)
			{
				dst = pool + *poolused;
				i = 0UL;
				while (i < nlen)
				{
					dst[i] = name[i];
					i++;
				}
				dst[nlen] = '\0';
				*poolused += nlen + 1UL;
				name = dst;
				i = 0UL;
				while (i < nlen && i < 255UL)
				{
					cc_prevname[i] = dst[i];
					i++;
				}
				cc_prevname[i] = '\0';
			}
		}
	}
	cc_lines[cc_nline].hunk = hunk;
	cc_lines[cc_nline].offset = offset;
	cc_lines[cc_nline].line = line;
	cc_lines[cc_nline].name = name;
	cc_nline++;
}

static LONG
cc_trace_parse_debug(BPTR fh, ULONG nlongs, ULONG hunk, char *pool,
	ULONG *poolused)
{
	ULONG base;
	ULONG magic;
	ULONG nlen;
	ULONG left;
	ULONG line;
	ULONG off;
	char name[256];
	ULONG i;
	ULONG nbytes;

	if (nlongs < 3UL || nlongs > (ULONG)CC_TRACE_MAXDBG)
		return cc_trace_skip(fh, (LONG)(nlongs * 4UL));
	if (!cc_trace_read32(fh, &base))
		return 0L;
	left = nlongs - 1UL;
	if (base == CC_LINE_MAGIC)
	{
		magic = CC_LINE_MAGIC;
		base = 0UL;
	}
	else
	{
		if (!cc_trace_read32(fh, &magic))
			return 0L;
		left--;
	}
	if (magic != CC_LINE_MAGIC)
		return cc_trace_skip(fh, (LONG)(left * 4UL));
	if (!cc_trace_read32(fh, &nlen))
		return 0L;
	left--;
	nlen &= 0x00FFFFFFUL;
	if (nlen > left)
		return 0L;
	nbytes = nlen * 4UL;
	i = 0UL;
	{
		char ch;

		while (i < nbytes)
		{
			if (Read(fh, (APTR)&ch, 1L) != 1L)
				return 0L;
			if (i < 255UL)
				name[i] = ch;
			i++;
		}
	}
	if (nbytes < 256UL)
		name[nbytes] = '\0';
	else
		name[255] = '\0';
	{
		LONG z;

		z = (LONG)nbytes - 1L;
		while (z >= 0L && name[z] == '\0')
			z--;
		if (z >= 0L && (z + 1L) < 256L)
			name[z + 1L] = '\0';
	}
	left -= nlen;
	while (left >= 2UL)
	{
		if (!cc_trace_read32(fh, &line))
			return 0L;
		if (!cc_trace_read32(fh, &off))
			return 0L;
		left -= 2UL;
		cc_trace_add_line(hunk, off + base, line, name, pool, poolused);
	}
	if (left > 0UL)
		return cc_trace_skip(fh, (LONG)(left * 4UL));
	return 1L;
}

static void
cc_trace_parse_file(BPTR fh, char *pool, ULONG *poolused)
{
	ULONG id;
	ULONG nlongs;
	ULONG hunk;
	ULONG typ;

	hunk = 0UL;
	cc_have_parse = 0UL;
	cc_code_bits = 0UL;
	cc_prevname[0] = '\0';
	while (cc_trace_read32(fh, &id))
	{
		typ = cc_trace_hunkid(id);
		if (typ == (ULONG)CC_HUNK_HEADER)
		{
			if (!cc_trace_skip_header(fh))
				return;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_END)
			continue;
		if (typ == (ULONG)CC_HUNK_OVERLAY || typ == (ULONG)CC_HUNK_BREAK)
			return;
		if (typ == (ULONG)CC_HUNK_CODE || typ == (ULONG)CC_HUNK_DATA
		    || typ == (ULONG)CC_HUNK_BSS)
		{
			if (!cc_trace_read32(fh, &nlongs))
				return;
			nlongs &= 0x00FFFFFFUL;
			if (typ == (ULONG)CC_HUNK_CODE && hunk < 32UL)
				cc_code_bits |= (1UL << hunk);
			if (typ != (ULONG)CC_HUNK_BSS)
			{
				if (!cc_trace_skip(fh, (LONG)(nlongs * 4UL)))
					return;
			}
			hunk++;
			cc_have_parse = 1UL;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_RELOC32)
		{
			if (!cc_trace_skip_reloc32(fh))
				return;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_RELOC32SHORT
		    || typ == (ULONG)CC_HUNK_DREL32)
		{
			if (!cc_trace_skip_reloc16(fh))
				return;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_SYMBOL)
		{
			if (hunk == 0UL)
			{
				if (!cc_trace_parse_symbol(fh, 0UL, pool, poolused))
					return;
			}
			else if (!cc_trace_parse_symbol(fh, hunk - 1UL, pool,
			    poolused))
				return;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_DEBUG)
		{
			if (!cc_trace_read32(fh, &nlongs))
				return;
			if (hunk == 0UL)
			{
				if (!cc_trace_skip(fh, (LONG)(nlongs * 4UL)))
					return;
			}
			else if (!cc_trace_parse_debug(fh, nlongs, hunk - 1UL,
			    pool, poolused))
				return;
			continue;
		}
		/* Unknown: next long is usually a longword count. */
		if (!cc_trace_read32(fh, &nlongs))
			return;
		nlongs &= 0x00FFFFFFUL;
		if (nlongs > (ULONG)CC_TRACE_MAXDBG)
			return;
		if (!cc_trace_skip(fh, (LONG)(nlongs * 4UL)))
			return;
	}
}

/*
 * True if the load file has HUNK_DEBUG or HUNK_SYMBOL.  Skips payloads
 * only — no LINE/symbol tables, no AllocMem.
 */
static LONG
cc_trace_probe(BPTR fh)
{
	ULONG id;
	ULONG nlongs;
	ULONG hunk;
	ULONG typ;
	LONG found;

	found = 0L;
	hunk = 0UL;
	cc_have_parse = 0UL;
	cc_code_bits = 0UL;
	while (cc_trace_read32(fh, &id))
	{
		typ = cc_trace_hunkid(id);
		if (typ == (ULONG)CC_HUNK_HEADER)
		{
			if (!cc_trace_skip_header(fh))
				return found;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_END)
			continue;
		if (typ == (ULONG)CC_HUNK_OVERLAY || typ == (ULONG)CC_HUNK_BREAK)
			return found;
		if (typ == (ULONG)CC_HUNK_CODE || typ == (ULONG)CC_HUNK_DATA
		    || typ == (ULONG)CC_HUNK_BSS)
		{
			if (!cc_trace_read32(fh, &nlongs))
				return found;
			nlongs &= 0x00FFFFFFUL;
			if (typ == (ULONG)CC_HUNK_CODE && hunk < 32UL)
				cc_code_bits |= (1UL << hunk);
			if (typ != (ULONG)CC_HUNK_BSS)
			{
				if (!cc_trace_skip(fh, (LONG)(nlongs * 4UL)))
					return found;
			}
			hunk++;
			cc_have_parse = 1UL;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_RELOC32)
		{
			if (!cc_trace_skip_reloc32(fh))
				return found;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_RELOC32SHORT
		    || typ == (ULONG)CC_HUNK_DREL32)
		{
			if (!cc_trace_skip_reloc16(fh))
				return found;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_SYMBOL)
		{
			found = 1L;
			if (!cc_trace_skip_symbol(fh))
				return found;
			continue;
		}
		if (typ == (ULONG)CC_HUNK_DEBUG)
		{
			found = 1L;
			if (!cc_trace_read32(fh, &nlongs))
				return found;
			if (!cc_trace_skip(fh, (LONG)(nlongs * 4UL)))
				return found;
			continue;
		}
		if (!cc_trace_read32(fh, &nlongs))
			return found;
		nlongs &= 0x00FFFFFFUL;
		if (nlongs > (ULONG)CC_TRACE_MAXDBG)
			return found;
		if (!cc_trace_skip(fh, (LONG)(nlongs * 4UL)))
			return found;
	}
	return found;
}

static void
cc_trace_apply_code_bits(void)
{
	ULONG i;

	if (cc_have_parse == 0UL || cc_code_bits == 0UL)
		return;
	i = 0UL;
	while (i < cc_nseg)
	{
		if (cc_segs[i].hunk < 32UL
		    && (cc_code_bits & (1UL << cc_segs[i].hunk)) == 0UL)
			cc_segs[i].is_code = 0UL;
		i++;
	}
}

typedef struct
{
	ULONG pc;
	ULONG hunk;
	ULONG off;
	ULONG line;
	STRPTR file;
	STRPTR sym;
} cc_trace_fr;

static cc_trace_fr cc_cand[CC_TRACE_MAXCAND];

/* One pass after parse — dump must not walk names O(n^2) per PC. */
static void
cc_trace_build_spans(void)
{
	ULONG i;
	ULONG j;
	STRPTR leaf;
	LONG found;

	cc_nfile = 0UL;
	i = 0UL;
	while (i < cc_nline)
	{
		if (cc_lines[i].line != 0UL && cc_lines[i].name != (STRPTR)NULL)
		{
			leaf = cc_trace_leaf(cc_lines[i].name);
			found = 0L;
			j = 0UL;
			while (j < cc_nfile)
			{
				if (cc_files[j].hunk == cc_lines[i].hunk
				    && cc_trace_name_is(cc_files[j].leaf, leaf))
				{
					if (cc_lines[i].offset < cc_files[j].mino)
						cc_files[j].mino = cc_lines[i].offset;
					if (cc_lines[i].offset > cc_files[j].maxo)
						cc_files[j].maxo = cc_lines[i].offset;
					found = 1L;
					break;
				}
				j++;
			}
			if (found == 0L && cc_nfile < (ULONG)CC_TRACE_MAXFILE)
			{
				cc_files[cc_nfile].leaf = leaf;
				cc_files[cc_nfile].hunk = cc_lines[i].hunk;
				cc_files[cc_nfile].mino = cc_lines[i].offset;
				cc_files[cc_nfile].maxo = cc_lines[i].offset;
				cc_nfile++;
			}
		}
		i++;
	}
}

/* LINE only if this PC sits inside that file's own offset span. */
static cc_trace_line *
cc_trace_lookup_line(ULONG hunk, ULONG offset)
{
	cc_trace_line *best;
	cc_trace_span *own;
	ULONG i;
	ULONG bestoff;
	ULONG spanlen;
	ULONG bestlen;

	own = (cc_trace_span *)NULL;
	bestlen = 0UL;
	i = 0UL;
	while (i < cc_nfile)
	{
		if (cc_files[i].hunk == hunk && offset >= cc_files[i].mino
		    && offset <= cc_files[i].maxo + 64UL)
		{
			spanlen = cc_files[i].maxo - cc_files[i].mino;
			if (own == (cc_trace_span *)NULL || spanlen < bestlen)
			{
				own = &cc_files[i];
				bestlen = spanlen;
			}
		}
		i++;
	}
	if (own == (cc_trace_span *)NULL)
		return (cc_trace_line *)NULL;

	best = (cc_trace_line *)NULL;
	bestoff = 0UL;
	i = 0UL;
	while (i < cc_nline)
	{
		if (cc_lines[i].line != 0UL && cc_lines[i].hunk == hunk
		    && cc_lines[i].offset <= offset
		    && cc_lines[i].offset >= own->mino
		    && cc_lines[i].offset <= own->maxo
		    && (best == (cc_trace_line *)NULL
		        || cc_lines[i].offset >= bestoff))
		{
			best = &cc_lines[i];
			bestoff = cc_lines[i].offset;
		}
		i++;
	}
	return best;
}

static cc_trace_line *
cc_trace_lookup_kind(ULONG hunk, ULONG offset, ULONG wantline, ULONG win,
	LONG anyhunk)
{
	cc_trace_line *best;
	ULONG i;
	ULONG bestoff;
	ULONG delta;

	best = (cc_trace_line *)NULL;
	bestoff = 0UL;
	i = 0UL;
	while (i < cc_nline)
	{
		if (cc_lines[i].line == 0UL)
		{
			if (wantline != 0UL)
			{
				i++;
				continue;
			}
		}
		else if (wantline == 0UL)
		{
			i++;
			continue;
		}
		if (anyhunk == 0L && cc_lines[i].hunk != hunk)
		{
			i++;
			continue;
		}
		if (cc_lines[i].offset > offset)
		{
			i++;
			continue;
		}
		delta = offset - cc_lines[i].offset;
		if (delta > win)
		{
			i++;
			continue;
		}
		if (best == (cc_trace_line *)NULL || cc_lines[i].offset >= bestoff)
		{
			best = &cc_lines[i];
			bestoff = cc_lines[i].offset;
		}
		i++;
	}
	return best;
}

static STRPTR
cc_trace_leaf(STRPTR name)
{
	STRPTR p;
	STRPTR leaf;

	if (name == (STRPTR)NULL)
		return name;
	leaf = name;
	p = name;
	while (*p != '\0')
	{
		if (*p == '/' || *p == ':' || *p == '\\')
			leaf = p + 1;
		p++;
	}
	return leaf;
}

static STRPTR
cc_trace_symleaf(STRPTR name)
{
	STRPTR s;

	s = name;
	if (s == (STRPTR)NULL)
		return s;
	/* One SAS/C prefix only, so _main and __main stay distinct. */
	if (*s == '_' || *s == '@')
		s++;
	return s;
}

static LONG
cc_trace_streq(STRPTR a, STRPTR b)
{
	if (a == (STRPTR)NULL || b == (STRPTR)NULL)
		return 0L;
	while (*a != '\0' && *b != '\0' && *a == *b)
	{
		a++;
		b++;
	}
	return (*a == '\0' && *b == '\0') ? 1L : 0L;
}

static LONG
cc_trace_name_is(STRPTR name, STRPTR want)
{
	return cc_trace_streq(cc_trace_leaf(name), want);
}

static LONG
cc_trace_sym_is(STRPTR name, STRPTR want)
{
	return cc_trace_streq(cc_trace_symleaf(name), want);
}

static LONG
cc_trace_sym_pfx(STRPTR name, STRPTR pfx)
{
	STRPTR a;
	STRPTR b;

	a = cc_trace_symleaf(name);
	b = pfx;
	if (a == (STRPTR)NULL || b == (STRPTR)NULL)
		return 0L;
	while (*b != '\0')
	{
		if (*a != *b)
			return 0L;
		a++;
		b++;
	}
	return 1L;
}

/*
 * This .o is one coagulated run.  Skip PCs in it so the dump does not
 * list its own helpers as the innermost frames.
 */
static LONG
cc_trace_in_self(ULONG pc)
{
	ULONG lo;
	ULONG hi;

	lo = (ULONG)cc_trace_msg;
	hi = (ULONG)cc_crt_abort + 64UL;
	if ((ULONG)cc_trace_init < lo)
		lo = (ULONG)cc_trace_init;
	if ((ULONG)cc_trace_dump < lo)
		lo = (ULONG)cc_trace_dump;
	if (pc >= lo && pc < hi)
		return 1L;
	return 0L;
}

static LONG
cc_trace_omitted(ULONG pc)
{
	ULONG i;

	i = 0UL;
	while (i < cc_nomit)
	{
		if (cc_omit[i] == pc)
			return 1L;
		i++;
	}
	return 0L;
}

static LONG
cc_trace_lib_off(ULONG pc, ULONG *offp)
{
	UBYTE *lib0;
	ULONG libsz;

	if (CCLibBase == (struct Library *)NULL)
		return 0L;
	lib0 = (UBYTE *)CCLibBase - CCLibBase->lib_NegSize;
	libsz = (ULONG)CCLibBase->lib_NegSize + (ULONG)CCLibBase->lib_PosSize;
	if (pc < (ULONG)lib0 || pc >= (ULONG)lib0 + libsz)
		return 0L;
	if (offp != (ULONG *)NULL)
		*offp = pc - (ULONG)lib0;
	return 1L;
}

static cc_trace_seg *
cc_trace_find_seg(ULONG pc)
{
	ULONG i;
	ULONG base;
	ULONG end;

	i = 0UL;
	while (i < cc_nseg)
	{
		if (cc_segs[i].is_code)
		{
			base = (ULONG)cc_segs[i].base;
			end = base + cc_segs[i].size;
			if (pc >= base && pc < end)
				return &cc_segs[i];
		}
		i++;
	}
	return (cc_trace_seg *)NULL;
}

static LONG
cc_trace_boring(cc_trace_fr *fr)
{
	if (fr == (cc_trace_fr *)NULL)
		return 1L;
	if (cc_trace_name_is(fr->file, (STRPTR)"cc_trace.c"))
		return 1L;
	if (cc_trace_name_is(fr->file, (STRPTR)"cctest_util.c"))
		return 1L;
	if (cc_trace_name_is(fr->file, (STRPTR)"cc_crt.c"))
		return 1L;
	if (cc_trace_name_is(fr->file, (STRPTR)"cc_entry.c"))
		return 1L;
	if (cc_trace_sym_is(fr->sym, (STRPTR)"ct_panic_hook"))
		return 1L;
	if (cc_trace_sym_is(fr->sym, (STRPTR)"cc_crt_abort"))
		return 1L;
	if (cc_trace_sym_pfx(fr->sym, (STRPTR)"cc_trace"))
		return 1L;
	return 0L;
}

static LONG
cc_trace_stop(cc_trace_fr *fr)
{
	if (fr == (cc_trace_fr *)NULL)
		return 0L;
	if (cc_trace_name_is(fr->file, (STRPTR)"cc_start.asm"))
		return 1L;
	if (cc_trace_sym_is(fr->sym, (STRPTR)"startup"))
		return 1L;
	if (cc_trace_sym_is(fr->sym, (STRPTR)"_startup"))
		return 1L;
	return 0L;
}

static LONG
cc_trace_fill(ULONG pc, cc_trace_fr *fr)
{
	cc_trace_seg *sg;
	cc_trace_line *ln;
	cc_trace_line *sy;

	if (fr == (cc_trace_fr *)NULL)
		return 0L;
	fr->pc = pc;
	fr->hunk = 0UL;
	fr->off = 0UL;
	fr->line = 0UL;
	fr->file = (STRPTR)NULL;
	fr->sym = (STRPTR)NULL;

	if (cc_trace_in_self(pc) || cc_trace_omitted(pc))
		return 0L;
	if (pc == (ULONG)CCLibBase)
		return 0L;
	/* Library frames are the panic itself; the message already named it. */
	if (cc_trace_lib_off(pc, (ULONG *)NULL))
		return 0L;

	sg = cc_trace_find_seg(pc);
	if (sg == (cc_trace_seg *)NULL)
		return 0L;
	fr->hunk = sg->hunk;
	fr->off = pc - (ULONG)sg->base;
	if (cc_nline > 0UL)
	{
		ln = cc_trace_lookup_line(sg->hunk, fr->off);
		if (ln != (cc_trace_line *)NULL)
		{
			fr->file = ln->name;
			fr->line = ln->line;
		}
		sy = cc_trace_lookup_kind(sg->hunk, fr->off, 0UL,
		    CC_TRACE_SYM_WIN, 0L);
		if (sy == (cc_trace_line *)NULL)
			sy = cc_trace_lookup_kind(sg->hunk, fr->off, 0UL,
			    CC_TRACE_SYM_WIN, 1L);
		if (sy != (cc_trace_line *)NULL)
			fr->sym = sy->name;
	}
	if (cc_trace_boring(fr))
		return 0L;
	return 1L;
}

static void
cc_trace_print_where(cc_trace_fr *fr)
{
	char hex[12];
	char dec[12];
	STRPTR leaf;
	STRPTR sym;

	if (fr == (cc_trace_fr *)NULL)
		return;
	if (fr->sym != (STRPTR)NULL)
	{
		sym = cc_trace_symleaf(fr->sym);
		if (sym != (STRPTR)NULL && *sym != '\0')
		{
			cc_trace_msg(sym);
			cc_trace_msg((STRPTR)"()");
			if (fr->file != (STRPTR)NULL && fr->line != 0UL)
				cc_trace_msg((STRPTR)" in ");
			else
				cc_trace_msg((STRPTR)" at ");
		}
	}
	if (fr->file != (STRPTR)NULL && fr->line != 0UL)
	{
		leaf = cc_trace_leaf(fr->file);
		cc_trace_msg(leaf);
		cc_trace_msg((STRPTR)" line ");
		cc_trace_dec(fr->line, dec);
		cc_trace_msg(dec);
	}
	else
	{
		cc_trace_msg((STRPTR)"hunk ");
		cc_trace_dec(fr->hunk, dec);
		cc_trace_msg(dec);
		cc_trace_msg((STRPTR)" + $");
		cc_trace_hex8(fr->off, hex);
		cc_trace_msg(hex);
	}
	cc_trace_msg((STRPTR)"\n");
}

static void
cc_trace_print_fr(cc_trace_fr *fr, LONG first)
{
	if (fr == (cc_trace_fr *)NULL)
		return;
	if (cc_trace_stop(fr))
	{
		if (first)
			cc_trace_msg((STRPTR)"  (no application call site on the stack)\n");
		else
			cc_trace_msg((STRPTR)"  which was entered from program start\n");
		return;
	}
	if (first)
		cc_trace_msg((STRPTR)"  the call into CClib was:\n    ");
	else
		cc_trace_msg((STRPTR)"  which was called from:\n    ");
	cc_trace_print_where(fr);
}

static LONG
cc_trace_same_fun(cc_trace_fr *a, cc_trace_fr *b)
{
	if (a == (cc_trace_fr *)NULL || b == (cc_trace_fr *)NULL)
		return 0L;
	if (a->sym != (STRPTR)NULL && b->sym != (STRPTR)NULL)
		return cc_trace_eq(a->sym, b->sym);
	if (a->file != (STRPTR)NULL && b->file != (STRPTR)NULL
	    && a->line != 0UL && a->line == b->line
	    && cc_trace_name_is(a->file, cc_trace_leaf(b->file)))
		return 1L;
	return 0L;
}

static ULONG
cc_trace_fun_hits(ULONG ncand, ULONG idx)
{
	ULONG i;
	ULONG n;

	n = 0UL;
	i = 0UL;
	while (i < ncand)
	{
		if (cc_trace_same_fun(&cc_cand[i], &cc_cand[idx]))
			n++;
		i++;
	}
	return n;
}

static LONG
cc_trace_is_main(cc_trace_fr *fr)
{
	if (fr == (cc_trace_fr *)NULL)
		return 0L;
	if (cc_trace_sym_is(fr->sym, (STRPTR)"main"))
		return 1L;
	if (cc_trace_streq(fr->sym, (STRPTR)"_main"))
		return 1L;
	if (cc_trace_streq(fr->sym, (STRPTR)"__main"))
		return 1L;
	return 0L;
}

void
cc_trace_init(void)
{
	BPTR fh;
	ULONG need;
	char *pool;
	ULONG poolused;

	cc_trace_clear();
	cc_trace_snapshot();
	fh = cc_trace_open_exe();
	if (fh == (BPTR)NULL)
		return;
	/* Stripped load file: no LINE/ADDSYM tables, no second pass. */
	if (cc_trace_probe(fh) == 0L)
	{
		Close(fh);
		cc_trace_apply_code_bits();
		return;
	}
	if (Seek(fh, 0L, OFFSET_BEGINNING) < 0)
	{
		Close(fh);
		cc_trace_apply_code_bits();
		return;
	}

	cc_count = 0UL;
	cc_strneed = 0UL;
	poolused = 0UL;
	cc_lines = (cc_trace_line *)NULL;
	cc_trace_parse_file(fh, (char *)NULL, &poolused);
	if (cc_count == 0UL)
	{
		Close(fh);
		cc_trace_apply_code_bits();
		return;
	}

	need = cc_count * sizeof(cc_trace_line) + cc_strneed;
	cc_block = AllocMem(need, MEMF_ANY | MEMF_CLEAR);
	if (cc_block == (APTR)NULL)
	{
		Close(fh);
		cc_trace_apply_code_bits();
		return;
	}
	cc_blocksz = need;
	cc_lines = (cc_trace_line *)cc_block;
	pool = (char *)cc_block + cc_count * sizeof(cc_trace_line);
	poolused = 0UL;
	cc_nline = 0UL;
	(void)Seek(fh, 0L, OFFSET_BEGINNING);
	cc_trace_parse_file(fh, pool, &poolused);
	Close(fh);
	cc_trace_apply_code_bits();
	cc_trace_build_spans();
}

void
cc_trace_omit(APTR fn)
{
	ULONG i;
	ULONG pc;

	if (fn == (APTR)NULL)
		return;
	pc = (ULONG)fn;
	i = 0UL;
	while (i < cc_nomit)
	{
		if (cc_omit[i] == pc)
			return;
		i++;
	}
	if (cc_nomit < (ULONG)CC_TRACE_MAXOMIT)
	{
		cc_omit[cc_nomit] = pc;
		cc_nomit++;
	}
}

void __saveds
cc_trace_dump(void)
{
	struct Task *t;
	ULONG *p;
	ULONG *end;
	ULONG pc;
	ULONG last;
	ULONG i;
	ULONG ncand;
	ULONG printed;
	ULONG words;
	LONG pending;
	LONG have;
	ULONG sp;
	cc_trace_fr fr;

	if (cc_dumping)
		return;
	cc_dumping = 1;
	cc_trace_msg((STRPTR)"CClib: how that panic was reached\n");
	if (cc_nseg == 0UL)
		cc_trace_snapshot();
	if (cc_nfile == 0UL && cc_nline > 0UL)
		cc_trace_build_spans();

	t = FindTask((STRPTR)NULL);
	sp = cc_trace_sp();
	if (t == (struct Task *)NULL)
	{
		cc_dumping = 0;
		return;
	}
	p = (ULONG *)((sp + 3UL) & ~3UL);
	end = (ULONG *)t->tc_SPUpper;
	if ((ULONG)p < (ULONG)t->tc_SPLower)
		p = (ULONG *)t->tc_SPLower;
	if (end < p)
	{
		cc_dumping = 0;
		return;
	}

	last = 0UL;
	ncand = 0UL;
	words = 0UL;
	while (p < end && ncand < (ULONG)CC_TRACE_MAXCAND
	    && words < (ULONG)CC_TRACE_MAXWORD)
	{
		pc = *p;
		p++;
		words++;
		if ((pc & 1UL) != 0UL)
			continue;
		if (pc == last)
			continue;
		if (!cc_trace_fill(pc, &fr))
			continue;
		last = pc;
		cc_cand[ncand] = fr;
		ncand++;
	}

	/*
	 * Stack scan sees leftover words from functions that already
	 * returned (same function repeating).  Keep the first site, then
	 * only LINE / main / startup, and drop anything that appears
	 * more than once.
	 */
	pending = -1L;
	have = 0L;
	printed = 0UL;
	i = 0UL;
	while (i < ncand && printed < (ULONG)CC_TRACE_MAXFRAME)
	{
		if (cc_cand[i].file == (STRPTR)NULL && cc_cand[i].line == 0UL
		    && cc_cand[i].sym == (STRPTR)NULL)
		{
			if (have == 0L)
				pending = (LONG)i;
			i++;
			continue;
		}
		if (i > 0UL && cc_trace_fun_hits(ncand, i) >= 2UL)
		{
			i++;
			continue;
		}
		if (have != 0L && cc_cand[i].line == 0UL
		    && !cc_trace_is_main(&cc_cand[i])
		    && !cc_trace_stop(&cc_cand[i]))
		{
			i++;
			continue;
		}
		if (have == 0L && pending >= 0L)
		{
			printed++;
			cc_trace_print_fr(&cc_cand[pending], 1L);
			have = 1L;
			if (printed >= (ULONG)CC_TRACE_MAXFRAME)
				break;
		}
		printed++;
		cc_trace_print_fr(&cc_cand[i], printed == 1UL ? 1L : 0L);
		have = 1L;
		if (cc_trace_stop(&cc_cand[i]))
			break;
		i++;
	}
	if (have == 0L && pending >= 0L)
	{
		printed++;
		cc_trace_print_fr(&cc_cand[pending], printed == 1UL ? 1L : 0L);
	}
	if (printed == 0UL)
		cc_trace_msg((STRPTR)"  (no code addresses on stack)\n");
	cc_dumping = 0;
}

void
cc_trace_clear(void)
{
	if (cc_block != (APTR)NULL)
	{
		FreeMem(cc_block, cc_blocksz);
		cc_block = (APTR)NULL;
		cc_blocksz = 0UL;
	}
	cc_lines = (cc_trace_line *)NULL;
	cc_nline = 0UL;
	cc_nfile = 0UL;
	cc_have_parse = 0UL;
	cc_code_bits = 0UL;
}

void __stdargs __saveds
cc_crt_abort(LONG code)
{
	exit(code);
}
