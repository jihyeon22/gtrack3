<<<<<<< HEAD
#ifdef TEST_CODE_ENABLE
	#include "depend.h"
#else
	#include <base/sender.h>
	#include <base/devel.h>
	#include <board/power.h>
	#include <board/crit-data.h>
	#include <util/tools.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <base/devel.h>
#include <base/error.h>

#include "config.h"
#include "lotte_gpsmng.h"
#include "lotte_gps_filter.h"
#include "lotte_gps_utill.h"
#include "lotte_geofence.h"
#include "lotte_packet.h"
#include "logd/logd_rpc.h"
#include "debug.h"

lotte_gps_manager_t lgm = {
	.gps_initial_count = GPS_INIT_MAX_WAITING_TIME,
	.first_gps_active = eINACTIVE,
	.server_mileage = 0,
	.gps_mileage = 0,
	.fixed_gpsdata.active = eINACTIVE
};

int enable_gps_log = 0;

void init_gps_manager()
{
	lgm.gps_initial_count = GPS_INIT_MAX_WAITING_TIME;
	lgm.first_gps_active = eINACTIVE;
	lgm.fixed_gpsdata.active = eINACTIVE;
}

//*
lotte_gps_manager_t* load_gps_manager()
{
	return &lgm;
}
//*/


int iscollection_gps_time(gpsData_t *pData)
{
	int diff_time;
	if(lgm.first_gps_active == eINACTIVE)
		return 0;

	diff_time = difftime(pData->utc_sec, lgm.reported_gpsdata.utc_sec);
	if( diff_time >= get_collection_interval())
		return 1;

	return 0;
}


void _record_last_gps_data(gpsData_t *pData)
{
	memcpy(&lgm.last_gpsdata, pData, sizeof(gpsData_t));
}

void _record_reported_gps_data(gpsData_t *pData)
{
	memcpy(&lgm.reported_gpsdata, pData, sizeof(gpsData_t));
}

void _record_last_fixed_gps_data(gpsData_t *pData, int ign_on)
{
	static time_t last_mileage_time = 0;
	static int move_detect = 0;
	static float lat = 0;
	static float lon = 0;
	static unsigned int count = 0;

	if(enable_gps_log)
	LOGI(LOG_TARGET, "++Mileage:%d %d %d/%f %f/%d %d/%d %d",
		ign_on, lgm.fixed_gpsdata.active,count, lat, lon, move_detect, pData->speed,
		pData->utc_sec, last_mileage_time);
	unsigned int status_bit = 0;

	if(ign_on != POWER_IGNITION_OFF)
	{
		count++;
		status_bit += 1;
	}
	
	if(lgm.fixed_gpsdata.active) 
	{
		status_bit += 1<<1;
		if(lat == 0 || lon == 0)
		{
			lat = lgm.fixed_gpsdata.lat;
			lon = lgm.fixed_gpsdata.lon;
			status_bit += 1<<2;
		}
	
		if(ign_on != POWER_IGNITION_OFF)
		{
			status_bit += 1<<3;
			if(pData->speed != 0)
			{
				move_detect = 1;
				status_bit += 1<<4;
			}
		
			if(pData->utc_sec - last_mileage_time >= MILEAGE_INTERVAL || count >= MILEAGE_INTERVAL )
			{
				status_bit += 1<<5;
				count = 0;
				if(pData->utc_sec - last_mileage_time < 0)
				{
					devel_webdm_send_log("ERROR: abnormal gps utc sec! %d %d", pData->utc_sec, last_mileage_time);
				}
				
				last_mileage_time = pData->utc_sec;
				
				if(move_detect == 1)
				{
					double tmp_mileage = 0, tmp_dist = 0;
				
					status_bit += 1<<6;

					tmp_dist = get_distance_meter(lat, pData->lat, lon, pData->lon);
					if(tmp_dist < 0)
					{
						return;
					}
					
					tmp_mileage = lgm.gps_mileage + tmp_dist;
					
					if(isnan(tmp_mileage) != 0)
					{
						static int send_log = 0;
						if(send_log == 0)
						{
							devel_webdm_send_log("_record_last_fixed_gps_data : nan error");
							send_log = 1;
						}
						
						LOGE(LOG_TARGET, "mileage result is nan. Dist:%f Mileage:before-%f after-%f\n", tmp_dist, lgm.gps_mileage, tmp_mileage);
						
						return;
					}

					move_detect = 0;
					
					lgm.gps_mileage = tmp_mileage;
					crit_set_data_mileage_write(lgm.server_mileage + lgm.gps_mileage);
					lat = pData->lat;
					lon = pData->lon;

					LOGI(LOG_TARGET, "move detect! %f %f %f", lgm.gps_mileage, lat, lon);
				}
			}
		}
	}

	memcpy(&lgm.fixed_gpsdata, pData, sizeof(gpsData_t));
	if(enable_gps_log)
	LOGI(LOG_TARGET, "--Mileage:%d %d %d/%f %f/%d %d/%d %d/%01d%01d%01d%01d%01d%01d%01d",
		ign_on, lgm.fixed_gpsdata.active,count, lat, lon, move_detect, pData->speed,
		pData->utc_sec, last_mileage_time,
		CheckBit(status_bit, 0),  CheckBit(status_bit, 1), CheckBit(status_bit, 2), CheckBit(status_bit, 3),
		CheckBit(status_bit, 4), CheckBit(status_bit, 5), CheckBit(status_bit, 6));
}


