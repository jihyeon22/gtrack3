#ifndef __INCLUDE_DEFINES_H__
#define __INCLUDE_DEFINES_H__

#include <time.h>

#define DEFINES_IP_LEN	15
#define DEFINES_IP_BUFF_LEN	(DEFINES_IP_LEN + 1)

typedef enum mdsReturn mdsReturn_t;
enum mdsReturn
{
	DEFINES_MDS_NOK = -1,
	DEFINES_MDS_OK = 0,
};

char *strptime(const char *s, const char *format, struct tm *tm);
double round(double x);
size_t strnlen(const char *s, size_t maxlen);

#endif

