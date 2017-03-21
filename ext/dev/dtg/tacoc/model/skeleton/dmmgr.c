#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <atcmd.h>
#include <board/battery.h>
#include <dm/dm.h>
#include <dm/event.h>
#include <dm/util.h>
#include <dm/package.h>
#include <dm/config.h>
#include <dm/update.h>
#include <dm/sys.h>
#include <base/dmmgr.h>
#include <dtg_regist_process.h>
#include <dtg_data_manage.h>
#include <common/gpio.h>
#include <util/nettool.h>


#include <wrapper/dtg_log.h>

static int g_incomplete_event_buffer[eEVENT_MAX] = {0};

//ini : "/system/mds/system/bin/dm.ini";
//pkg : "/system/mds/system/bin/PACKAGE"
int // dmmgr_init(char *ini, char *pkg)
{
	dm_info *dmi = NULL;
	dm_res res;

	DTG_LOGD("**** %s-%s EXAMPLE****\n", libdm_name, libdm_version);
	res = dm_global_init();
	if (res != DM_OK) {
		DTG_LOGE("dm_global_init failure\n");
		return -1;
	}

	dmi = dm_get_info();
	if (dmi == NULL) {
		DTG_LOGE("dm infomation NULL\n");
		return -1;
	}
	dmi->user_ini = strdup(ini);
	dmi->user_package = strdup(pkg);

	res = dm_load_ini(dmi, dmi->user_ini);
	if (res != DM_OK) {
		DTG_LOGE("dm_load_ini failure\n");
		return -1;
	}

	dmi->version.hw 	= dm_get_hw_version();
	dmi->version.amss 	= dm_get_amss_version();
	dmi->version.linu 	= dm_get_linux_version();

	dmi->modem.imei		= strdup(atcmd_get_imei());
	dmi->modem.phone	= strdup(atcmd_get_phonenum());

	dmi->package.name 	= dm_load_package_file(dmi->user_package);

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	dmi->gpio.ign		= IGN_LINE_CHECK_GPIO;		// fix
	dmi->gpio.epwr		= DC_LINE_CHECK_GPIO;		// fix

	dmi->func.epwr_get	= battery_get_battlevel_car_dm;
	dmi->func.batt_get	= battery_get_battlevel_internal_dm;

	return 0;
}


