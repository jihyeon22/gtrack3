
#include <stdint.h>

/**************************************************************
 * CRC-8
 **************************************************************/
uint8_t crc8(uint8_t crc, const uint8_t *buf, int len)
{
	while(len--) {
		crc ^= *buf++;
	}

	return crc & 0xff;
}

