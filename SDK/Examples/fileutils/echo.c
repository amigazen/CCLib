/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * echo.c - write arguments to stdout (CClib CRT example)
 */

#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char **argv)
{
	int i;

	i = 1;
	while (i < argc)
	{
		fputs(argv[i], stdout);
		i++;
		if (i < argc)
			fputc((int)' ', stdout);
	}
	fputc((int)'\n', stdout);
	if (ferror(stdout))
		return 1;
	return 0;
}
