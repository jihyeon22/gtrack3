#ifndef __BOARD_MODEM_TIME_H__
#define __BOARD_MODEM_TIME_H__

#define DIFF_MODEM_SEC_AND_UTC_SEC		315932400

#define MODEM_TIME_MIN_FAIL_VALUE	1388534400
#define MODEM_TIME_MAX_FAIL_VALUE	1703894400

typedef enum
{
	MODEM_TIME_RET_ERR_CANNOT_OPEN = -2,
	MODEM_TIME_RET_FAIL = -1,
	MODEM_TIME_RET_SUCCESS = 0,
	
}MODEM_TIME_RET;

int get_modem_time_tm(struct tm* time);
time_t get_modem_time_utc_sec();

#endif

