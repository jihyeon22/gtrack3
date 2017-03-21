#include <string.h>
#include "wrapper/dtg_convtools.h"

unsigned char bcdtoi(unsigned char val)
{
  return ( (val/16*10) + (val%16) );
}

unsigned char itobcd(unsigned char val)
{
  return ( (val/10*16) + (val%10) );
}

long char_mbtol(char *srcptr, int size)
{
	char tmp_buf[128];
	int result;
	
	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	memcpy(tmp_buf, srcptr, size);
	result = strtol(tmp_buf, NULL, 10);
	return result;
}

double char_mbtod(char *srcptr, int size)
{
	char tmp_buf[128];
	int result;

	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	memcpy(tmp_buf, srcptr, size);
	result = strtod(tmp_buf, NULL);
	return result;
}

bool bcc_check(unsigned char bcc, void *data, size_t size)
{
	int i;
	unsigned char _bcc = 0x0;
	unsigned char *buf = (char*)data;
	
	_bcc = buf[0];
	for(i = 1; i < size; i++) {
		_bcc = _bcc ^ buf[i];
	}

	if (_bcc != bcc)
		return false;

	return true;
}
