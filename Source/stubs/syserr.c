/*
 * stubs/syserr.c - historic sys_errlist for apps that reference it
 */

char *sys_errlist[] =
{
	"No Error",
	"File Not Found",
	"Bad File Handle",
	"Not Enough Memory",
	"File Exists",
	"Invalid Function Number",
	"Too Many Files Opened",
	"Not A Console Device",
	"Invalid Access Code",
};

int sys_nerr = 9;
