#include <stdarg.h>
#include <pthread.h>

#include <include/defines.h>
#include <base/config.h>
#include <base/watchdog.h>
#include <at/at_util.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#endif
#include <util/poweroff.h>
#include <util/nettool.h>
#include <util/tools.h>

#include <dm/dm.h>
#include <dm/config.h>
#include <dm/update.h>
#include <board/gpio.h>
#include <board/battery.h>
#include <logd_rpc.h>

#include <callback.h>

#include "dmmgr.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_REGI

#define DM_MUTEX_CODE

int pthread_mutex_timedlock( pthread_mutex_t * mutex, const struct timespec * abs_timeout );
void _deinit_essential_functions(void);

static int incomplete_event_buffer[eEVENT_MAX] = {0};
static int flag_send_key_on = 0;
static int flag_run_update = 0;

static time_t prev_time_dm_send = 0;

#ifdef DM_MUTEX_CODE
static pthread_mutex_t g_mutex1_timeout = PTHREAD_MUTEX_INITIALIZER;
#endif

#ifdef USE_GPS_MODEL
void gps_get(dm_gpsData_t *out)
{
	gps_get_curr_data((gpsData_t *)out);
}
#endif

void set_dm_info(dm_info *dmi)
{
	static char imei[AT_LEN_IMEI_BUFF] = {0};
	static char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
	
	at_get_imei(imei, sizeof(imei));
	at_get_phonenum(phonenum, sizeof(phonenum));
	dmi->version.hw 	= dm_get_hw_version();
	dmi->version.amss 	= dm_get_amss_version();
	dmi->version.linu 	= dm_get_linux_version();
	
	dmi->modem.telecom	= dm_get_telecom_provider();
	dmi->modem.imei		= imei;
	dmi->modem.phone	= phonenum;
	
	dmi->package.name 	= dm_load_package_file(dmi->user_package);
	dmi->package.major	= -1;	// NOT USED
	dmi->package.minor	= -1;	// NOT USED
	
	dmi->gpio.ign		= GPIO_SRC_NUM_IGNITION;	// fix
	dmi->gpio.epwr		= GPIO_SRC_NUM_POWER;		// fix
//	dmi->gpio.gp0		= 11;	// user set
//	dmi->gpio.gp1		= 13;	// user set
//	dmi->gpio.gp2		= 14;	// user set
//	dmi->gpio.gp3		= 15;	// user set
//	dmi->gpio.gp4		= 16;	// user set
//	dmi->gpio.gp5		= 18;	// user set
//	dmi->gpio.gp6		= -1;
//	dmi->gpio.gp7		= -1;
//	dmi->gpio.gp8		= -1;
//	dmi->gpio.gp9		= -1;
	
	//dmi->func.send_sms	= at_send_sms;
	dmi->func.send_sms	= NULL;
	dmi->func.rssi_get	= at_get_rssi;
	dmi->func.gpio_get	= gpio_get_value;
#ifdef USE_GPS_MODEL
	dmi->func.gps_get	= gps_get;
#endif
	dmi->func.epwr_get	= battery_get_battlevel_car;
	dmi->func.batt_get	= battery_get_battlevel_internal;
}

void dmmgr_init(void)
{
	configurationBase_t *conf = NULL;
	dm_res res;
	
	conf = get_config_base();

	if(conf->webdm.enable == 0)
	{
		return;
	}

	res = dm_global_init();
	if (res != DM_OK) {
		printf("dm_global_init failture\n");
		return;
	}

	dm_info *dmi = dm_get_info();
	if (dmi == NULL) {
		printf("dm infomation NULL\n");
		return;
	} else {
		dmi->user_ini = DM_FILE_PATH;
		dmi->user_package = PACKAGE_FILE;
		res = dm_load_ini(dmi, dmi->user_ini);
		if (res != DM_OK) {
			printf("dm load ini error\n");
		}

		set_dm_info(dmi);

		/* Gpio Init */
		gpio_set_direction(dmi->gpio.gp0, eGpioInput);
		gpio_set_direction(dmi->gpio.gp1, eGpioInput);
		gpio_set_direction(dmi->gpio.gp2, eGpioInput);
		gpio_set_direction(dmi->gpio.gp3, eGpioInput);
		gpio_set_direction(dmi->gpio.gp4, eGpioInput);
		gpio_set_direction(dmi->gpio.gp5, eGpioInput);

	}
	
	printf("dmmgr init success\n");
}

