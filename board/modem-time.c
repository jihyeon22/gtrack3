#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "modem-time.h"

#include <logd_rpc.h>
#define LOG_TARGET eSVC_COMMON
time_t get_modem_time_utc_sec()
{
	// w200 / w100 이외의 모델은 modem time 은 지원하지 않으므로 그냥 local time 으로 대신한다.
	struct timeval tv;
	struct tm ttm;


	gettimeofday(&tv, NULL);

	localtime_r(&tv.tv_sec, &ttm);

	if ( ttm.tm_year+1900 < 2016)
		return 0;
/*
	{ 
		char test_buff[128] = {0,};

		sprintf(test_buff, " cur time is [%d:%d - %d:%d:%d]\r\n", ttm.tm_mon + 1, ttm.tm_mday, ttm.tm_hour, ttm.tm_min, ttm.tm_sec);
		LOGT(LOG_TARGET, test_buff);
	}
*/
	return (mktime(&ttm));
}


int get_modem_time_tm(struct tm* time_tm)
{
	// w200 / w100 이외의 모델은 modem time 은 지원하지 않으므로 그냥 local time 으로 대신한다.
  	time_t t;
    struct tm *lt;

	struct tm* tm_ptr;

    if((t = time(NULL)) == -1) {
        perror("get_modem_time_tm() call error");
        return MODEM_TIME_RET_FAIL;
    }

    if((lt = localtime(&t)) == NULL) {
        perror("get_modem_time_tm() call error");
        return MODEM_TIME_RET_FAIL;
    }
/*
	{
		char   time_str[26];	
		sprintf(time_str, "%d-%d-%d %d:%d:%d", lt->tm_year+1900,
						lt->tm_mon+1,
						lt->tm_mday,
						lt->tm_hour,
						lt->tm_min,
						lt->tm_sec);

		printf("%s\r\n",time_str);
	}
*/
	if ( lt->tm_year+1900 < 2016)
		return MODEM_TIME_RET_FAIL;

	memcpy(time_tm, lt, sizeof(struct tm));
	return MODEM_TIME_RET_SUCCESS;
}
