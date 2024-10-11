#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "logger.h"


int log_start(const char *fname)
{
#if defined(_WIN32) || defined(DOS)
	if(!fname) fname = "CON";
#else
	if(!fname) fname = "/dev/tty";
#endif

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
