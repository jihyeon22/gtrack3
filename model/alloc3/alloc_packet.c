#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "alloc_packet.h"
//#include "hdlc_async.h"
#include "config.h"
//#include "power.h"

#include <base/gpstool.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/mileage.h>
#include <board/power.h>
#include <board/battery.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/nettool.h>
#include <util/stackdump.h>

#include <time.h>
#include <board/modem-time.h>

#include "logd/logd_rpc.h"
#include "netcom.h"
#include "data-list.h"
#include "alloc_util.h"
#include "alloc_packet_tool.h"
#include "tagging.h"
#include "geofence.h"


////////////////////////////////////////////////////////////////////////////////////////
// define......
////////////////////////////////////////////////////////////////////////////////////////
#define LOG_TARGET eSVC_MODEL

#define TMP_2_DATA_BUFF_SIZE	64
#define TMP_1_DATA_BUFF_SIZE	1024

////////////////////////////////////////////////////////////////////////////////////////
// global var
////////////////////////////////////////////////////////////////////////////////////////
extern int g_rfid_complelte_flag;

////////////////////////////////////////////////////////////////////////////////////////
// period pkt info
////////////////////////////////////////////////////////////////////////////////////////
pkt_define_t allocation_period_pkt[] = {
	{ ePKT_PERI_IDX_PROTCOL_ID, 		1, 	"Protocol id" },
	{ ePKT_PERI_IDX_MSG_TYPE,			1, 	"Msg Type" },
	{ ePKT_PERI_IDX_DATA_CONTINUOUS,	1, 	"Data Continus"},
	{ ePKT_PERI_IDX_DEV_ID,		 		15,	"device ID" },
	{ ePKT_PERI_IDX_EVT_CODE,			1,	"Event Code" },
	{ ePKT_PERI_IDX_EVT_DATA,			3,	"Event Data" },
	{ ePKT_PERI_IDX_DATE,				14,	"Date" },
	{ ePKT_PERI_IDX_GPS_STAT,			1,	"Gps Status" },
	{ ePKT_PERI_IDX_GPS_LAT,			11,	"GPS latitude" },
	{ ePKT_PERI_IDX_GPS_LOT,			11,	"GPS longitude"},
	{ ePKT_PERI_IDX_GPS_DIR,			1,	"GPS Dir"},
	{ ePKT_PERI_IDX_GPS_SPEED,			4,	"GPS Speed"},
	{ ePKT_PERI_IDX_TOTAL_DIST,			6,	"Total Distance"},
	{ ePKT_PERI_IDX_TEMP1,				5,	"Thermal 1"},
	{ ePKT_PERI_IDX_TEMP2,				5,	"Thermal 2"},
	{ ePKT_PERI_IDX_TEMP3,				5,	"Thermal 3"},
	{ ePKT_PERI_IDX_REPORT_CYCLE_SEC,	4,	"Report Sec"},
	{ ePKT_PERI_IDX_GPIO_STAT,			1,	"Gpio Stat"},
	{ ePKT_PERI_IDX_PWR_TYPE,			1, 	"Power Type"},
	{ ePKT_PERI_IDX_CREAT_CYCLE_SEC,	4,	"Create Sec"},
	{ ePKT_PERI_IDX_CAR_BATT_VOLT,		4,	"Car Batt"},
	{ ePKT_PERI_IDX_STOP_ZONE_ID,		8,	"Stop Zone ID"},
	{ ePKT_PERI_IDX_STOP_ZONE_IDX,		2,	"Stop Zone Idx" },
	{ ePKT_PERI_IDX_STOP_ZONE_STAT,		1,	"Stop Zone Stat" },
	{ ePKT_PERI_IDX_MAX_PREIOD_PKT,		-1, "null" },
};

