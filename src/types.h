#ifndef TYPES_H_
#define TYPES_H_

#ifdef _MSC_VER
typedef signed char int8_t;
typedef short int16_t;
typedef long int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

#elif defined(__sgi)
#include <sys/types.h>

#else
#include <stdint.h>
#endif

#endif	/* TYPES_H_ */
