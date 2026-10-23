/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * cat.c - copy files or stdin to stdout (CClib CRT example)
 *
 * Ordinary hosted C: <stdio.h> pulls in proto/cclib.h automatically.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static int
cat_fp(FILE *fp, char *name)
{
	char buf[BUFSIZ];
	size_t n;
	size_t w;

	for (;;)
	{
		n = fread(buf, (size_t)1, sizeof(buf), fp);
		if (n == (size_t)0)
			break;
		w = fwrite(buf, (size_t)1, n, stdout);
		if (w != n)
		{
			perror("cat: stdout");
			return 1;
		}
	}
	if (ferror(fp))
	{
		perror(name);
		return 1;
	}
	return 0;
}

int
main(int argc, char **argv)
{
	int i;
	int status;
	FILE *fp;

	status = 0;
	if (argc < 2)
	{
		if (cat_fp(stdin, "stdin") != 0)
			status = 1;
		return status;
	}

	i = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], "-") == 0)
		{
			if (cat_fp(stdin, "stdin") != 0)
				status = 1;
		}
		else
		{
			fp = fopen(argv[i], "r");
			if (fp == (FILE *)NULL)
			{
				perror(argv[i]);
				status = 1;
			}
			else
			{
				if (cat_fp(fp, argv[i]) != 0)
					status = 1;
				if (fclose(fp) != 0)
				{
					perror(argv[i]);
					status = 1;
				}
			}
		}
		i++;
	}
	return status;
}
