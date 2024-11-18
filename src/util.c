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