#ifdef FEATURE_GEO_FENCE_SIMULATION
extern int geo_test_flag;
#endif

int check_geofence(gpsData_t *pData)
{
	char smsmsg[100] = {0,};
	
	fence_notification_t fence_chk;

#ifdef FEATURE_GEO_FENCE_SIMULATION
	if(geo_test_flag == 0) //geo in
	{
		pData->lat = 37.399693;
		pData->lon = 127.100937;
	}

	else if(geo_test_flag == 1) //geo out
	{
		pData->lat = 37.394865;
		pData->lon = 127.105379;
	}
#endif


	if(get_geo_fence0_enable()) {
		fence_chk = get_geofence0_notification(*pData);
		if(fence_chk == eFENCE_IN_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #0 entry notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM0_ENTRY_EVT , NULL, ePIPE_1);

			sprintf(smsmsg, "fence0 pos lat:%3.7f, lon:%3.7f entry\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
		else if(fence_chk == eFENCE_OUT_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #0 exit notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM0_EXIT_EVT  , NULL, ePIPE_1);

			sprintf(smsmsg, "fence0 pos lat:%3.7f, lon:%3.7f exit\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
	}

	if(get_geo_fence1_enable()) {
		fence_chk = get_geofence1_notification(*pData);
		if(fence_chk == eFENCE_IN_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #1 entry notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM1_ENTRY_EVT , NULL, ePIPE_1);

			sprintf(smsmsg, "fence1 pos lat:%3.7f, lon:%3.7f entry\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
		else if(fence_chk == eFENCE_OUT_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #1 exit notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM1_EXIT_EVT  , NULL, ePIPE_1);

			sprintf(smsmsg, "fence1 pos lat:%3.7f, lon:%3.7f exit\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
	}
	return 0;
}

void recovery_gps_data(gpsData_t *pData)
{

#if defined(FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS) && defined(FEATURE_INVALID_GPS_COPY_ZERO)
	#error "FEATURE_INACTIVE_GPS_COPY_LAST_FIX_GPS, FEATURE_INVALID_GPS_COPY_ZERO both define error"
#endif

#ifdef FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS	
	if(lgm.first_gps_active == eACTIVE) 
	{
		pData->lat       = lgm.fixed_gpsdata.lat;
		pData->lon       = lgm.fixed_gpsdata.lon;
		pData->speed     = lgm.fixed_gpsdata.speed;
		pData->angle     = lgm.fixed_gpsdata.angle;
		pData->satellite = lgm.fixed_gpsdata.satellite; //keep satellite status
		pData->hdop      = lgm.fixed_gpsdata.hdop; //keep HDOP status
	}
	else
	{
		pData->lat       = 0;
		pData->lon       = 0;
		pData->speed     = 0;
		pData->angle     = 0;
		pData->satellite = 0; //keep satellite status
		pData->hdop      = 0; //keep HDOP status
	}
#endif

#ifdef FEATURE_INVALID_GPS_COPY_ZERO	
	pData->lat       = 0;
	pData->lon       = 0;
	//pData->speed     = 0;
	//pData->angle     = 0;
	//pData->satellite = 0; //keep satellite status
	//pData->hdop      = 0; //keep HDOP status
#endif

}


time_t get_device_time()
{
	time_t t = time(NULL);
	return t;
}

int fake_gps_time(gpsData_t *pData)
{
#ifdef FEATURE_FAKE_TIME_INACTIVE_GPS
	time_t ct;
	struct tm *tm;

	ct = get_device_time();
	tm = localtime(&ct);

	pData->year = tm->tm_year + 1900;
	pData->mon = tm->tm_mon + 1;
	pData->day = tm->tm_mday;
	pData->hour = tm->tm_hour;
	pData->min = tm->tm_min;
	pData->sec = tm->tm_sec;
	pData->utc_sec = ct;
#endif

	return 0;
}

gps_condition_t inactive_gps_process(gpsData_t cur_gps, gpsData_t *pData)
{
	gps_condition_t ret = eUNKNOWN_GPS_DATA;

	check_gps_invalid(&cur_gps , power_get_ignition_status());

	memset(pData, 0x00, sizeof(gpsData_t));
	switch(lgm.first_gps_active) 
	{
		case eINACTIVE:
			if(lgm.gps_initial_count <= 0) {
				//fake_gps_time(pData);
				lgm.first_gps_active = eACTIVE;
				ret = eUSE_GPS_DATA;
			}
			else {
				lgm.gps_initial_count--;
				ret = eSKIP_NO_INITIAL_GPS; //skip gps data
			}
			break;
		case eACTIVE:
			//fake_gps_time(pData);
			ret = eUSE_GPS_DATA;
			recovery_gps_data(&cur_gps);
			break;
		default:
			ret = eUNKNOWN_GPS_DATA;
			break;
	}

	if(ret == eUSE_GPS_DATA) {
		_record_last_gps_data(&cur_gps);

		if(iscollection_gps_time(&cur_gps) == 0)
			ret = eSKIP_NO_COLLECTION_TIME;
		else
			_record_reported_gps_data(&cur_gps);

		memcpy(pData, &cur_gps, sizeof(gpsData_t));
	}

	return ret;
}

gps_condition_t gps_filter(gpsData_t cur_gps)
{
	gps_condition_t ret = eUSE_GPS_DATA;

	configurationModel_t *conf = NULL;
	conf = get_config_model();


	if(conf->model.dist_filter_enable)
	{
		if(distance_filter(cur_gps, lgm.last_gpsdata, conf->model.dist_filter_value))
			ret = eSKIP_DISTANCE_FILTER;
	}

	if(conf->model.sat_filter_enable)
	{
		if(trust_satlites_filter(cur_gps, conf->model.sat_filter_value))
			ret = eSKIP_TRUST_SATLITES_FILTER;
	}


	if(conf->model.hdop_filter_enable)
	{
		if(trust_HDOP_filter(cur_gps, conf->model.hdop_filter_value))
			ret = eSKIP_TRUST_HDOP_FILTER;
	}


	if(conf->model.speed_filter_enable) 
	{
		if(trust_speed_filter(cur_gps, conf->model.speed_filter_value))
			ret = eSKIP_TRUST_SPEED_FILTER;
	}

	return ret;
}

void fix_gps_pos_in_key_off(gpsData_t *cur_gps, int ign_on)
{
	if(lgm.fixed_gpsdata.active) 
	{
		if(ign_on == POWER_IGNITION_OFF)
		{
			cur_gps->lat       = lgm.fixed_gpsdata.lat;
			cur_gps->lon       = lgm.fixed_gpsdata.lon;
			cur_gps->speed     = 0;
			cur_gps->angle     = lgm.fixed_gpsdata.angle;
		}
	}

}

gps_condition_t active_gps_process(gpsData_t cur_gps, gpsData_t *pData)
{
	int ign_on;
	gps_condition_t ret = eUSE_GPS_DATA;
	memset(pData, 0x00, sizeof(gpsData_t));

#ifdef FEATURE_KEYOFF_FIX_LAST_POSITION
	ign_on = power_get_ignition_status();
#else
	ign_on = POWER_IGNITION_ON;
#endif

	check_gps_invalid(&cur_gps , ign_on);

	fix_gps_pos_in_key_off(&cur_gps, ign_on);

	ret = gps_filter(cur_gps);
	_record_last_gps_data(&cur_gps);
	
	if(enable_gps_log)
	LOGI(LOG_TARGET, "++active_gps_process:%d %d/%f %f %d",
		ign_on, ret, cur_gps.lat, cur_gps.lon, cur_gps.speed);
		
	if(ret == eUSE_GPS_DATA)
	{
		//valid gps record.
		lgm.first_gps_active = eACTIVE;
		_record_last_fixed_gps_data(&cur_gps, ign_on);

		//geo fence check
		check_geofence(&cur_gps);
	}

	if(iscollection_gps_time(&cur_gps) == 0)
		return eSKIP_NO_COLLECTION_TIME;

	if(ret != eUSE_GPS_DATA) //filtering gps
		recovery_gps_data(&cur_gps);

	_record_reported_gps_data(&cur_gps);
	memcpy(pData, &cur_gps, sizeof(gpsData_t));

	return ret;
}

	
int get_server_mileage()
{
	return lgm.server_mileage;
}

int get_gps_mileage()
{
	return lgm.gps_mileage;
}

void set_server_mileage(int mileage)
{
	lgm.server_mileage = mileage; 
}

void set_gps_mileage(int mileage)
{
	lgm.gps_mileage = mileage;
}

void check_gps_invalid(gpsData_t *pCurData , int ign)
{
	static int count = 0;
	static int send_log = 0;

	if(ign == 0)
	{
		count = 0;
		return;
	}

	if(pCurData->active)
	{
		if(send_log)
		{
			error_critical(eERROR_LOG,"Find GPS during %d secs.", count);
			devel_webdm_send_log("Find GPS during %d secs.", count);
			send_log = 0;
		}
		count = 0;
		return;
	}
	else
	{
		count++;
	}

	if(count >= CHECK_GPS_INVALID_SECS)
	{
		if(!send_log)
		{
			error_critical(eERROR_LOG,"Can't find GPS signal during %d secs.", count);
			devel_webdm_send_log("Can't find GPS signal during %d secs.", count);
			send_log = 1;
		}
	}
}

=======
#ifdef TEST_CODE_ENABLE
	#include "depend.h"
#else
	#include <base/sender.h>
	#include <base/devel.h>
	#include <board/power.h>
	#include <board/crit-data.h>
	#include <util/tools.h>
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <base/devel.h>
#include <base/error.h>

#include "config.h"
#include "lotte_gpsmng.h"
#include "lotte_gps_filter.h"
#include "lotte_gps_utill.h"
#include "lotte_geofence.h"
#include "lotte_packet.h"
#include "logd/logd_rpc.h"
#include "debug.h"

lotte_gps_manager_t lgm = {
	.gps_initial_count = GPS_INIT_MAX_WAITING_TIME,
	.first_gps_active = eINACTIVE,
	.server_mileage = 0,
	.gps_mileage = 0,
	.fixed_gpsdata.active = eINACTIVE
};

int enable_gps_log = 0;

void init_gps_manager()
{
	lgm.gps_initial_count = GPS_INIT_MAX_WAITING_TIME;
	lgm.first_gps_active = eINACTIVE;
	lgm.fixed_gpsdata.active = eINACTIVE;
}

//*
lotte_gps_manager_t* load_gps_manager()
{
	return &lgm;
}
//*/


int iscollection_gps_time(gpsData_t *pData)
{
	int diff_time;
	if(lgm.first_gps_active == eINACTIVE)
		return 0;

	diff_time = difftime(pData->utc_sec, lgm.reported_gpsdata.utc_sec);
	if( diff_time >= get_collection_interval())
		return 1;

	return 0;
}


void _record_last_gps_data(gpsData_t *pData)
{
	memcpy(&lgm.last_gpsdata, pData, sizeof(gpsData_t));
}

void _record_reported_gps_data(gpsData_t *pData)
{
	memcpy(&lgm.reported_gpsdata, pData, sizeof(gpsData_t));
}

void _record_last_fixed_gps_data(gpsData_t *pData, int ign_on)
{
	static time_t last_mileage_time = 0;
	static int move_detect = 0;
	static float lat = 0;
	static float lon = 0;
	static unsigned int count = 0;

	if(enable_gps_log)
	LOGI(LOG_TARGET, "++Mileage:%d %d %d/%f %f/%d %d/%d %d",
		ign_on, lgm.fixed_gpsdata.active,count, lat, lon, move_detect, pData->speed,
		pData->utc_sec, last_mileage_time);
	unsigned int status_bit = 0;

	if(ign_on != POWER_IGNITION_OFF)
	{
		count++;
		status_bit += 1;
	}
	
	if(lgm.fixed_gpsdata.active) 
	{
		status_bit += 1<<1;
		if(lat == 0 || lon == 0)
		{
			lat = lgm.fixed_gpsdata.lat;
			lon = lgm.fixed_gpsdata.lon;
			status_bit += 1<<2;
		}
	
		if(ign_on != POWER_IGNITION_OFF)
		{
			status_bit += 1<<3;
			if(pData->speed != 0)
			{
				move_detect = 1;
				status_bit += 1<<4;
			}
		
			if(pData->utc_sec - last_mileage_time >= MILEAGE_INTERVAL || count >= MILEAGE_INTERVAL )
			{
				status_bit += 1<<5;
				count = 0;
				if(pData->utc_sec - last_mileage_time < 0)
				{
					devel_webdm_send_log("ERROR: abnormal gps utc sec! %d %d", pData->utc_sec, last_mileage_time);
				}
				
				last_mileage_time = pData->utc_sec;
				
				if(move_detect == 1)
				{
					double tmp_mileage = 0, tmp_dist = 0;
				
					status_bit += 1<<6;

					tmp_dist = get_distance_meter(lat, pData->lat, lon, pData->lon);
					if(tmp_dist < 0)
					{
						return;
					}
					
					tmp_mileage = lgm.gps_mileage + tmp_dist;
					
					if(isnan(tmp_mileage) != 0)
					{
						static int send_log = 0;
						if(send_log == 0)
						{
							devel_webdm_send_log("_record_last_fixed_gps_data : nan error");
							send_log = 1;
						}
						
						LOGE(LOG_TARGET, "mileage result is nan. Dist:%f Mileage:before-%f after-%f\n", tmp_dist, lgm.gps_mileage, tmp_mileage);
						
						return;
					}

					move_detect = 0;
					
					lgm.gps_mileage = tmp_mileage;
					crit_set_data_mileage_write(lgm.server_mileage + lgm.gps_mileage);
					lat = pData->lat;
					lon = pData->lon;

					LOGI(LOG_TARGET, "move detect! %f %f %f", lgm.gps_mileage, lat, lon);
				}
			}
		}
	}

	memcpy(&lgm.fixed_gpsdata, pData, sizeof(gpsData_t));
	if(enable_gps_log)
	LOGI(LOG_TARGET, "--Mileage:%d %d %d/%f %f/%d %d/%d %d/%01d%01d%01d%01d%01d%01d%01d",
		ign_on, lgm.fixed_gpsdata.active,count, lat, lon, move_detect, pData->speed,
		pData->utc_sec, last_mileage_time,
		CheckBit(status_bit, 0),  CheckBit(status_bit, 1), CheckBit(status_bit, 2), CheckBit(status_bit, 3),
		CheckBit(status_bit, 4), CheckBit(status_bit, 5), CheckBit(status_bit, 6));
}


#ifdef FEATURE_GEO_FENCE_SIMULATION
extern int geo_test_flag;
#endif

int check_geofence(gpsData_t *pData)
{
	char smsmsg[100] = {0,};
	
	fence_notification_t fence_chk;

#ifdef FEATURE_GEO_FENCE_SIMULATION
	if(geo_test_flag == 0) //geo in
	{
		pData->lat = 37.399693;
		pData->lon = 127.100937;
	}

	else if(geo_test_flag == 1) //geo out
	{
		pData->lat = 37.394865;
		pData->lon = 127.105379;
	}
#endif


	if(get_geo_fence0_enable()) {
		fence_chk = get_geofence0_notification(*pData);
		if(fence_chk == eFENCE_IN_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #0 entry notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM0_ENTRY_EVT , NULL, ePIPE_1);

			sprintf(smsmsg, "fence0 pos lat:%3.7f, lon:%3.7f entry\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
		else if(fence_chk == eFENCE_OUT_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #0 exit notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM0_EXIT_EVT  , NULL, ePIPE_1);

			sprintf(smsmsg, "fence0 pos lat:%3.7f, lon:%3.7f exit\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
	}

	if(get_geo_fence1_enable()) {
		fence_chk = get_geofence1_notification(*pData);
		if(fence_chk == eFENCE_IN_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #1 entry notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM1_ENTRY_EVT , NULL, ePIPE_1);

			sprintf(smsmsg, "fence1 pos lat:%3.7f, lon:%3.7f entry\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
		else if(fence_chk == eFENCE_OUT_NOTIFICATION) {
			LOGT(LOG_TARGET, "geo fence #1 exit notification!!!\n");
			sender_add_data_to_buffer(eGEO_FENCE_NUM1_EXIT_EVT  , NULL, ePIPE_1);

			sprintf(smsmsg, "fence1 pos lat:%3.7f, lon:%3.7f exit\n",  pData->lat, pData->lon);
			printf("%s> %s", __func__, smsmsg);
			devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
		}
	}
	return 0;
}

void recovery_gps_data(gpsData_t *pData)
{

#if defined(FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS) && defined(FEATURE_INVALID_GPS_COPY_ZERO)
	#error "FEATURE_INACTIVE_GPS_COPY_LAST_FIX_GPS, FEATURE_INVALID_GPS_COPY_ZERO both define error"
#endif

#ifdef FEATURE_INVALD_GPS_COPY_LAST_FIX_GPS	
	if(lgm.first_gps_active == eACTIVE) 
	{
		pData->lat       = lgm.fixed_gpsdata.lat;
		pData->lon       = lgm.fixed_gpsdata.lon;
		pData->speed     = lgm.fixed_gpsdata.speed;
		pData->angle     = lgm.fixed_gpsdata.angle;
		pData->satellite = lgm.fixed_gpsdata.satellite; //keep satellite status
		pData->hdop      = lgm.fixed_gpsdata.hdop; //keep HDOP status
	}
	else
	{
		pData->lat       = 0;
		pData->lon       = 0;
		pData->speed     = 0;
		pData->angle     = 0;
		pData->satellite = 0; //keep satellite status
		pData->hdop      = 0; //keep HDOP status
	}
#endif

#ifdef FEATURE_INVALID_GPS_COPY_ZERO	
	pData->lat       = 0;
	pData->lon       = 0;
	//pData->speed     = 0;
	//pData->angle     = 0;
	//pData->satellite = 0; //keep satellite status
	//pData->hdop      = 0; //keep HDOP status
#endif

}


time_t get_device_time()
{
	time_t t = time(NULL);
	return t;
}

int fake_gps_time(gpsData_t *pData)
{
#ifdef FEATURE_FAKE_TIME_INACTIVE_GPS
	time_t ct;
	struct tm *tm;

	ct = get_device_time();
	tm = localtime(&ct);

	pData->year = tm->tm_year + 1900;
	pData->mon = tm->tm_mon + 1;
	pData->day = tm->tm_mday;
	pData->hour = tm->tm_hour;
	pData->min = tm->tm_min;
	pData->sec = tm->tm_sec;
	pData->utc_sec = ct;
#endif

	return 0;
}

gps_condition_t inactive_gps_process(gpsData_t cur_gps, gpsData_t *pData)
{
	gps_condition_t ret = eUNKNOWN_GPS_DATA;

	check_gps_invalid(&cur_gps , power_get_ignition_status());

	memset(pData, 0x00, sizeof(gpsData_t));
	switch(lgm.first_gps_active) 
	{
		case eINACTIVE:
			if(lgm.gps_initial_count <= 0) {
				//fake_gps_time(pData);
				lgm.first_gps_active = eACTIVE;
				ret = eUSE_GPS_DATA;
			}
			else {
				lgm.gps_initial_count--;
				ret = eSKIP_NO_INITIAL_GPS; //skip gps data
			}
			break;
		case eACTIVE:
			//fake_gps_time(pData);
			ret = eUSE_GPS_DATA;
			recovery_gps_data(&cur_gps);
			break;
		default:
			ret = eUNKNOWN_GPS_DATA;
			break;
	}

	if(ret == eUSE_GPS_DATA) {
		_record_last_gps_data(&cur_gps);

		if(iscollection_gps_time(&cur_gps) == 0)
			ret = eSKIP_NO_COLLECTION_TIME;
		else
			_record_reported_gps_data(&cur_gps);

		memcpy(pData, &cur_gps, sizeof(gpsData_t));
	}

	return ret;
}

gps_condition_t gps_filter(gpsData_t cur_gps)
{
	gps_condition_t ret = eUSE_GPS_DATA;

	configurationModel_t *conf = NULL;
	conf = get_config_model();


	if(conf->model.dist_filter_enable)
	{
		if(distance_filter(cur_gps, lgm.last_gpsdata, conf->model.dist_filter_value))
			ret = eSKIP_DISTANCE_FILTER;
	}

	if(conf->model.sat_filter_enable)
	{
		if(trust_satlites_filter(cur_gps, conf->model.sat_filter_value))
			ret = eSKIP_TRUST_SATLITES_FILTER;
	}


	if(conf->model.hdop_filter_enable)
	{
		if(trust_HDOP_filter(cur_gps, conf->model.hdop_filter_value))
			ret = eSKIP_TRUST_HDOP_FILTER;
	}


	if(conf->model.speed_filter_enable) 
	{
		if(trust_speed_filter(cur_gps, conf->model.speed_filter_value))
			ret = eSKIP_TRUST_SPEED_FILTER;
	}

	return ret;
}

void fix_gps_pos_in_key_off(gpsData_t *cur_gps, int ign_on)
{
	if(lgm.fixed_gpsdata.active) 
	{
		if(ign_on == POWER_IGNITION_OFF)
		{
			cur_gps->lat       = lgm.fixed_gpsdata.lat;
			cur_gps->lon       = lgm.fixed_gpsdata.lon;
			cur_gps->speed     = 0;
			cur_gps->angle     = lgm.fixed_gpsdata.angle;
		}
	}

}

gps_condition_t active_gps_process(gpsData_t cur_gps, gpsData_t *pData)
{
	int ign_on;
	gps_condition_t ret = eUSE_GPS_DATA;
	memset(pData, 0x00, sizeof(gpsData_t));

#ifdef FEATURE_KEYOFF_FIX_LAST_POSITION
	ign_on = power_get_ignition_status();
#else
	ign_on = POWER_IGNITION_ON;
#endif

	check_gps_invalid(&cur_gps , ign_on);

	fix_gps_pos_in_key_off(&cur_gps, ign_on);

	ret = gps_filter(cur_gps);
	_record_last_gps_data(&cur_gps);
	
	if(enable_gps_log)
	LOGI(LOG_TARGET, "++active_gps_process:%d %d/%f %f %d",
		ign_on, ret, cur_gps.lat, cur_gps.lon, cur_gps.speed);
		
	if(ret == eUSE_GPS_DATA)
	{
		//valid gps record.
		lgm.first_gps_active = eACTIVE;
		_record_last_fixed_gps_data(&cur_gps, ign_on);

		//geo fence check
		check_geofence(&cur_gps);
	}

	if(iscollection_gps_time(&cur_gps) == 0)
		return eSKIP_NO_COLLECTION_TIME;

	if(ret != eUSE_GPS_DATA) //filtering gps
		recovery_gps_data(&cur_gps);

	_record_reported_gps_data(&cur_gps);
	memcpy(pData, &cur_gps, sizeof(gpsData_t));

	return ret;
}

	
int get_server_mileage()
{
	return lgm.server_mileage;
}

int get_gps_mileage()
{
	return lgm.gps_mileage;
}

void set_server_mileage(int mileage)
{
	lgm.server_mileage = mileage; 
}

void set_gps_mileage(int mileage)
{
	lgm.gps_mileage = mileage;
}

void check_gps_invalid(gpsData_t *pCurData , int ign)
{
	static int count = 0;
	static int send_log = 0;

	if(ign == 0)
	{
		count = 0;
		return;
	}

	if(pCurData->active)
	{
		if(send_log)
		{
			error_critical(eERROR_LOG,"Find GPS during %d secs.", count);
			devel_webdm_send_log("Find GPS during %d secs.", count);
			send_log = 0;
		}
		count = 0;
		return;
	}
	else
	{
		count++;
	}

	if(count >= CHECK_GPS_INVALID_SECS)
	{
		if(!send_log)
		{
			error_critical(eERROR_LOG,"Can't find GPS signal during %d secs.", count);
			devel_webdm_send_log("Can't find GPS signal during %d secs.", count);
			send_log = 1;
		}
	}
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
