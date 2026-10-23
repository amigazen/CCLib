/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * wc.c - word count (CClib CRT + wchar + ccpthread example)
 *
 * Classic POSIX surface: -l lines, -w words, -c bytes, -m characters.
 * Character counts use mbtowc (MB_CUR_MAX==1 identity on CClib today).
 *
 * Multiple input files are counted in parallel (one pthread per file);
 * stdin / single file stay on the main thread.  Workers never print —
 * only main writes to stdout after join (shared CON: is not thread-safe).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <wchar.h>
#include <pthread.h>

typedef struct
{
	unsigned long lines;
	unsigned long words;
	unsigned long bytes;
	unsigned long chars;
	int status; /* 0 ok, 1 error */
	int saved_errno;
	const char *name;
} wc_counts;

typedef struct
{
	const char *path; /* NULL => stdin; "-" => stdin */
	wc_counts *out;
} wc_job;

static int g_opt_l;
static int g_opt_w;
static int g_opt_c;
static int g_opt_m;

static void
wc_counts_zero(wc_counts *c)
{
	c->lines = 0UL;
	c->words = 0UL;
	c->bytes = 0UL;
	c->chars = 0UL;
	c->status = 0;
	c->saved_errno = 0;
	c->name = (const char *)NULL;
}

static void
wc_counts_add(wc_counts *dst, const wc_counts *src)
{
	dst->lines += src->lines;
	dst->words += src->words;
	dst->bytes += src->bytes;
	dst->chars += src->chars;
	if (src->status != 0)
		dst->status = 1;
}

/*
 * Count one stream.  Word state is local (POSIX: a word is a non-empty
 * sequence of non-whitespace separated by whitespace).
 */
static int
wc_fp(FILE *fp, const char *name, wc_counts *out)
{
	char buf[BUFSIZ];
	size_t n;
	size_t i;
	int in_word;
	int mb;
	wchar_t wc;
	unsigned char ch;

	wc_counts_zero(out);
	out->name = name;
	in_word = 0;

	for (;;)
	{
		n = fread(buf, (size_t)1, sizeof(buf), fp);
		if (n == (size_t)0)
			break;

		out->bytes += (unsigned long)n;
		i = 0;
		while (i < n)
		{
			ch = (unsigned char)buf[i];
			if (ch == (unsigned char)'\n')
				out->lines++;

			if (isspace((int)ch))
				in_word = 0;
			else if (!in_word)
			{
				in_word = 1;
				out->words++;
			}

			/*
			 * Character count via mbtowc.  With MB_CUR_MAX==1 this is
			 * identity; still exercises the conversion LVO path.
			 */
			mb = mbtowc(&wc, (const char *)&buf[i], n - i);
			if (mb > 0)
			{
				out->chars++;
				i += (size_t)mb;
			}
			else if (mb == 0)
			{
				/* Embedded NUL counts as one character. */
				out->chars++;
				i++;
			}
			else
			{
				/* Incomplete/invalid: skip one byte, still count. */
				out->chars++;
				i++;
			}
		}
	}

	if (ferror(fp))
	{
		out->status = 1;
		out->saved_errno = errno;
		return 1;
	}
	return 0;
}

static int
wc_path(const char *path, wc_counts *out)
{
	FILE *fp;
	int rc;
	const char *name;

	if (path == (const char *)NULL || strcmp(path, "-") == 0)
	{
		name = (path != (const char *)NULL) ? path : "";
		return wc_fp(stdin, name, out);
	}

	fp = fopen(path, "r");
	if (fp == (FILE *)NULL)
	{
		wc_counts_zero(out);
		out->name = path;
		out->status = 1;
		out->saved_errno = errno;
		return 1;
	}

	name = path;
	rc = wc_fp(fp, name, out);
	if (fclose(fp) != 0)
	{
		out->status = 1;
		out->saved_errno = errno;
		rc = 1;
	}
	return rc;
}

/* Worker entry: count one file into job->out.  No printf. */
static void *
wc_worker(void *arg)
{
	wc_job *job;

	job = (wc_job *)arg;
	(void)wc_path(job->path, job->out);
	return (void *)NULL;
}

static void
wc_print_one(const wc_counts *c, int show_name)
{
	if (g_opt_l)
		printf("%7lu", c->lines);
	if (g_opt_w)
	{
		if (g_opt_l)
			printf(" ");
		printf("%7lu", c->words);
	}
	if (g_opt_c)
	{
		if (g_opt_l || g_opt_w)
			printf(" ");
		printf("%7lu", c->bytes);
	}
	if (g_opt_m)
	{
		if (g_opt_l || g_opt_w || g_opt_c)
			printf(" ");
		printf("%7lu", c->chars);
	}
	if (show_name && c->name != (const char *)NULL && c->name[0] != '\0')
		printf(" %s", c->name);
	printf("\n");
}

