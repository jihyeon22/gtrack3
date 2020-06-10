#ifndef _CONVTOOLS_H_
#define _CONVTOOLS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

unsigned char bcdtoi(unsigned char val);
unsigned char itobcd(unsigned char val);
long char_mbtol(char *srcptr, int size);
double char_mbtod(char *srcptr, int size);
float char_mbtof(char *srcptr, int size);
bool bcc_check(unsigned char bcc, void *data, size_t size);

#endif
