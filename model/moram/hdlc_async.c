
#include <stdint.h>
#include <stdio.h>
#include "debug.h"

unsigned char crc8(unsigned char crc, const unsigned char *buf, int len)
{
	while(len--) {
		crc ^= *buf++;
	}

	return crc & 0xff;
}


/**************************************************************
 * Async HDLC encoding & decoding
 **************************************************************/
int hdlc_async_encode(uint8_t *dst, const uint8_t *src, int len)
{
	int dstlen = 0;

	while(len--) {
		if(*src == 0x7e || *src == 0x7d) {
			*dst = 0x7d;
			dst++;
			dstlen++;
			*dst = *src ^ 0x20;
		} else {
			*dst = *src;
		}
		dst++;
		dstlen++;
		src++;
	}
	return dstlen;
}

int hdlc_async_decode(uint8_t *dst, const uint8_t *src, int len)
{
	int dstlen = 0;

	while(len--) {
		if(*src == 0x7e) {
			break;
		} else if(*src == 0x7d) {
			src++;
			*dst = *src ^ 0x20;
		} else {
			*dst = *src;
		}
		dst++;
		dstlen++;
		src++;
	}

	return dstlen;
}

void hdlc_async_print_data(unsigned char *base, int len)
{
	int size = len;
	int line = 20;

	printf("\t");
	while(len--) {
		if(*base == 0x7e) {
			printf("%02x ", *base);
			break;
		} else if(*base == 0x7d) {
			printf("%02x^", *base);
		} else {
			printf("%02x ", *base);
		}
		base++;
		if(line-- <= 0) {
			printf("\n");
			printf("\t");
			line = 20;
		}
	}
	printf("(%d bytes)\n", size);
}

