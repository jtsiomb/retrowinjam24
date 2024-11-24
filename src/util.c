#include <stdio.h>
#include "util.h"


unsigned int calc_shift(unsigned int x)
{
	unsigned int shift = 0;
	while(x > 1) {
		shift++;
		x >>= 1;
	}
	return shift;
}

const char *memsizestr(unsigned long sz)
{
	static char buf[32];
	static const char *suffix[] = {"bytes", "kb", "mb", "gb", "tb"};
	int sidx = 0;

	while(sz > 1024 * 1024 && sidx < sizeof suffix / sizeof *suffix - 1) {
		sz >>= 10;
		sidx++;
	}

	sprintf(buf, "%lu %s", sz, suffix[sidx]);
	return buf;
}