int dmmgr_send(dmEventType_t type, const char *log, int evo_errno)
{
	configurationBase_t *conf = NULL;
	int res = -1;
	time_t cur_ktime = 0;
#ifdef DM_MUTEX_CODE
	struct timespec abs_time;
	int ret;
#endif
	int skip = 0;
	
	conf = get_config_base();

	if(conf->webdm.enable == 0)
	{
		return DM_EV_RESP_OK;
	}

	if(type >= eEVENT_MAX)
	{
		return -1;
	}

	if(type == eEVENT_KEY_ON && conf->webdm.tx_ignition)
	{
		flag_send_key_on = 1;
	}

	if(nettool_get_state() == DEFINES_MDS_NOK)
	{
		switch(type)
		{
			case eEVENT_PWR_ON:
			case eEVENT_PWR_OFF:
			case eEVENT_KEY_ON:
			case eEVENT_KEY_OFF:
			case eEVENT_STATUS:
			case eEVENT_UPDATE:
				incomplete_event_buffer[type] = 1;
				break;
			default:
				break;
		}
		
		return DM_EV_RESP_OK;
	}

#ifdef DM_MUTEX_CODE
	clock_gettime(CLOCK_REALTIME , &abs_time);
    abs_time.tv_sec += 60;

	ret = pthread_mutex_timedlock(&g_mutex1_timeout,&abs_time);
	if(ret != 0){
		LOGD(LOG_TARGET, "%s Wait timed out [%d]\n", __FUNCTION__, ret);
		return -1;
	}
#endif

	switch(type)
	{
		case eEVENT_PWR_ON:
			if(conf->webdm.tx_power == 0)
			{
				skip = 1;
				res = DM_EV_RESP_OK;
				break;
			}
			res = dm_event(DM_EVENT_PWR_ON,
					EVO_IMEI | 
					EVO_HW_VER | EVO_AMSS_VER | EVO_LINUX_VER |
					EVO_TELECOM | EVO_PHONE | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			break;
		case eEVENT_PWR_OFF:
			if(conf->webdm.tx_power == 0)
			{
				skip = 1;
				res = DM_EV_RESP_OK;
				break;
			}
			res = dm_event(DM_EVENT_PWR_OFF, 
					EVO_IMEI | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			break;
		case eEVENT_KEY_ON:
			if(conf->webdm.tx_ignition == 0)
			{
				skip = 1;
				res = DM_EV_RESP_OK;
				break;
			}
			res = dm_event(DM_EVENT_KEY_ON,
					EVO_IMEI | EVO_UP_VER | EVO_LOG | EVO_ERRNO |
					EVO_PHONE | EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			break;
		case eEVENT_KEY_OFF:
			if(conf->webdm.tx_ignition == 0)
			{
				skip = 1;
				res = DM_EV_RESP_OK;
				break;
			}
			res = dm_event(DM_EVENT_KEY_OFF,
					EVO_IMEI | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			break;
		case eEVENT_REPORT:
		{
			if(conf->webdm.tx_report== 0)
			{
				skip = 1;
				res = DM_EV_RESP_OK;
				break;
			}
			res = dm_event(DM_EVENT_REPORT,
					EVO_IMEI | EVO_KEY_STAT | 
					EVO_USR0 | EVO_USR1 | EVO_USR2 | EVO_USR3 | EVO_USR4 | EVO_USR5 |
					EVO_BATT_VOLT |
					EVO_GPS_LAT | EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP | 
					EVO_LOG | EVO_ERRNO,
					log, evo_errno);
			break;
		}
		case eEVENT_STATUS:
		{
			res = dm_event(DM_EVENT_STATUS,
					EVO_HW_VER | EVO_AMSS_VER | EVO_LINUX_VER | EVO_UP_VER | 
					EVO_TELECOM | EVO_IMEI | EVO_PHONE | EVO_RSSI |
					EVO_GP0 | EVO_GP1 | EVO_GP2 | EVO_GP3 | EVO_GP4 | 
					EVO_GP5 | EVO_GP6 | EVO_GP7 | EVO_GP8 | EVO_GP9 |
					EVO_USR0 | EVO_USR1 | EVO_USR2 | EVO_USR3 | EVO_USR4 | EVO_USR5 |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | 
					EVO_GPS_LAT | EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP |
					EVO_LOG | EVO_ERRNO,
					log, evo_errno);
			break;
		}
		case eEVENT_LOG:
		{
			if(conf->webdm.tx_log== 0)
			{
				skip = 1;
				res = DM_EV_RESP_OK;
				break;
			}
			res = dm_event(DM_EVENT_LOG,
					EVO_IMEI | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			
			break;
		}
		case eEVENT_WARNING:
		{
			res = dm_event(DM_EVENT_WARNING,
					EVO_IMEI | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			break;
		}
		case eEVENT_BREAKDOWN:
		{
			res = dm_event(DM_EVENT_BREAKDOWN,
					EVO_IMEI | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);

			break;
		}
		case eEVENT_UPDATE:
			res = dm_event(DM_EVENT_UPDATE,
					EVO_IMEI | EVO_UP_VER | EVO_LOG | EVO_ERRNO |
					EVO_MAIN_VOLT | EVO_BATT_VOLT |
					EVO_KEY_STAT | EVO_PWR_STAT | EVO_GPS_LAT |
					EVO_GPS_LON | EVO_GPS_SPEED | EVO_GPS_HDOP,
					log, evo_errno);
			break;
		default:
			;
	}

	if (res == DM_EV_RESP_UPDATE) {
		if(flag_run_update == 1)
		{
			goto FINISH;
		}
		flag_run_update = 1;
				
		_deinit_essential_functions();
		terminate_app_callback();
		dm_update();
		poweroff(__FUNCTION__, sizeof(__FUNCTION__));
	}
	else if(res != DM_EV_RESP_OK)
	{
		LOGE(LOG_TARGET, "DM Server Error Response [%d]\n", res);
	}

FINISH:
	if(skip == 0)
	{
		if((cur_ktime = tools_get_kerneltime()) > 0)
		{
			prev_time_dm_send = cur_ktime;
		}
	}

#ifdef DM_MUTEX_CODE
	pthread_mutex_unlock(&g_mutex1_timeout);
#endif
	return res;
}

void dmmgr_deinit(void)
{
	dm_global_shutdown();
}

void dmmgr_send_incomplete_event(void)
{
	int i = 0;

	for(i = 0;i < eEVENT_MAX;i++)
	{
		if(incomplete_event_buffer[i] == 1)
		{
			incomplete_event_buffer[i] = 0;
			dmmgr_send(i, "late event", 0);
		}
	}
}

int dmmgr_sended_key_on(void)
{
	return flag_send_key_on;
}

void dmmgr_alive_send(void)
{
	static time_t time_log = 0;
	time_t cur_ktime = 0;

	if((cur_ktime = tools_get_kerneltime()) <= 0)
	{
		LOGE(LOG_TARGET, "%s> Fail tools_get_kerneltime %u\n", __FUNCTION__, cur_ktime);
		return;
	}

	if(cur_ktime - time_log > 10)
	{
		time_log = cur_ktime;
		LOGI(LOG_TARGET, "%s> prev:%u cur:%u cond:%d (%d)\n",
			__FUNCTION__, prev_time_dm_send, cur_ktime, TIME_TO_SEND_ALIVE, cur_ktime -prev_time_dm_send);
	}

	if(prev_time_dm_send == 0)
	{
		prev_time_dm_send = cur_ktime;
		return;
	}

	if(cur_ktime - prev_time_dm_send > TIME_TO_SEND_ALIVE)
	{
		prev_time_dm_send = cur_ktime;
		dmmgr_send(eEVENT_LOG, "Alive", 0);
		get_watchdog_status(); //send watchdog status into dm-server
	}
}

