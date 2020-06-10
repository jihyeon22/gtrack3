
#include <stdint.h>
#include <stdio.h>

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
	*dst = 0x7e;
	dstlen++;

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

void hdlc_async_print_data(const uint8_t *base, int len)
{
	int size = len;

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
	}
	printf("(%d bytes)\n", size);
}