////////////////////////////////////////////////////////////////////////////////////////
// binary receive pkt info
////////////////////////////////////////////////////////////////////////////////////////
pkt_define_t allocation_get_passenger_info_pkt[] = {
	{ ePKT_PESG_IDX_PROTCOL_ID, 			1,	"Protocol id" },
	{ ePKT_PESG_IDX_DEV_ID,					15,	"device ID" },
	{ ePKT_PESG_IDX_VERSION_INFO, 			8,	"Passenger Ver Info"},
	{ ePKT_PESG_IDX_MAX_PASSENGER_INFO_PKT, -1, "null" }
};



// ----------------------------------------------------------------------------------------
//  packet define..
// ----------------------------------------------------------------------------------------
int mkpkt_get_passenger(unsigned char ** buff, char* phonenum, char* version_info)
{
	int pkt_size = 0;
	
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};
	char tmp_buff_2[TMP_2_DATA_BUFF_SIZE] = {0,};
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );
	
	// ePKT_PESG_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_GET_PASSENGER);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PESG_IDX_DEV_ID
	memset(tmp_buff_2, 0x20, MAX_DEV_ID_LED);
	tmp_buff_2[MAX_DEV_ID_LED] = '\0';
	strncpy(tmp_buff_2, phonenum, strlen(phonenum));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%15s", tmp_buff_2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PESG_IDX_MAX_PASSENGER_INFO_PKT
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%.8s", version_info);

	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );
	
	*buff = malloc(pkt_size);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	return pkt_size;
}


// ----------------------------------------------------------------------------------------
//  packet define..
// ----------------------------------------------------------------------------------------
int mkpkt_report_data(	unsigned char ** buff, char ev_code, char* ev_data, char *phonenum, 
						int temp1, int temp2, int temp3, 
						gpsData_t gpsdata, 
						char* zone_id, int zone_idx, int zone_state, 
						int total_dist_cm)
{
	int pkt_size = 0;
	
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};
	char tmp_buff_2[TMP_2_DATA_BUFF_SIZE] = {0,};
	int tmp_size = 0;
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );
	
	// ePKT_PERI_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_PERIOD_REPORT);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_MSG_TYPE
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", eMSG_TYPE_PREIOD_REPORT);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	
	// ePKT_PERI_IDX_DATA_CONTINUOUS
	// 0 : no continue data
	// 1 : continue data 
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_DEV_ID
	memset(tmp_buff_2, 0x20, MAX_DEV_ID_LED);
	tmp_buff_2[MAX_DEV_ID_LED] = '\0';
	strncpy(tmp_buff_2, phonenum, strlen(phonenum));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%15s", tmp_buff_2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_EVT_CODE
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ev_code);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_EVT_DATA
	tmp_size = strlen(ev_data);
	strncpy(tmp_buff_2, "000\0", sizeof(tmp_buff_2));
	strncpy(tmp_buff_2, ev_data, MIN(allocation_period_pkt[ePKT_PERI_IDX_EVT_DATA].field_size, tmp_size) );
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s", tmp_buff_2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_DATE				// 14
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%04d", gpsdata.year);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%02d", gpsdata.mon);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%02d", gpsdata.day);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%02d", gpsdata.hour);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%02d", gpsdata.min);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%02d", gpsdata.sec);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_GPS_STAT				// 1
	if(gpsdata.active == 1)
		pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", eSAT_GSP);
	else
		pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", eNOSIGNAL_GPS);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	if(gpsdata.lat == 0 || gpsdata.lon == 0)
	{
		gpsData_t last_gpsdata;
		gps_valid_data_get(&last_gpsdata);

        gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;

		printf("last gpsdata[%.7f]/[%.7f]\r\n", gpsdata.lat, gpsdata.lon);
	}
	// ePKT_PERI_IDX_GPS_LAT				// 11
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%.7f", gpsdata.lat);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_GPS_LOT				// 11
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%.7f", gpsdata.lon);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_GPS_DIR,			// 1
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", convert_angle(gpsdata.angle));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_GPS_SPEED,			// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", gpsdata.speed);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_TOTAL_DIST,		// 6
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", total_dist_cm);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", temp1);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_TEMP2,				// 5
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", temp2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_TEMP3,				// 5
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", temp3);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_REPORT_CYCLE_SEC,	// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d",  get_report_interval());
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_GPIO_STAT,			// 1
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_PWR_TYPE,			// 1
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	//ePKT_PERI_IDX_CREAT_CYCLE_SEC,	// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", get_report_interval());
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_CAR_BATT_VOLT,		// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%4.1f", (float)(battery_get_battlevel_car()/1000.0));
	//pkt_size += sprintf(tmp_buff_1 + pkt_size, "12");
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_STOP_ZONE_ID,		// 8
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s", zone_id);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_STOP_ZONE_IDX,		// 2
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", zone_idx);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_STOP_ZONE_STAT,	// 1
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", zone_state);
	//pkt_size += sprintf(buff + pkt_size, "%c", PACKET_SPILT);
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );
	
	//printf("tmp_buff_1 is [%s] / [%d] \r\n", tmp_buff_1, pkt_size);
	*buff = malloc(pkt_size);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	return pkt_size;
}


