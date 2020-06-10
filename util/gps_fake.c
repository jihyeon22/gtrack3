<<<<<<< HEAD
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <float.h>

#include <base/config.h>
#include <base/error.h>
#include <base/devel.h>
#include <board/modem-time.h>
#include <board/crit-data.h>
#include <util/storage.h>
#include <util/tools.h>
#include <util/list.h>
#include <include/defines.h>
#include <config.h>

#include <logd_rpc.h>

#include <netcom.h>

#include <board/board_system.h>
#include <board/power.h>
#include <mdsapi/mds_api.h>

#include <base/gpstool.h>
#include <util/gps_fake.h>

static gpsData_t g_cur_gps_data;

static gpsData_t last_valid_gpsdata;
static gpsData_t saved_valid_gpsdata;

pthread_mutex_t gps_fake_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOG_TARGET eSVC_BASE

#define GPS_FAKE_MGR_GPSDATA

#ifdef GPS_FAKE_MGR_GPSDATA
#endif

void gps_fake_mutex_lock()
{
    pthread_mutex_lock(&gps_fake_mutex);
}

void gps_fake_mutex_unlock()
{
    pthread_mutex_unlock(&gps_fake_mutex);
}

void gps_valid_data_get(gpsData_t *last)
{
    gps_fake_mutex_lock();
#ifdef GPS_FAKE_MGR_GPSDATA
    memcpy(last, &last_valid_gpsdata, sizeof(gpsData_t));
#else
    memcpy(last, &g_cur_gps_data, sizeof(gpsData_t));
#endif
    gps_fake_mutex_unlock();
}

void gps_get_curr_data(gpsData_t* out)
{
    struct tm loc_time;
    gpsData_t tmp_gpsdata = {0,};

    //if (g_cur_gps_data.year < 2016)
    if (1) // force gps time set
    {
        // printf("[FAKE GPS] not valid gps time : set modem time\r\n");
        gps_fake_mutex_lock();
        memcpy(&tmp_gpsdata, &g_cur_gps_data, sizeof(gpsData_t));
        gps_fake_mutex_unlock();
        
        get_modem_time_tm(&loc_time);
        // gpsdata.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
        // _gps_utc_sec_localtime(gpsdata.utc_sec, &loc_time, conf->gps.gps_time_zone);
        tmp_gpsdata.year = loc_time.tm_year + 1900;
        tmp_gpsdata.mon = loc_time.tm_mon + 1;
        tmp_gpsdata.day = loc_time.tm_mday;
        tmp_gpsdata.hour = loc_time.tm_hour;
        tmp_gpsdata.min = loc_time.tm_min;
        tmp_gpsdata.sec = loc_time.tm_sec;

        tmp_gpsdata.utc_sec = get_modem_time_utc_sec();
        memcpy(out, &tmp_gpsdata, sizeof(gpsData_t));
    }
    else
    {
        gps_fake_mutex_lock();
        memcpy(out, &g_cur_gps_data, sizeof(gpsData_t));
        gps_fake_mutex_unlock();
    }

}

void gps_reset(int type)
{
    // fake : do nothing.
}

int gps_valid_data_write(void)
{
#ifdef GPS_FAKE_MGR_GPSDATA
	gpsData_t last;
	
	gps_valid_data_get(&last);

	if(last.active == 0)
	{
		return 0;
	}

	if(memcmp(&saved_valid_gpsdata, &last, sizeof(saved_valid_gpsdata)) == 0)
	{
		return 0;
	}

	crit_set_data_gps_write(last.lat, last.lon, last.utc_sec);
	
	if(storage_save_file(GPS_DATA_PATH, &last, sizeof(last)) < 0)
	{
		return -1;
	}
	memcpy(&saved_valid_gpsdata, &last, sizeof(saved_valid_gpsdata));
	
	//devel_webdm_send_log("gps_valid_data_write success");
#endif
	return 0;
}

