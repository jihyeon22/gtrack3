#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <board/board_system.h>
#include <at/at_util.h>
#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <base/dmmgr.h>
#include <board/power.h>
#include <board/battery.h>
#include <util/tools.h>
#include <logd_rpc.h>
#include "devel.h"

#include <config.h>

#include <dm/dm.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_COMMON

int devel_webdm_send_status_current(const int no_event)
{
	configurationBase_t* conf = get_config_base();
	if(conf->webdm.enable ==0 || conf->webdm.tx_report==0)
	{
		return 0;
	}

	char buffer[10] = {0};
	snprintf(buffer, sizeof(buffer), "%d", no_event);

	LOGT(LOG_TARGET, "send_status_webdm_server %d\n", no_event);

	dm_evo_insert_usrdata(EVO_USR4, buffer);

	return dmmgr_send(eEVENT_REPORT, NULL, 0);
}

int devel_webdm_send_log(const char *format, ...)
{
	va_list va;
	char phonenum[AT_LEN_PHONENUM_BUFF] = {0,};
	char imei[AT_LEN_IMEI_BUFF] = {0,};
	char log_buff[4096] = {0,};

	configurationBase_t* conf = get_config_base();
	if(conf->webdm.enable ==0 || conf->webdm.tx_log ==0)
	{
		return 0;
	}
	
	at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);
	at_get_imei(imei, AT_LEN_IMEI_BUFF);

	va_start(va, format);
	vsnprintf(log_buff, sizeof(log_buff) - 1, format, va);
	va_end(va);
	
	LOGT(LOG_TARGET, "send_log_webdm_server %s\n", log_buff);
	
	return dmmgr_send(eEVENT_LOG, log_buff, 0);
}

int devel_send_sms_noti(const char* buff, const int buff_len, const int timeout_sec)
{
	configurationBase_t* conf = get_config_base();
	int res = 0;
	int n_try = 3;

	if(conf->noti.turn_on_sms_enable == 1)
	{
		int phonelen = 0;
		phonelen = strlen(conf->noti.target_num);
		if(phonelen >= 6 && phonelen <= 11)
		{
			do
			{
				res = at_send_sms(conf->noti.target_num, buff);
			}
			while(res < 0 && n_try-- > 0);
		}
	}
	return res;
}

void devel_log_poweroff(const char *log, const int log_len)
{
	char buff[192] = {0,};
	char buff_date[64] = {0};
	configurationBase_t* conf = get_config_base();
	time_t t;

	printf("log_poweroff enable %d\n", conf->common.log_poweroff);
	
	if(conf->common.log_poweroff == 0)
	{
		return;
	}

	t = time(NULL);
	asctime_r(localtime(&t), buff_date);

	snprintf(buff, sizeof(buff)-1, "%.63s%.120s\n", buff_date, log);

	tools_write_data(LOG_PWR_PATH, (unsigned char *)buff, strlen(buff), true);
}