// ----------------------------------------------------------------------------------------
//  packet define..
// ----------------------------------------------------------------------------------------
int mkpkt_tag_data(	unsigned char ** buff, char *phonenum,
						int count, char* zone_id, char* date,
						int num_rfid, char* list_rfid)
{
	int pkt_size = 0;
	
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};
	char tmp_buff_2[TMP_2_DATA_BUFF_SIZE] = {0,};
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );
	
	// ePKT_PERI_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_TAG_REPORT);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_DEV_ID
	memset(tmp_buff_2, 0x20, MAX_DEV_ID_LED);
	tmp_buff_2[MAX_DEV_ID_LED] = '\0';
	strncpy(tmp_buff_2, phonenum, strlen(phonenum));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%.15s", tmp_buff_2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// // ePKT_PERI_IDX_COUNT
	// pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", count);
	// pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_STOP_ZONE_ID,		// 8
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%.8s", zone_id);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_DATE				// 12
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%.14s", date);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_NUM_RFID,		// 2
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", num_rfid);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_LIST_RFID,	// 350
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s", list_rfid);
	//pkt_size += sprintf(buff + pkt_size, "%c", PACKET_SPILT);
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );
	
	//printf("tmp_buff_1 is [%s] / [%d] \r\n", tmp_buff_1, pkt_size);
	*buff = malloc(pkt_size);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	return pkt_size;
}


// ----------------------------------------------------------------------------------------
//  packet define..
// ----------------------------------------------------------------------------------------
int mkpkt_sms_resp_tcp_data(unsigned char ** buff, char *phonenum)
{
	int pkt_size = 0;
	
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};
	char tmp_buff_2[TMP_2_DATA_BUFF_SIZE] = {0,};

	configurationModel_t *conf = get_config_model();
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );
	
	// ePKT_PERI_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_RESP_SMS);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_MSG_TYPE
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", eMSG_TYPE_RESP_SMS);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_DEV_ID
	memset(tmp_buff_2, 0x20, MAX_DEV_ID_LED);
	tmp_buff_2[MAX_DEV_ID_LED] = '\0';
	strncpy(tmp_buff_2, phonenum, strlen(phonenum));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%15s", tmp_buff_2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_PWR_TYPE,			// 1
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s",  conf->model.report_ip);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d",  conf->model.report_port);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d/%d/%d/%d",
		conf->model.report_interval_keyon, conf->model.report_interval_keyon,
		conf->model.report_interval_keyoff, conf->model.report_interval_keyoff);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", get_rssi_gps());
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_DTG_STAT,			// 0
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
		
	// ePKT_PERI_IDX_CAR_BATT_VOLT,		// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%4.1f", (float)(battery_get_battlevel_car()/1000.0));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_CAR_BATT_VOLT,		// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%4.1f", (float)(battery_get_battlevel_internal()/1000.0));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_TEMP_STAUS,			// 0
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_IGN_CTRL,			// 0
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_VERSION,
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s", VERSION_APP_HARDCODING);
	
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );
	
	//printf("tmp_buff_1 is [%s] / [%d] \r\n", tmp_buff_1, pkt_size);
	*buff = malloc(pkt_size);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	return pkt_size;
}

