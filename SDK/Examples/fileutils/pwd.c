/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * pwd.c - print working directory (CClib CRT example)
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	char buf[PATH_MAX];
	char *path;

	(void)argc;
	(void)argv;

	path = getcwd(buf, (long)sizeof(buf));
	if (path == (char *)NULL)
	{
		perror("getcwd");
		return 1;
	}
	printf("%s\n", buf);
	return 0;
}