void gps_valid_data_clear(void)
{
	gps_fake_mutex_lock();
	memset(&last_valid_gpsdata, 0, sizeof(last_valid_gpsdata));
	gps_fake_mutex_unlock();
}

int gps_valid_data_read(void)
{
#ifdef GPS_FAKE_MGR_GPSDATA
	gpsData_t last;
	float crit_lat = 0, crit_lon = 0;
	unsigned int crit_utc = 0;
	int res_load_file = 0;
	
    int i = 3;

    memset(&last, 0x00, sizeof(last));


    while(i--)
    {
	    res_load_file = storage_load_file(GPS_DATA_PATH, &last, sizeof(last));	
	    if(res_load_file < 0)
	    {
            printf("CRIT GPS DATA ++ [%d] \r\n", __LINE__);
            LOGE(LOG_TARGET, "WARN : CRIT GPS DATA return fail 1\n");
            printf("WARN : CRIT GPS DATA return fail 1\r\n");
		    gps_valid_data_clear();
            usleep(100);
            continue;
	    }
        break;
    }

	
	if(res_load_file < 0 && crit_get_support_stat() == 0)
	{		
        printf("CRIT GPS DATA ++ [%d] \r\n", __LINE__);
		LOGE(LOG_TARGET, "read valid gps fail\n");
		LOGE(LOG_TARGET, "WARN : CRIT GPS DATA return fail 3\n");
        printf("WARN : CRIT GPS DATA return fail 3\r\n");
		return -1;
	}

	crit_get_data_gps(&crit_lat, &crit_lon, &crit_utc);

    if(last.utc_sec == crit_utc)
    {
        devel_webdm_send_log("INFO : CRIT GPS DATA 0 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        LOGE(LOG_TARGET, "INFO : CRIT GPS DATA 0 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        printf( "INFO : CRIT GPS DATA 0 : last [%d] / crit [%d] / [%f, %f] .\r\n", last.utc_sec, crit_utc, last.lat,last.lon);
    }

	if(last.utc_sec < crit_utc)
	{	
		last.lat = crit_lat;
		last.lon = crit_lon;
		last.utc_sec = crit_utc;
        devel_webdm_send_log("WARN : CRIT GPS DATA 1 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
		LOGE(LOG_TARGET, "WARN : CRIT GPS DATA 1 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        printf( "WARN : CRIT GPS DATA 1 : last [%d] / crit [%d] / [%f, %f] .\r\n", last.utc_sec, crit_utc, last.lat,last.lon);
	}

	//Only for debugging.
	if(crit_utc < last.utc_sec)
	{
        printf("CRIT GPS DATA ++ [%d] \r\n", __LINE__);
		LOGE(LOG_TARGET, "WARN : CRIT GPS DATA 2 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
		devel_webdm_send_log("WARN : CRIT GPS DATA 2 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        printf( "WARN : CRIT GPS DATA 2 : last [%d] / crit [%d] / [%f, %f] .\r\n", last.utc_sec, crit_utc, last.lat,last.lon);
	}
	
    if ( ( last.lat != 0 ) && ( last.lon != 0 ))
    {
        gps_valid_data_set(&last);
	    memcpy(&saved_valid_gpsdata, &last, sizeof(saved_valid_gpsdata));
    }

	LOGT(LOG_TARGET, "read valid gps [%f, %f]\n", last_valid_gpsdata.lat, last_valid_gpsdata.lon);
#endif

	return 0;
}

void gps_valid_data_set(gpsData_t *last)
{
	gps_fake_mutex_lock();
	memcpy(&last_valid_gpsdata, last, sizeof(gpsData_t));
	gps_fake_mutex_unlock();
}


void gps_set_curr_data(gpsData_t* in)
{
    gps_fake_mutex_lock();
    memcpy(&g_cur_gps_data, in, sizeof(gpsData_t));
    gps_fake_mutex_unlock();
    
#ifdef GPS_FAKE_MGR_GPSDATA
    // last valid set
    if (in->active == 1)
    {
        gps_valid_data_set(in);
    }
#endif

    
}
=======
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <float.h>

#include <base/config.h>
#include <base/error.h>
#include <base/devel.h>
#include <board/modem-time.h>
#include <board/crit-data.h>
#include <util/storage.h>
#include <util/tools.h>
#include <util/list.h>
#include <include/defines.h>
#include <config.h>

#include <logd_rpc.h>

#include <netcom.h>

#include <board/board_system.h>
#include <board/power.h>
#include <mdsapi/mds_api.h>

#include <base/gpstool.h>
#include <util/gps_fake.h>

static gpsData_t g_cur_gps_data;

static gpsData_t last_valid_gpsdata;
static gpsData_t saved_valid_gpsdata;

pthread_mutex_t gps_fake_mutex = PTHREAD_MUTEX_INITIALIZER;

#define LOG_TARGET eSVC_BASE

#define GPS_FAKE_MGR_GPSDATA

#ifdef GPS_FAKE_MGR_GPSDATA
#endif

void gps_fake_mutex_lock()
{
    pthread_mutex_lock(&gps_fake_mutex);
}

void gps_fake_mutex_unlock()
{
    pthread_mutex_unlock(&gps_fake_mutex);
}

void gps_valid_data_get(gpsData_t *last)
{
    gps_fake_mutex_lock();
#ifdef GPS_FAKE_MGR_GPSDATA
    memcpy(last, &last_valid_gpsdata, sizeof(gpsData_t));
#else
    memcpy(last, &g_cur_gps_data, sizeof(gpsData_t));
#endif
    gps_fake_mutex_unlock();
}

void gps_get_curr_data(gpsData_t* out)
{
    struct tm loc_time;
    gpsData_t tmp_gpsdata = {0,};

    //if (g_cur_gps_data.year < 2016)
    if (1) // force gps time set
    {
        // printf("[FAKE GPS] not valid gps time : set modem time\r\n");
        gps_fake_mutex_lock();
        memcpy(&tmp_gpsdata, &g_cur_gps_data, sizeof(gpsData_t));
        gps_fake_mutex_unlock();
        
        get_modem_time_tm(&loc_time);
        // gpsdata.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
        // _gps_utc_sec_localtime(gpsdata.utc_sec, &loc_time, conf->gps.gps_time_zone);
        tmp_gpsdata.year = loc_time.tm_year + 1900;
        tmp_gpsdata.mon = loc_time.tm_mon + 1;
        tmp_gpsdata.day = loc_time.tm_mday;
        tmp_gpsdata.hour = loc_time.tm_hour;
        tmp_gpsdata.min = loc_time.tm_min;
        tmp_gpsdata.sec = loc_time.tm_sec;

        tmp_gpsdata.utc_sec = get_modem_time_utc_sec();
        memcpy(out, &tmp_gpsdata, sizeof(gpsData_t));
    }
    else
    {
        gps_fake_mutex_lock();
        memcpy(out, &g_cur_gps_data, sizeof(gpsData_t));
        gps_fake_mutex_unlock();
    }

}

void gps_reset(int type)
{
    // fake : do nothing.
}

int gps_valid_data_write(void)
{
#ifdef GPS_FAKE_MGR_GPSDATA
	gpsData_t last;
	
	gps_valid_data_get(&last);

	if(last.active == 0)
	{
		return 0;
	}

	if(memcmp(&saved_valid_gpsdata, &last, sizeof(saved_valid_gpsdata)) == 0)
	{
		return 0;
	}

	crit_set_data_gps_write(last.lat, last.lon, last.utc_sec);
	
	if(storage_save_file(GPS_DATA_PATH, &last, sizeof(last)) < 0)
	{
		return -1;
	}
	memcpy(&saved_valid_gpsdata, &last, sizeof(saved_valid_gpsdata));
	
	//devel_webdm_send_log("gps_valid_data_write success");
#endif
	return 0;
}

void gps_valid_data_clear(void)
{
	gps_fake_mutex_lock();
	memset(&last_valid_gpsdata, 0, sizeof(last_valid_gpsdata));
	gps_fake_mutex_unlock();
}

int gps_valid_data_read(void)
{
#ifdef GPS_FAKE_MGR_GPSDATA
	gpsData_t last;
	float crit_lat = 0, crit_lon = 0;
	unsigned int crit_utc = 0;
	int res_load_file = 0;
	
    int i = 3;

    memset(&last, 0x00, sizeof(last));


    while(i--)
    {
	    res_load_file = storage_load_file(GPS_DATA_PATH, &last, sizeof(last));	
	    if(res_load_file < 0)
	    {
            printf("CRIT GPS DATA ++ [%d] \r\n", __LINE__);
            LOGE(LOG_TARGET, "WARN : CRIT GPS DATA return fail 1\n");
            printf("WARN : CRIT GPS DATA return fail 1\r\n");
		    gps_valid_data_clear();
            usleep(100);
            continue;
	    }
        break;
    }

	
	if(res_load_file < 0 && crit_get_support_stat() == 0)
	{		
        printf("CRIT GPS DATA ++ [%d] \r\n", __LINE__);
		LOGE(LOG_TARGET, "read valid gps fail\n");
		LOGE(LOG_TARGET, "WARN : CRIT GPS DATA return fail 3\n");
        printf("WARN : CRIT GPS DATA return fail 3\r\n");
		return -1;
	}

	crit_get_data_gps(&crit_lat, &crit_lon, &crit_utc);

    if(last.utc_sec == crit_utc)
    {
        devel_webdm_send_log("INFO : CRIT GPS DATA 0 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        LOGE(LOG_TARGET, "INFO : CRIT GPS DATA 0 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        printf( "INFO : CRIT GPS DATA 0 : last [%d] / crit [%d] / [%f, %f] .\r\n", last.utc_sec, crit_utc, last.lat,last.lon);
    }

	if(last.utc_sec < crit_utc)
	{	
		last.lat = crit_lat;
		last.lon = crit_lon;
		last.utc_sec = crit_utc;
        devel_webdm_send_log("WARN : CRIT GPS DATA 1 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
		LOGE(LOG_TARGET, "WARN : CRIT GPS DATA 1 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        printf( "WARN : CRIT GPS DATA 1 : last [%d] / crit [%d] / [%f, %f] .\r\n", last.utc_sec, crit_utc, last.lat,last.lon);
	}

	//Only for debugging.
	if(crit_utc < last.utc_sec)
	{
        printf("CRIT GPS DATA ++ [%d] \r\n", __LINE__);
		LOGE(LOG_TARGET, "WARN : CRIT GPS DATA 2 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
		devel_webdm_send_log("WARN : CRIT GPS DATA 2 : last [%d] / crit [%d] / [%f, %f] .\n", last.utc_sec, crit_utc, last.lat,last.lon);
        printf( "WARN : CRIT GPS DATA 2 : last [%d] / crit [%d] / [%f, %f] .\r\n", last.utc_sec, crit_utc, last.lat,last.lon);
	}
	
    if ( ( last.lat != 0 ) && ( last.lon != 0 ))
    {
        gps_valid_data_set(&last);
	    memcpy(&saved_valid_gpsdata, &last, sizeof(saved_valid_gpsdata));
    }

	LOGT(LOG_TARGET, "read valid gps [%f, %f]\n", last_valid_gpsdata.lat, last_valid_gpsdata.lon);
#endif

	return 0;
}

void gps_valid_data_set(gpsData_t *last)
{
	gps_fake_mutex_lock();
	memcpy(&last_valid_gpsdata, last, sizeof(gpsData_t));
	gps_fake_mutex_unlock();
}


void gps_set_curr_data(gpsData_t* in)
{
    gps_fake_mutex_lock();
    memcpy(&g_cur_gps_data, in, sizeof(gpsData_t));
    gps_fake_mutex_unlock();
    
#ifdef GPS_FAKE_MGR_GPSDATA
    // last valid set
    if (in->active == 1)
    {
        gps_valid_data_set(in);
    }
#endif

    
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