//jwrho++
int mkpkt_bus_stop_info_tcp_data(unsigned char ** buff, char *phonenum)
{
	int pkt_size = 0;
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};
	char tmp_buff_2[TMP_2_DATA_BUFF_SIZE] = {0,};

	struct tm cur_time;
	time_t system_time;
	struct tm *timeinfo;

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );
	
	// ePKT_PERI_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_BUS_STOP_INFO);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
	
	// ePKT_PERI_IDX_DEV_ID
	memset(tmp_buff_2, 0x20, MAX_DEV_ID_LED);
	tmp_buff_2[MAX_DEV_ID_LED] = '\0';
	strncpy(tmp_buff_2, phonenum, strlen(phonenum));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%15s", tmp_buff_2);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	//TODO : date insert

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%04d%02d%02d%02d%02d%02d", 
																			timeinfo->tm_year+1900,
																			timeinfo->tm_mon+1,
																			timeinfo->tm_mday,
																			timeinfo->tm_hour,
																			timeinfo->tm_min,
																			timeinfo->tm_sec);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );


	*buff = malloc(pkt_size);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	return pkt_size;
}
//jwrho--

// ----------------------------------------------------------------------------------------
//  packet define..
// ----------------------------------------------------------------------------------------
int mkpkt_sms_resp_sms_data(unsigned char ** buff)
{
	int pkt_size = 0;
	
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};

	configurationModel_t *conf = get_config_model();

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );

	// ePKT_PERI_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_RESP_SMS);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_MSG_TYPE
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", eMSG_TYPE_RESP_SMS);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_PWR_TYPE,			// 1
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s",  conf->model.report_ip);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d",  conf->model.report_port);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d/%d/%d/%d",
		conf->model.report_interval_keyon, conf->model.report_interval_keyon,
		conf->model.report_interval_keyoff, conf->model.report_interval_keyoff);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", get_rssi_gps());
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_DTG_STAT,			// 0
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);
		
	// ePKT_PERI_IDX_CAR_BATT_VOLT,		// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%4.1f", (float)(battery_get_battlevel_car()/1000.0));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_CAR_BATT_VOLT,		// 4
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%4.1f", (float)(battery_get_battlevel_internal()/1000.0));
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_TEMP_STAUS,			// 0
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_IGN_CTRL,			// 0
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", 0);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// ePKT_PERI_IDX_VERSION,
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%s", VERSION_APP_HARDCODING);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", SMS_END_CHAR );

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );
	
	printf("tmp_buff_1 is [%s] / [%d] \r\n", tmp_buff_1, pkt_size);
	*buff = malloc(pkt_size+1);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	(*buff)[pkt_size] = '\0';

	int i = 0;
	for(i = 0; i<= pkt_size; i++)
	{
		printf("%x", (*buff)[i]);
	}
	printf("\n");
	
	printf("buff is [%s] / [%d] \r\n", *buff, pkt_size);
	return pkt_size;
}
// ----------------------------------------------------------------------------------------
//  packet define..
// ----------------------------------------------------------------------------------------
int mkpkt_sms_resp_dm_data(unsigned char ** buff, int setup)
{
	int pkt_size = 0;
	
	char tmp_buff_1[TMP_1_DATA_BUFF_SIZE] = {0,};

	configurationModel_t *conf = get_config_model();

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_START_CHAR );

	printf("ePROTOCOL_ID_RESP_DM : %c\n", ePROTOCOL_ID_RESP_DM);
	// ePKT_PERI_IDX_PROTCOL_ID
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", ePROTOCOL_ID_RESP_DM);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);

	// // ePKT_PERI_IDX_MSG_TYPE
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%d", setup);
	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_SPILT);	

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", SMS_END_CHAR );

	pkt_size += sprintf(tmp_buff_1 + pkt_size, "%c", PACKET_END_CHAR );
	
	printf("tmp_buff_1 is [%s] / [%d] \r\n", tmp_buff_1, pkt_size);
	*buff = malloc(pkt_size+1);
	
	memcpy(*buff, tmp_buff_1, pkt_size);
	(*buff)[pkt_size] = '\0';

	int i = 0;
	for(i = 0; i<= pkt_size; i++)
	{
		printf("%x", (*buff)[i]);
	}
	printf("\n");
	
	printf("buff is [%s] / [%d] \r\n", *buff, pkt_size);
	return pkt_size;
}
// ==============================================================
// ==============================================================
void pkt_send_period_report(gpsData_t gpsdata)
{
	alloc_evt_pkt_info_t pkt_info;
	printf("send timimg!!!!!!\r\n");
	
	pkt_info.gpsdata = gpsdata;
	pkt_info.diff_distance = mileage_get_m() * 100;
	
	//devel_webdm_send_log("debug : [%s] : [%d]", __func__, pkt_info.diff_distance);
	LOGI(LOG_TARGET, "MAKE PKT : REPORT : distance [%d]\r\n", pkt_info.diff_distance);
	
	mileage_set_m(0);
	
	sender_add_data_to_buffer(PACKET_TYPE_REPORT, &pkt_info, ePIPE_1);
}

