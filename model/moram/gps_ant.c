#include <time.h>

#include <base/gpstool.h>
#include <base/sender.h>
#include <at/at_util.h>
#include <util/tools.h>
#include "logd/logd_rpc.h"

#include <lotte_packet.h>
#include "gps_ant.h"
#include "debug.h"

#include <mdsapi/mds_api.h>

int _gps_ant_is_timing_check(void)
{
	static time_t prev_time_sense = 0;
	time_t cur_time;

	cur_time =  tools_get_kerneltime();
	if(cur_time <= 0)
	{
		return -1;
	}

	if(prev_time_sense == 0)
	{
		prev_time_sense = cur_time;
		
		return 1;
	}

	if(cur_time - prev_time_sense >= GPS_ANT_TIME_SENSE)
	{
		prev_time_sense = cur_time;

		return 1;
	}

	return 0;
}

int gps_ant_check(void)
{
	static int is_gps_disconnect = -1;
	char ant[16] = {0};
	gpsData_t gpsdata;
	int res_ant = 0;

	int gps_ant_stat = 0;

	if(_gps_ant_is_timing_check() <= 0)
	{
		return 0;
	}

	gps_get_curr_data(&gpsdata);
	if(is_gps_disconnect <= 0 && gpsdata.active == 1)
	{
		return 0;
	}

	if ( mds_api_gps_util_get_gps_ant() == DEFINES_MDS_API_OK)
		gps_ant_stat = 1;
	else
		gps_ant_stat = 0;


	LOGT(LOG_TARGET, " %s() gps ant stat : %d\n", __FUNCTION__, gps_ant_stat);
	
	if(gps_ant_stat == 1)
	{
		if(is_gps_disconnect == 1)
		{
			sender_add_data_to_buffer(eGPS_ANT_CONNECT, NULL, ePIPE_1);
		}
		is_gps_disconnect = 0;
		
		return 0;
	}

	if(is_gps_disconnect <= 0)
	{
		sender_add_data_to_buffer(eGPS_ANT_DISCONNECT, NULL, ePIPE_1);
		is_gps_disconnect = 1;
	}

	return 0;
}

