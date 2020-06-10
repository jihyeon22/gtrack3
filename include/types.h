#ifndef __INCLUDE_TYPES_H__
#define __INCLUDE_TYPES_H__

#include <stdint.h>

typedef int8_t		CHAR;
typedef uint8_t		BYTE;
typedef uint16_t	WORD;

typedef int16_t		INT16;
typedef uint16_t	UINT16;

typedef int32_t		INT32;
typedef uint32_t	UINT32;

typedef int64_t		INT64;
typedef uint64_t	UNIT64;

typedef enum
{
	false,
	true
} BOOL;

#define PACKED(x)	__attribute__ ((packed)) x

#endif /* __TYPES_H__ */