void pkt_send_tagging(void)
{
	printf("send tagging!!!!!!\r\n");
/*
	if(get_first_geo_fence() == -1)
		return;
*/
	void *data = NULL;

	while(list_pop(&tagging_data_list, &data) >= 0)
	{
		LOGI(LOG_TARGET, "MAKE PKT : TAGGING\r\n");

		sender_add_data_to_buffer(PACKET_TYPE_TAGGING, data, ePIPE_1);	
	}
}

void pkt_send_geofence_in(gpsData_t gpsdata, int fence_idx)
{
	alloc_evt_pkt_info_t evt_info;

	memset(&evt_info, 0, sizeof(evt_info));
	
	evt_info.gpsdata = gpsdata;
	evt_info.diff_distance =  mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : GEOFENCE IN : geofence idx [%d] / distance [%d]\r\n", fence_idx, evt_info.diff_distance);
	mileage_set_m(0);
	
	// geofence 의 id 로 정류장의 id를 얻어온다.
	get_geo_fence_id(fence_idx, evt_info.zone_id);
	

	evt_info.zone_idx = fence_idx;
	evt_info.zone_stat = SUB_EVT_CODE_GEOFENCE_IN;
	
	sender_add_data_to_buffer(PACKET_TYPE_GEO_FENCE_IN, &evt_info, ePIPE_1);
}


void pkt_send_geofence_out(gpsData_t gpsdata, int fence_idx)
{
	alloc_evt_pkt_info_t evt_info;

	memset(&evt_info, 0, sizeof(evt_info));
	
	evt_info.gpsdata = gpsdata;
	evt_info.diff_distance =  mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : GEOFENCE OUT : geofence idx [%d] / distance [%d]\r\n", fence_idx, evt_info.diff_distance);
	
	mileage_set_m(0);

	// geofence 의 id 로 정류장의 id를 얻어온다.
	get_geo_fence_id(fence_idx, evt_info.zone_id);
	
	evt_info.zone_idx = fence_idx;
	evt_info.zone_stat = SUB_EVT_CODE_GEOFENCE_OUT;

	sender_add_data_to_buffer(PACKET_TYPE_GEO_FENCE_OUT, &evt_info, ePIPE_1);
}

