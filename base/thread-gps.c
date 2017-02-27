#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <base/devel.h>
#include <base/error.h>
#include <base/watchdog.h>
#include <board/led.h>
#include <board/uart.h>
#include <util/tools.h>
#include <callback.h>
#include "logd/logd_rpc.h"

#include "at/at_util.h"

#include <mdsapi/mds_udp_ipc.h>

#define DEV_GPS_TIMEOUT_SEC	60

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_GPS


static int flag_run_thread_gps = 1;

static int will_read_size;
static char buffer[GPS_MAX_BUFF_SIZE];
static char *pbufferin = NULL;
static int read_size;
static int read_error_count = 0;


#define MAX_UDP_GPSD_MAX_ERR_1_CNT	15
#define MAX_UDP_GPSD_MAX_ERR_2_CNT	3

int gps_init()
{
	int ret;
	pbufferin = buffer;
	will_read_size = GPS_MAX_BUFF_SIZE - 1;

	gps_on(GPS_TYPE_AGPS);
	led_noti(eLedcmd_GPS_SEARCH);

	printf("gps init retrun 0\r\n");
	return 0;
}

void gps_deinit()
{
	printf("gps_deinit...1\n");
	
	printf("gps_deinit...4\n");
	gps_off();
	printf("gps_deinit...5\n");
}

void _init_thread_gps(void)
{

	if(gps_init() < 0)
	{
		LOGE(LOG_TARGET, "GPS Data Port Open False..\r\n");
		error_critical(eERROR_EXIT, "_init_thread_gps GPS Error");
	}
}

int _splite_gps_context(void)
{
	char *pDelimeter, *plastbyte;
	int gps_data_size;
	int res = 0;
	plastbyte = pbufferin + read_size;
	pDelimeter = strstr(buffer, "$GPGSA");
	if(pDelimeter != NULL) {
		pDelimeter = tools_strnchr(pDelimeter, 0x0a, plastbyte - pDelimeter);
	}

	if(pDelimeter == NULL)
	{
		pbufferin += read_size;
		if(pbufferin >= buffer + GPS_MAX_BUFF_SIZE - 1)
		{
			LOGE(LOG_TARGET, "Critical! GPS Logic has been broken.\nClear buffer.\n");
			error_critical(eERROR_LOG , "GPS Logic broken");
			pbufferin = buffer;
			will_read_size = GPS_MAX_BUFF_SIZE - 1;
			memset(buffer, 0, GPS_MAX_BUFF_SIZE - 1);
		}
		else
		{
			will_read_size = (buffer + GPS_MAX_BUFF_SIZE - 1) - pbufferin;
			//LOGT(LOG_TARGET, "%d %d\n", __LINE__, will_read_size);
		}
		res = 0;
	}
	else
	{
		pDelimeter += 1; //Point at next byte of 0x0a
		gps_data_size = pDelimeter - buffer;

		gps_parse(buffer, gps_data_size);

		//Set pointer to point next gps context
		memmove(buffer, pDelimeter, plastbyte - pDelimeter);
		read_size = 0;
		memset(buffer + (plastbyte - pDelimeter), 0, (GPS_MAX_BUFF_SIZE - 1) - (plastbyte - pDelimeter));
		pbufferin = buffer + (plastbyte - pDelimeter);
		will_read_size = (buffer + GPS_MAX_BUFF_SIZE - 1) - pbufferin;
		res = 1;
	}
	return res;
}

void _check_gps_read_err(int sms_noti)
{
	if(read_size == 0)
	{
		read_error_count++;
	}

	if(read_error_count > 30 || read_size < 0)	
	{
		read_error_count = 0;
		LOGE(LOG_TARGET, "read_size is minus value\n");
		static int noti_gpsfd_triger = 0;
		if(noti_gpsfd_triger < 3 && sms_noti != 0)
		{
			char debugbuff[30] = {0,};

			sprintf(debugbuff, "GPS Read Error %d non gps fd mode", read_size);

			devel_send_sms_noti(debugbuff, strlen(debugbuff), 3);
		}
		if(noti_gpsfd_triger >= 3)
		{
			gps_deinit();
			sleep(1);

			if(gps_init() < 0)
			{
				LOGE(LOG_TARGET, "GPS Data Port Open False..\r\n");
				error_critical(eERROR_EXIT, "_check_gps_read_err : Open False");
			}
			noti_gpsfd_triger = 0;
		}
		noti_gpsfd_triger++;
	}
}

void _check_gps_timeout_err(time_t time_gps_avail, int sms_noti)
{
	static time_t pre_reset_time = 0;
	static time_t pre_time_gps_avail = 0;

	if(pre_time_gps_avail == time_gps_avail)
	{
		error_critical(eERROR_EXIT, "_check_gps_timeout_err : no signal");
		return;
	}
	
	if(time_gps_avail > pre_reset_time)
	{
		LOGE(LOG_TARGET, "GPS Reset!!!!!\n");
		gps_deinit();
		sleep(1);
		
		if(gps_init() < 0)
		{
			LOGE(LOG_TARGET, "GPS Data Port Open False..\r\n");
			error_critical(eERROR_EXIT, "_check_gps_timeout_err : open fail");
		}
		
		pre_reset_time = tools_get_kerneltime() + 600;
		pre_time_gps_avail = time_gps_avail;
		
		if(sms_noti != 0) {
			devel_send_sms_noti("GPS Reset!", sizeof("GPS Reset!"), 3);
		}
	}
}

