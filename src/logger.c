#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include "logger.h"


int log_start(const char *fname)
{
	if(!fname) fname = "CON";

	fflush(stdout);
	fflush(stderr);
	freopen(fname, "w", stdout);
	freopen(fname, "w", stderr);

	setvbuf(stdout, 0, _IONBF, 0);
	return 0;
}

void log_stop(void)
{
	log_start(0);
}