void pkt_send_get_rfid()
{
	char rfid_version_buff[8] = {0,};
	int ret = 0;
	alloc_get_rfid_pkt_info_t get_rfid_data;
	
	ret = load_file_passenger_info(rfid_version_buff);
	
	if (ret == 0)
	{
		// get_passenger_info_ver(&rfid_version_buff);
		strncpy(get_rfid_data.version, rfid_version_buff, 8);
	}
	else
	{
		init_passenger_info();
		strncpy(get_rfid_data.version, "00000000", 8);
	}
	
	LOGI(LOG_TARGET, "MAKE PKT : GET RFID : rfid ver [%s]\r\n", get_rfid_data.version);	
	
	sender_add_data_to_buffer(PACKET_TYPE_GET_RFID, &get_rfid_data, ePIPE_1);
}

void pkt_send_get_geofence()
{
	alloc_evt_pkt_info_t pkt_info;
	gpsData_t gpsdata;
	
	gps_get_curr_data(&gpsdata);
	
	pkt_info.gpsdata = gpsdata;
	pkt_info.diff_distance = mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : GET GEOFENCE\r\n");
	
	mileage_set_m(0);
	
	if (g_rfid_complelte_flag == 1)
		sender_add_data_to_buffer(PACKET_TYPE_GET_GEOFENCE, &pkt_info, ePIPE_1);
	
}

void pkt_send_start_drive()
{
	alloc_evt_pkt_info_t evt_info;
	gpsData_t gpsdata;

	memset(&evt_info, 0, sizeof(evt_info));
	
	gps_get_curr_data(&gpsdata);
	evt_info.gpsdata = gpsdata;
	evt_info.diff_distance =  mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : START DRIVE : distance [%d]\r\n", evt_info.diff_distance);
	
	mileage_set_m(0);
	
	evt_info.evt_code = eEVT_CODE_START_DRIVE;
	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &evt_info, ePIPE_1);
}

void pkt_send_end_drive()
{
	alloc_evt_pkt_info_t evt_info;
	gpsData_t gpsdata;

	memset(&evt_info, 0, sizeof(evt_info));
	
	gps_get_curr_data(&gpsdata);
	evt_info.gpsdata = gpsdata;
	evt_info.diff_distance =  mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : END DRIVE : distance [%d]\r\n", evt_info.diff_distance);
	
	mileage_set_m(0);
	
	evt_info.evt_code = eEVT_CODE_END_DRIVE;
	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &evt_info, ePIPE_1);
}


void pkt_send_btn_emergency()
{
	alloc_evt_pkt_info_t evt_info;
	gpsData_t gpsdata;

	memset(&evt_info, 0, sizeof(evt_info));
	
	gps_get_curr_data(&gpsdata);
	evt_info.gpsdata = gpsdata;
	evt_info.diff_distance =  mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : EMERGENCY BTN : distance [%d]\r\n", evt_info.diff_distance);
	
	mileage_set_m(0);
	
	evt_info.evt_code = eEVT_CODE_EMERGENCY_BTN;
	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &evt_info, ePIPE_1);
}

void pkt_send_btn_passenger_full()
{
	alloc_evt_pkt_info_t evt_info;
	gpsData_t gpsdata;

	memset(&evt_info, 0, sizeof(evt_info));
	
	gps_get_curr_data(&gpsdata);
	evt_info.gpsdata = gpsdata;
	evt_info.diff_distance =  mileage_get_m() * 100;
	
	LOGI(LOG_TARGET, "MAKE PKT : PASSENGER FULL : distance [%d]\r\n", evt_info.diff_distance);
	
	mileage_set_m(0);
	
	evt_info.evt_code = eEVT_CODE_PASSENGER_FULL;
	sender_add_data_to_buffer(PACKET_TYPE_EVENT, &evt_info, ePIPE_1);
}

