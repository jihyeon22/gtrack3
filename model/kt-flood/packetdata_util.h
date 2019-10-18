#ifndef __MODEL_PACKETDATA_UTIL_H__
#define __MODEL_PACKETDATA_UTIL_H__

#include <util/list.h>

// char uniqueID[5];
int hex2bin( const char *s);
int get_phonenum_binary(char *out);
int to_bigedian(int bits32);

#endif
