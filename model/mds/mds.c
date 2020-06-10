#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include "packet.h"
#include <base/gpstool.h>
#include <board/power.h>
#include <util/list.h>


#include "mds.h"
#include "mds_ctx.h"
#include "board/power.h"
#include "data-list.h"
#include "config.h"
#include "netcom.h"

#include "logd/logd_rpc.h"
#include "base/devel.h"
#include "util/tools.h"

#include "at/at_util.h"
#include "at/at_log.h"

#include "board/modem-time.h"
#include "board/board_system.h"
// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

int mds_init()
{
	struct tm time = {0,};
	
	// 1. wait for network active...
	while(get_ctx_network() != 1)
	{
		LOGD(LOG_TARGET, "mds_init: wait for network active\r\n");
		sleep(1);		
	}
	
	// 2. network parameter init
	init_network_param();
	
	// 3. set time..
	
		
	// 4. init status is poweron
	set_ctx_power(PACKET_POWER_STATUS_ON);
	
	// 5. check power status
	if ( POWER_IGNITION_ON == power_get_ignition_status() )
	{
		// keyon...
		LOGD(LOG_TARGET, "mds_init: First RUN : Ignition ON\r\n");
		
		if ( get_modem_time_tm(&time) != MODEM_TIME_RET_SUCCESS ) 
		{
			time_t tm_time;
			struct tm* ptime;
			
			LOGI(LOG_TARGET, "get modem time : use atd \r\n");
			
			at_get_modemtime(&tm_time,0);
			ptime = localtime(&tm_time);
			
			memcpy(&time, ptime, sizeof(time));
		}
		
		set_ctx_power(PACKET_POWER_STATUS_ON);
		set_pkt_ctx_keyon_time(time.tm_year + 1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	}
	else
	{
		// key off...
		LOGD(LOG_TARGET, "mds_init: First RUN : Ignition OFF\r\n");
		
		if ( get_modem_time_tm(&time) != MODEM_TIME_RET_SUCCESS ) 
		{
			time_t tm_time;
			struct tm* ptime;
			
			LOGI(LOG_TARGET, "get modem time : use atd \r\n");
			
			at_get_modemtime(&tm_time,0);
			ptime = localtime(&tm_time);
			
			memcpy(&time, ptime, sizeof(time));
		}
	
		set_ctx_power(PACKET_POWER_STATUS_OFF);
		set_pkt_ctx_keyoff_time(time.tm_year + 1900, time.tm_mon+1, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
	}

	// 6. get dial...
	{
		char temp_buff[128] = {0};
		at_get_phonenum(temp_buff, PACKET_DEFINE_TEL_NO_LEN);
		set_pkt_ctx_deivce_phone_num(temp_buff,PACKET_DEFINE_TEL_NO_LEN);
		LOGI(LOG_TARGET, "mds_init: dial is [%s]\r\n",temp_buff);
	}

	return 0;	
}

#if 0
int model_mds_get_gpsdata(gpsData_t* pgpsdata)
{
	gpsData_t* pgpsdata_tmp;
	int res = 0;
	int find_gps = 0;
	
	struct timeval tv;
	struct tm *ptm;
		
	memset(pgpsdata, 0x00, sizeof(gpsData_t));


	// flush gps data.
	pthread_mutex_lock(&packet_list.mutex);
	
	while (1)
	{
	
		res = list_pop((void *)&pgpsdata_tmp, &packet_list.head_list, &packet_list.tail_list);
		
	
		if ((res == 0) && (pgpsdata_tmp))
		{
			memcpy(pgpsdata, pgpsdata_tmp, sizeof(gpsData_t));
			free(pgpsdata_tmp);
			pgpsdata_tmp = NULL;
			find_gps = 1;
		}
		else
		{
			break;
		}
	}
	pthread_mutex_unlock(&packet_list.mutex);
	
	
	 
	if ( pgpsdata->utc_sec == 0 )
	{	
		res =  at_sync_systemtime_from_modemtime();
		
		printf(" model_mds_get_gpsdata : GPS Data Not Found \r\n");
		
		gettimeofday(&tv, NULL);
		ptm = localtime(&tv.tv_sec);
		
		pgpsdata->year = ptm->tm_year + 1900;
		pgpsdata->mon = ptm->tm_mon+1;
		pgpsdata->day = ptm->tm_mday;
		pgpsdata->hour = ptm->tm_hour;
		pgpsdata->min = ptm->tm_min;
		pgpsdata->sec = ptm->tm_sec;
		
		//return MODEL_MDS_FAIL;
	}
	
	return MODEL_MDS_SUCCESS;
					
}
#endif

void model_mds_make_poweron_packet(gpsData_t gpsdata)
{
	// maybe.. this function is called 1sec interval
	
	static int time_sec = 1;
	
	LOGD(LOG_TARGET, "PowerOn Routine - make pkt (%d/%d) / %d \r\n", get_ctx_keyon_gather_data_interval(), get_ctx_keyon_send_to_data_interval(), time_sec);
	
	if ( !(time_sec % get_ctx_keyon_gather_data_interval()) )
	{
		LOGD(LOG_TARGET, " -- : PowerOn Routine ==> Insert Poweron Packet / %d \r\n", time_sec);
		make_packet_keyon_data_insert(&gpsdata);
	}
	
	if (!(time_sec % get_ctx_keyon_send_to_data_interval()))
	{
		LOGD(LOG_TARGET, " -- : PowerOn Routine ==> Make Poweron Packet / %d \r\n", time_sec);
		make_packet_keyon_data_done();
	}
	
	time_sec++;
}


void model_mds_send_poweron_packet()
{
	// maybe.. this function is called 1sec interval
	
	static int time_sec = 1;
	int server_stat = model_mds_get_server_status() ;
	
	LOGD(LOG_TARGET, "PowerOn Routine - send pkt (%d) / %d / %d \r\n", get_ctx_keyon_send_to_data_interval(), server_stat, time_sec);
	
	if (time_sec > get_ctx_keyon_send_to_data_interval())
	{
		
		switch(server_stat)
		{
			case MODEL_MDS_SERVER_OK:
			{
				LOGI(LOG_TARGET, "PowerOn Routine send - server is ok : flush pkt / %d \r\n", time_sec);
				send_keyon_data(0);
				time_sec = 0;
				break;
			}
			case MODEL_MDS_SERVER_ERR_AND_CHK:
			{ 
				LOGI(LOG_TARGET, "PowerOn Routine send - server is err : one pkt / %d \r\n", time_sec);
				send_keyon_data(1);
				time_sec = 0;
				break;
			}
			default:
			{
				;
			}
		}
	}
	else
	{
		time_sec++;
	}
}


void model_mds_send_poweroff_event_packet(gpsData_t gpsdata)
{
	LOGI(LOG_TARGET, " PowerOff Routine - Poweroff event pkt \r\n");
	
	make_packet_keyoff_data_insert(&gpsdata);
	make_packet_keyoff_data_done();
	send_keyoff_data(0);
}


void model_mds_make_poweroff_packet(gpsData_t gpsdata)
{
	// 1초마다 불린다는 가정하에?
	static int time_sec = 1;
	//int server_stat = model_mds_get_server_status() ;
	
	LOGD(LOG_TARGET, " PowerOff Routine make pkt (%d) /%d \r\n", get_ctx_keyoff_gather_data_interval(), time_sec);
	
	if ( !(time_sec % get_ctx_keyoff_gather_data_interval()) )
	{
		LOGD(LOG_TARGET, " -- : PowerOff Routine ==> Insert Poweroff Packet / %d \r\n", time_sec);
		make_packet_keyoff_data_insert(&gpsdata);
	}
	
	if ( !(time_sec % get_ctx_keyoff_send_to_data_interval()) ) 
	{
		LOGD(LOG_TARGET, " -- : PowerOff Routine ==> Make Poweroff Packet / %d \r\n", time_sec);
		make_packet_keyoff_data_done();
	}
	
	time_sec++;
}

void model_mds_send_poweroff_packet()
{
	// 1초마다 불린다는 가정하에?
	static int time_sec = 1;
	int server_stat = model_mds_get_server_status() ;
	
	LOGD(LOG_TARGET, "PowerOff Routine - send pkt (%d) / %d / %d \r\n", model_mds_get_server_status(), server_stat, time_sec);
	
	if (time_sec > get_ctx_keyoff_send_to_data_interval() )
	{	
		switch(server_stat)
		{
			case MODEL_MDS_SERVER_OK:
			{
				LOGD(LOG_TARGET, "PowerOff Routine send - server is ok : flush pkt / %d \r\n", time_sec);
				send_keyoff_data(0);
				time_sec = 0;
				break;
			}
			case MODEL_MDS_SERVER_ERR_AND_CHK:
			{ 
				LOGD(LOG_TARGET, "PowerOff Routine send - server is err : one pkt / %d \r\n", time_sec);
				send_keyoff_data(1);
				time_sec = 0;
				break;
			}
			default:
			{
				;
			}
		}
	}
	else
	{
		time_sec++;
	}
	
}



int model_mds_get_server_status()
{
	int ret = MODEL_MDS_SERVER_NOK;
	int sleep_cnt = get_ctx_server_sleep();
	int server_stat = get_ctx_server_stat();
	
	// if sleep cnt is zero then send data timing..
	LOGD(LOG_TARGET, ">> server sleep count [%d] \r\n", sleep_cnt);
	
	if (sleep_cnt == 0)
	{
		if (server_stat == MDT_SERVER_STAT__SUCCESS)
		{
			LOGD(LOG_TARGET, ">> server status is good \r\n");
			ret = MODEL_MDS_SERVER_OK;
		}
		else
		{
			LOGE(LOG_TARGET, ">> server status is not good \r\n");
			ret = MODEL_MDS_SERVER_ERR_AND_CHK;
		}
	}
		
	return ret;
}

void model_mds_check_server()
{
	decrease_ctx_server_sleep();
}


void model_mds_server_req(const unsigned char response_code)
{
	static unsigned int server_err_cnt_protocol = 0;
	static unsigned int server_err_cnt_no_regi = 0;
	static unsigned int server_err_cnt_server_chk = 0;
	static unsigned int server_err_cnt_server_over = 0;
	
	static unsigned int last_err_time = 0;
	
	int cur_time = tools_get_kerneltime();
	
	char smsmsg[100] = {0,};
	sprintf(smsmsg,"server err %d %u\n", response_code, (unsigned int)tools_get_kerneltime());
		
	set_ctx_server_stat(response_code);
	
	switch (response_code)
	{
		case MDT_SERVER_STAT__SUCCESS:
		{
			LOGI(LOG_TARGET, ">>>>> Server request is SUCCESS \r\n");
			set_ctx_server_sleep(0);
			
			// clear all err counter
			server_err_cnt_protocol = 0;
			server_err_cnt_no_regi = 0;
			server_err_cnt_server_chk = 0;
			server_err_cnt_server_over = 0;
			
			last_err_time = 0;
			
			break;
		}
		case MDT_SERVER_STAT__ERR_PROTOCOL:
		{
			LOGE(LOG_TARGET, ">>>>> Server request is ERR_PROTOCOL \r\n");
			
			server_err_cnt_protocol ++;
			devel_webdm_send_log("Server Resp - Error : Protocol Error");
			devel_send_sms_noti(smsmsg, sizeof(smsmsg), 3);
			break;
		}
		case MDT_SERVER_STAT__ERR_NO_REGI:
		{
			LOGE(LOG_TARGET, ">>>>> Server request is ERR_NO_REGI \r\n");
			
			server_err_cnt_no_regi ++;
			devel_webdm_send_log("Server Resp - Error : No Reg");
			devel_send_sms_noti(smsmsg,sizeof(smsmsg), 3);
			break;
		}
		case MDT_SERVER_STAT__ERR_SERVER_CHECK:
		{
			LOGE(LOG_TARGET, ">>>>> Server request is ERR_SERVER_CHECK \r\n");
			
			server_err_cnt_server_chk ++;
			devel_webdm_send_log("Server Resp - Error : Server checking..");
			devel_send_sms_noti(smsmsg, sizeof(smsmsg), 3);
			break;
		}
		case MDT_SERVER_STAT__ERR_SERVER_OVER:
		{
			LOGE(LOG_TARGET, ">>>>> Server request is ERR_SERVER_OVER \r\n");
			
			server_err_cnt_server_over ++;
			devel_webdm_send_log("Server Resp - Error : Server overload..");
			devel_send_sms_noti(smsmsg, sizeof(smsmsg), 3);
			break;
		}
		default:
		{
			//devel_send_sms_noti(smsmsg, sizeof(smsmsg), 3);
		}
	}
	
	
	// server error routine..
	if (server_err_cnt_server_chk > 0)
	{
		LOGE(LOG_TARGET, ">>>>> Server chk err count is %d \r\n", server_err_cnt_server_chk);
		set_ctx_server_sleep(SERVER_ERR_RETRY_SLEEP_TIME);
		
	}
	
	if ((server_err_cnt_protocol > 0) || 
		(server_err_cnt_server_over > 0) || 
		(server_err_cnt_no_regi > 0) )
	{
		LOGE(LOG_TARGET, ">>>>> Server protocol err count is %d \r\n", server_err_cnt_protocol);
		LOGE(LOG_TARGET, ">>>>> Server server over err count is %d \r\n", server_err_cnt_server_over);
		LOGE(LOG_TARGET, ">>>>> Server noregi err count is %d \r\n", server_err_cnt_no_regi);
		
		set_ctx_server_sleep(SERVER_ERR_RETRY_SLEEP_TIME);
		
		if ( last_err_time == 0 )
		{
			last_err_time = tools_get_kerneltime();
		}
		
		// if 48hour over then.. don`t send packet forever ...
		if ((last_err_time - cur_time) > SERVER_ERR_EXPIRE_TIME) 
		{
			LOGE(LOG_TARGET, ">> Sever ERROR!!!! forever bye bye... \r\n");
			set_ctx_server_sleep(-1);
		}
	}
		
	// check 48 hour
}

void mds_load_and_set_config(void)
{
	configurationModel_t *conf = NULL;
	
	LOGI(LOG_TARGET, "Load config and set \r\n");
	
	// reload config
	if(!(conf = load_config_user())) 
	{
		load_config_user();
		return;
	}
	
	// re setting
	setting_network_param();

	// re setting
	set_ctx_keyon_gather_data_interval(conf->model.collect_interval_keyon);
	set_ctx_keyon_send_to_data_interval(conf->model.report_interval_keyon);
	set_ctx_keyoff_gather_data_interval(conf->model.collect_interval_keyoff);
	set_ctx_keyoff_send_to_data_interval(conf->model.report_interval_keyoff);

	set_ctx_server_sleep(0);
}
