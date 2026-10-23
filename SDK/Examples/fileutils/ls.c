/* SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026 amigazen project
 */

/*
 * ls.c - list directory entries (CClib CRT example)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

static int
list_one(char *path, int show_header)
{
	DIR *dir;
	struct dirent *ent;
	int status;

	status = 0;
	dir = opendir(path);
	if (dir == (DIR *)NULL)
	{
		perror(path);
		return 1;
	}

	if (show_header)
		printf("%s:\n", path);

	for (;;)
	{
		ent = readdir(dir);
		if (ent == (struct dirent *)NULL)
			break;
		printf("%s\n", ent->d_name);
	}

	if (closedir(dir) != 0)
	{
		perror(path);
		status = 1;
	}
	return status;
}

int
main(int argc, char **argv)
{
	char cwd[PATH_MAX];
	char *path;
	int i;
	int status;
	int many;

	status = 0;
	if (argc < 2)
	{
		path = getcwd(cwd, (long)sizeof(cwd));
		if (path == (char *)NULL)
		{
			perror("getcwd");
			return 1;
		}
		return list_one(cwd, 0);
	}

	many = (argc > 2) ? 1 : 0;
	i = 1;
	while (i < argc)
	{
		if (list_one(argv[i], many) != 0)
			status = 1;
		if (many && i + 1 < argc)
			printf("\n");
		i++;
	}
	return status;
}
