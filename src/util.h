#ifndef UTIL_H_
#define UTIL_H_

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

unsigned int calc_shift(unsigned int x);

#endif	/* UTIL_H_ */
