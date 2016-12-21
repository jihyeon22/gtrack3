#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include <base/config.h>
#include <board/crit-data.h>
#include <util/storage.h>
#include <util/tools.h>
#include <include/defines.h>
#include <logd_rpc.h>
#include "mileage.h"

#include <netcom.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MILEAGE


static mileageData_t mileagedata = {0,};
static double mileage_buf = 0;

int mileage_process(const gpsData_t *temp_gpsdata)
{
	configurationBase_t * conf = get_config_base();
	static int detect_move = 0;
	static time_t last_mileage_time = 0;

	time_t current_time = 0;
	
	if(conf->mileage.enable == 0)
	{
		return 0;
	}

	if(temp_gpsdata->lat < 32.657876 || temp_gpsdata->lat > 39.283294 ||
	        temp_gpsdata->lon < 123.263168 || temp_gpsdata->lon > 132.074203)
	{
		return -1;
	}

	mileageData_t *md = &mileagedata;

	if(temp_gpsdata->speed != 0)
	{
		detect_move = 1;
	}

	if(md->lastlat < 32.657876 || md->lastlat > 39.283294 ||
	        md->lastlon < 123.263168 || md->lastlon > 132.074203)
	{
		md->lastyear = temp_gpsdata->year;
		md->lastmon = temp_gpsdata->mon;
		md->lastday = temp_gpsdata->day;
		md->lastlat = temp_gpsdata->lat;
		md->lastlon = temp_gpsdata->lon;
		return 0;
	}

	if(conf->mileage.daily_mileage == 1 &&
		(md->lastyear != temp_gpsdata->year ||
		md->lastmon != temp_gpsdata->mon ||
		md->lastday != temp_gpsdata->day))
	{
		md->lastyear = temp_gpsdata->year;
		md->lastmon = temp_gpsdata->mon;
		md->lastday = temp_gpsdata->day;
		md->lastlat = temp_gpsdata->lat;
		md->lastlon = temp_gpsdata->lon;
		md->distance_sum = 0;
		return 0;
	}
	
	if(detect_move == 0)
	{
		return 0;
	}

	current_time = tools_get_kerneltime();

	if(current_time - last_mileage_time >= MILEAGE_CHECK_INTERVAL_SECS)
	{	
		last_mileage_time = current_time;

		LOGT(LOG_TARGET, "ACC DATA(adjust) Lat:%f Long:%f Distance:%f\n", md->lastlat, md->lastlon, mileage_buf);

		if(md->lastlat >= 32.657876 && md->lastlat <= 39.283294 &&
		        md->lastlon >= 123.263168 && md->lastlon <= 132.074203)
		{
			if(temp_gpsdata->lat != md->lastlat || temp_gpsdata->lon != md->lastlon)
			{
				mileage_buf += get_distance_m(md->lastlat, temp_gpsdata->lat, md->lastlon, temp_gpsdata->lon);
				md->lastlat = temp_gpsdata->lat;
				md->lastlon = temp_gpsdata->lon;
				md->lastyear = temp_gpsdata->year;
				md->lastmon = temp_gpsdata->mon;
				md->lastday = temp_gpsdata->day;
				detect_move = 0;

				md->distance_sum = mileage_buf;
				crit_set_data_mileage_detail_write(md->distance_sum, md->lastyear, md->lastmon, md->lastday, md->lastlat, md->lastlon);
	
				LOGT(LOG_TARGET, "acc result %f\n", mileage_buf);
			}
		}
	}
	
	return 0;
}

unsigned int mileage_get_m(void)
{
	return mileage_buf;
}

void mileage_set_m(const unsigned int val)
{
	mileage_buf = val;
}

int mileage_write(void)
{
	configurationBase_t * conf = get_config_base();
	
	if(conf->mileage.enable == 0)
	{
		return 0;
	}

	mileagedata.distance_sum = mileage_buf;

	crit_set_data_mileage_detail_write(mileagedata.distance_sum, mileagedata.lastyear, mileagedata.lastmon, mileagedata.lastday, mileagedata.lastlat, mileagedata.lastlon);

	if(storage_save_file(MILEAGE_PATH, &mileagedata, sizeof(mileagedata)) < 0)
	{
		return -1;
	}
	
	return 0;
}

int mileage_read(void)
{
	configurationBase_t * conf = get_config_base();
	mileageData_t crit;
	
	if(conf->mileage.enable == 0)
	{
		return 0;
	}	

	if(storage_load_file(MILEAGE_PATH, &mileagedata, sizeof(mileagedata)) < 0 && crit_get_support_stat() == 0)
	{
		memset(&mileagedata, 0, sizeof(mileagedata));
		mileage_buf = 0;
		return -1;
	}

	crit_get_data_mileage_detail(&crit.distance_sum, &crit.lastyear, &crit.lastmon, &crit.lastday, &crit.lastlat, &crit.lastlon);

	if(mileagedata.distance_sum >= crit.distance_sum)
	{
		mileage_buf = mileagedata.distance_sum;
	}
	else
	{
		mileage_buf = crit.distance_sum;
		mileagedata.distance_sum = crit.distance_sum;
		mileagedata.lastyear = crit.lastyear;
		mileagedata.lastmon = crit.lastmon;
		mileagedata.lastday = crit.lastday;
		mileagedata.lastlat = crit.lastlat;
		mileagedata.lastlon = crit.lastlon;
	}

	if(tools_get_kerneltime() < 60)
	{
		mileagedata.lastlat = 0;
		mileagedata.lastlon = 0;
	}
	
	LOGT(LOG_TARGET, "read mileage [%d m]\n", mileage_get_m());
	
	return 0;
}

double get_distance_km(double tlat1, double tlat2, double tlon1, double tlon2)
{
	if(tlat1 == tlat2 && tlon1 == tlon2)
	{
		return 0;
	}	

	double distance = ((acos(sin(tlat1 * M_PI / 180) * sin(tlat2 * M_PI / 180) + cos(tlat1 * M_PI / 180) * cos(tlat2 * M_PI / 180) * cos((tlon1 - tlon2) * M_PI / 180)) * 180 / M_PI) * 60 * 1.1515 * 1.609344);
	
	if(isnan(distance) != 0)
	{
		return -1.1;
	}
	
	return distance;
}

double get_distance_m(double tlat1, double tlat2, double tlon1, double tlon2)
{
	if(tlat1 == tlat2 && tlon1 == tlon2)
	{
		return 0;
	}

	double distance = ((acos(sin(tlat1 * M_PI / 180) * sin(tlat2 * M_PI / 180) + cos(tlat1 * M_PI / 180) * cos(tlat2 * M_PI / 180) * cos((tlon1 - tlon2) * M_PI / 180)) * 180 / M_PI) * 60 * 1.1515 * 1.609344);

	distance = distance * 1000.0;
	
	if(isnan(distance) != 0)
	{
		return -1.1;
	}	

	return distance;
}