int dmmgr_send(dmEventType_t type, const char *log, int evo_errno)
{
	int res = 0;
	
	DTG_LOGD("%s %d\n", __FUNCTION__, type);

	if(type >= eEVENT_MAX)
	{
		return -1;
	}

	if(nettool_get_state() == DEFINES_MDS_NOK) //No PPP Device
		return -1;

	switch(type)
	{
		case eEVENT_PWR_ON:
			res = dm_event(	DM_EVENT_PWR_ON,
			                EVO_UP_VER      |
							EVO_IMEI        | 	
							EVO_HW_VER      | 
							EVO_AMSS_VER    | 
							EVO_LINUX_VER   |
							EVO_TELECOM     | 
							EVO_PHONE       | 
							EVO_MAIN_VOLT   | 
							EVO_BATT_VOLT   |
							EVO_LOG         | 
							EVO_ERRNO,
							log, evo_errno);
			break;
		case eEVENT_PWR_OFF:
			res = dm_event(	DM_EVENT_PWR_OFF, 
			                EVO_UP_VER      |
							EVO_IMEI        | 
							EVO_PHONE       | 
							EVO_MAIN_VOLT   | 
							EVO_BATT_VOLT   |
							EVO_LOG         | 
							EVO_ERRNO,
							log, evo_errno);
			break;
		case eEVENT_KEY_ON:
			res = dm_event(	DM_EVENT_KEY_ON,
			                EVO_UP_VER      |
							EVO_IMEI        | 	
							EVO_HW_VER      | 
							EVO_AMSS_VER    | 
							EVO_LINUX_VER   |
							EVO_TELECOM     | 
							EVO_PHONE       | 
							EVO_MAIN_VOLT   | 
							EVO_BATT_VOLT   |
							EVO_LOG         | 
							EVO_ERRNO,
							log, evo_errno);
			break;
		case eEVENT_KEY_OFF:
			res = dm_event(	DM_EVENT_KEY_OFF,
			                EVO_UP_VER      |
							EVO_IMEI        | 
							EVO_MAIN_VOLT   | 
							EVO_BATT_VOLT   |
							EVO_PHONE       | 
							EVO_LOG         | 
							EVO_ERRNO,
							log, evo_errno);
			break;
		case eEVENT_REPORT:
		{
			res = dm_event(	DM_EVENT_REPORT,
							EVO_IMEI        | 
							EVO_KEY_STAT    | 
							EVO_USR0        | 
							EVO_USR1        | 
							EVO_USR2        | 
							EVO_USR3        | 
							EVO_USR4        | 
							EVO_USR5        |
							EVO_BATT_VOLT   |
							EVO_GPS_LAT     | 
							EVO_GPS_LON     | 
							EVO_GPS_SPEED   | 
							EVO_GPS_HDOP    | 
							EVO_LOG         | 
							EVO_ERRNO,
							log, evo_errno);
			break;
		}
		case eEVENT_STATUS:
		{
			res = dm_event(	DM_EVENT_STATUS,
							EVO_HW_VER      | 
							EVO_AMSS_VER    | 
							EVO_LINUX_VER   |	
							EVO_UP_VER      | 
							EVO_TELECOM     | 
							EVO_IMEI        | 
							EVO_PHONE       | 
							EVO_RSSI        |
							EVO_GP0         | 
							EVO_GP1         | 
							EVO_GP2         | 
							EVO_GP3         | 
							EVO_GP4         | 
							EVO_GP5         | 
							EVO_GP6         | 
							EVO_GP7         | 
							EVO_GP8         | 
							EVO_GP9         |
							EVO_USR0        | 
							EVO_USR1        | 
							EVO_USR2        | 
							EVO_USR3        | 
							EVO_USR4        | 
							EVO_USR5        |
							EVO_MAIN_VOLT   | 
							EVO_BATT_VOLT   |
							EVO_KEY_STAT    | 
							EVO_PWR_STAT    | 
							EVO_GPS_LAT     | 
							EVO_GPS_LON     | 
							EVO_GPS_SPEED   | 
							EVO_GPS_HDOP    |
							EVO_LOG         | 
							EVO_ERRNO,
							log, evo_errno);
			break;
		}
		case eEVENT_LOG:
		{
			res = dm_event(DM_EVENT_LOG, EVO_IMEI | EVO_PHONE | EVO_LOG | EVO_ERRNO, log, evo_errno);
			
			break;
		}
		case eEVENT_WARNING:
		{
			res = dm_event(DM_EVENT_WARNING, EVO_IMEI | EVO_PHONE | EVO_LOG | EVO_ERRNO, log, evo_errno);
			break;
		}
		case eEVENT_BREAKDOWN:
		{
			res = dm_event(DM_EVENT_BREAKDOWN, EVO_IMEI | EVO_LOG | EVO_ERRNO, log, evo_errno);
			break;
		}
		case eEVENT_UPDATE:
			res = dm_event(DM_EVENT_UPDATE,	EVO_IMEI | EVO_PHONE | EVO_UP_VER | EVO_LOG | EVO_ERRNO, log, evo_errno);
			break;
		default:
			;
	}
	
	if (res == DM_EV_RESP_UPDATE) {
		if(dm_update() == DM_OK) {
			DTG_LOGD("Type[%d], UPDATING...........\n", __FUNCTION__, type);
			dm_sys_poweroff(0);
		}
	}
	else if(res != DM_EV_RESP_OK)
	{
		DTG_LOGE("DM Server Error Response [%d]\n", res);
	}

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
		if(g_incomplete_event_buffer[i] == 1)
		{
			g_incomplete_event_buffer[i] = 0;
			dmmgr_send(i, "late event", 0);
		}
	}
}