static void
wc_perror(const char *name, int saved_errno)
{
	if (saved_errno != 0)
		errno = saved_errno;
	if (name != (const char *)NULL && name[0] != '\0'
	    && strcmp(name, "-") != 0)
		perror(name);
	else
		perror("wc");
}

/*
 * Parallel path: one thread per file path.  Results printed in argv order
 * after all joins so console output stays single-threaded.
 */
static int
wc_files_threaded(char **paths, int npaths)
{
	wc_counts *results;
	wc_job *jobs;
	pthread_t *tids;
	wc_counts total;
	int i;
	int err;
	int status;
	int created;

	results = (wc_counts *)calloc((size_t)npaths, sizeof(wc_counts));
	jobs = (wc_job *)calloc((size_t)npaths, sizeof(wc_job));
	tids = (pthread_t *)calloc((size_t)npaths, sizeof(pthread_t));
	if (results == (wc_counts *)NULL || jobs == (wc_job *)NULL
	    || tids == (pthread_t *)NULL)
	{
		perror("wc");
		free(results);
		free(jobs);
		free(tids);
		return 1;
	}

	created = 0;
	status = 0;
	i = 0;
	while (i < npaths)
	{
		jobs[i].path = paths[i];
		jobs[i].out = &results[i];
		err = pthread_create(&tids[i], (const pthread_attr_t *)NULL,
			wc_worker, (void *)&jobs[i]);
		if (err != 0)
		{
			fprintf(stderr, "wc: pthread_create: %s\n",
				strerror(err));
			status = 1;
			/* Count remaining files on the main thread. */
			while (i < npaths)
			{
				if (wc_path(paths[i], &results[i]) != 0)
				{
					wc_perror(paths[i], results[i].saved_errno);
					status = 1;
				}
				i++;
			}
			break;
		}
		created++;
		i++;
	}

	i = 0;
	while (i < created)
	{
		(void)pthread_join(tids[i], (void **)NULL);
		i++;
	}

	wc_counts_zero(&total);
	total.name = "total";
	i = 0;
	while (i < npaths)
	{
		if (results[i].status != 0)
		{
			wc_perror(paths[i], results[i].saved_errno);
			status = 1;
		}
		else
			wc_print_one(&results[i], 1);
		wc_counts_add(&total, &results[i]);
		i++;
	}
	if (npaths > 1)
		wc_print_one(&total, 1);

	free(results);
	free(jobs);
	free(tids);
	return status;
}

static int
wc_files_serial(char **paths, int npaths)
{
	wc_counts one;
	wc_counts total;
	int i;
	int status;

	wc_counts_zero(&total);
	total.name = "total";
	status = 0;
	i = 0;
	while (i < npaths)
	{
		if (wc_path(paths[i], &one) != 0)
		{
			wc_perror(paths[i], one.saved_errno);
			status = 1;
		}
		else
			wc_print_one(&one, 1);
		wc_counts_add(&total, &one);
		i++;
	}
	if (npaths > 1)
		wc_print_one(&total, 1);
	return status;
}

int
main(int argc, char **argv)
{
	int i;
	int status;
	int nfiles;
	char **files;
	wc_counts one;

	g_opt_l = 0;
	g_opt_w = 0;
	g_opt_c = 0;
	g_opt_m = 0;

	i = 1;
	while (i < argc && argv[i][0] == '-' && argv[i][1] != '\0'
	    && strcmp(argv[i], "-") != 0)
	{
		char *p;

		p = argv[i] + 1;
		while (*p != '\0')
		{
			if (*p == 'l')
				g_opt_l = 1;
			else if (*p == 'w')
				g_opt_w = 1;
			else if (*p == 'c')
				g_opt_c = 1;
			else if (*p == 'm')
				g_opt_m = 1;
			else
			{
				fprintf(stderr, "wc: invalid option -- %c\n",
					(int)*p);
				return 1;
			}
			p++;
		}
		i++;
	}

	/* Default POSIX: lines, words, bytes. */
	if (!g_opt_l && !g_opt_w && !g_opt_c && !g_opt_m)
	{
		g_opt_l = 1;
		g_opt_w = 1;
		g_opt_c = 1;
	}

	nfiles = argc - i;
	if (nfiles <= 0)
	{
		status = 0;
		if (wc_path((const char *)NULL, &one) != 0)
		{
			wc_perror("stdin", one.saved_errno);
			status = 1;
		}
		else
			wc_print_one(&one, 0);
		return status;
	}

	files = &argv[i];
	if (nfiles == 1)
		return wc_files_serial(files, nfiles);

	/*
	 * Two or more files: parallel count via ccpthread.  Falls back to
	 * serial for any path that is stdin ("-") mixed in — workers must
	 * not share the CRT stdin FILE*.
	 */
	i = 0;
	while (i < nfiles)
	{
		if (strcmp(files[i], "-") == 0)
			return wc_files_serial(files, nfiles);
		i++;
	}

	return wc_files_threaded(files, nfiles);
}