void _check_gps_delay_err(time_t time_gps_avail, int sms_noti)
{
	static time_t is_send_sms = 0;
	time_t nowtime = tools_get_kerneltime();
	if(nowtime - time_gps_avail > 2)
	{
		if(sms_noti != 0 && nowtime > is_send_sms) {
			char debugbuff[30] = {0,};

			LOGE(LOG_TARGET, "GPS delayed receive! %d sec\n", nowtime - time_gps_avail);

			sprintf(debugbuff, "GPS Error! %d sec", (unsigned int)(nowtime - time_gps_avail));
			devel_send_sms_noti(debugbuff, strlen(debugbuff), 2);
			is_send_sms = nowtime + 300;
		}
	}
}

void _check_null_data(char *buffer, int size, int noti)
{
	int null_value = 0;
	if((null_value = tools_null2space(buffer, size)) > 0)
	{
		static int noti_null_triger = 0;
		if(noti_null_triger == 0 && noti != 0)
		{
			char debugbuff[30] = {0,};
			sprintf(debugbuff, "GPS Null Error!%d", null_value);
			devel_send_sms_noti(debugbuff, strlen(debugbuff), 3);
			noti_null_triger = 1;
		}
	}
}

void _gps_led_notification(void)
{
	//GPS LED Notification
	static int count_gps_abnormal = 0; //For LED noti
	static int have_received_active_gpsdata = 0;
	gpsData_t temp_gpsdata;
	gps_get_curr_data(&temp_gpsdata);
	if(temp_gpsdata.active == 1)
	{
		count_gps_abnormal = 0;
		have_received_active_gpsdata = 1;
		led_noti(eLedcmd_GPS_GOOD);
	}
	else
	{
		if(have_received_active_gpsdata == 1 && ++count_gps_abnormal >= 5)
		{
			led_noti(eLedcmd_GPS_BAD);
		}
	}
}

/*===========================================================================================*/
void *thread_gps(void *args)
{
	struct timeval timeout;
	time_t time_gps_avail = 0;


	int result;

	fd_set reads;

	configurationBase_t * conf = get_config_base();

	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if(!flag_run_thread_gps)
	{
		return NULL;
	}
	
	_init_thread_gps();

	sleep(1);

	time_gps_avail = tools_get_kerneltime();
	// 60sec fix
	watchdog_set_time(eWdGps, 120);

	while(flag_run_thread_gps)
	{
		static int count_wd = 0;

		static int udp_gpsd_err_cnt1 = 0;
		static int udp_gpsd_err_cnt2 = 0;

		wd_dbg[eWdGps] = 1;

		if(count_wd == 0  || count_wd >= 30)
		{
			count_wd = 0;
			watchdog_set_cur_ktime(eWdGps);
		}
		count_wd++;

		read_size = mds_api_gpsd_get_nmea(pbufferin, GPS_MAX_BUFF_SIZE);
		//printf("gps recv udp ipc ::> [%s] [%d]\r\n", pbufferin, read_size);

		wd_dbg[eWdGps] = 2;

		// check gps data 1
		if ( udp_gpsd_err_cnt1 > MAX_UDP_GPSD_MAX_ERR_1_CNT )
		{
			LOGE(LOG_TARGET, "get gps data fail 1 : cannot recv form gpsd , gpsd reset\n");
			devel_webdm_send_log("GPSD req reset 1 : cannot recv gps data");
			gps_reset(GPS_TYPE_AGPS);
			udp_gpsd_err_cnt1 = 0;
			udp_gpsd_err_cnt2++;
		}

		wd_dbg[eWdGps] = 3;

		// check gps data 2
		if ( udp_gpsd_err_cnt2 > MAX_UDP_GPSD_MAX_ERR_2_CNT )
		{
			LOGE(LOG_TARGET, "get gps data fail 2 : cannot recv form gpsd , gpsd reset\n");
			devel_webdm_send_log("GPSD req reset 2 : cannot recv gps data");
			system("killall mds_gpsd3");
			udp_gpsd_err_cnt1 = 0;
			udp_gpsd_err_cnt2 = 0;
		}

		wd_dbg[eWdGps] = 4;

		if(read_size > 0)
		{
			wd_dbg[eWdGps] = 5;
			_check_null_data(pbufferin, read_size, conf->noti.turn_on_sms_enable);
			wd_dbg[eWdGps] = 6;
			read_error_count = 0;

			udp_gpsd_err_cnt1 = 0;
			udp_gpsd_err_cnt2 = 0;
		}
		else
		{
			LOGE(LOG_TARGET, "gps thread gps read error\n");
			wd_dbg[eWdGps] = 7;
			_check_gps_read_err(conf->noti.turn_on_sms_enable);

			udp_gpsd_err_cnt1++;
			
			continue;
		}

		while(1) {
			wd_dbg[eWdGps] = 8;
			if(_splite_gps_context() == 0)
			{
				break;
			}
			wd_dbg[eWdGps] = 9;
			_check_gps_delay_err(time_gps_avail, conf->noti.turn_on_sms_enable);
			_gps_led_notification();
			time_gps_avail = tools_get_kerneltime();

			wd_dbg[eWdGps] = 10;
			if(g_skip_gps_when_error > 0)
			{
				LOGT(LOG_TARGET, "GPS Skip - %d", g_skip_gps_when_error);
				g_skip_gps_when_error--;
				
				continue;
			}
			
			wd_dbg[eWdGps] = 11;
			gps_parse_one_context_callback();
			wd_dbg[eWdGps] = 12;
		}
		wd_dbg[eWdGps] = 13;

	}
	
	gps_off();

	return NULL;
}

void exit_thread_gps(void)
{
	flag_run_thread_gps = 0;
}



void test_gps_func()
{
	fd_set reads;
	printf("test_gps_func 1++\n");

	printf("test_gps_func 2++\n");
	sleep(1);
	gps_deinit();
	printf("test_gps_func 3++\n");
	sleep(1);
	printf("test_gps_func 4++\n");

	if(gps_init() < 0)
		printf("test_func error\n");
}

