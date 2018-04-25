#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <float.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/mileage.h>
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

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_GPS

#define GPS_DBG_MSG_SKIP_COUNT	3

static int _check_gps_data(gpsData_t *gpsdata);

#ifdef MDS_FEATURE_USE_NMEA_PORT
int gps_fd = GPS_INVALID_HANDLE;
char gps_dev_file[32] = GPS_DEV_FILE;
#endif // MDS_FEATURE_USE_NMEA_PORT


int gps_dbg_msg_count = 0;
int g_skip_gps_when_error = 0;
float emul_gps_lat = 0;
float emul_gps_lon = 0;
int emul_gps_speed = 0;

static gpsData_t cur_gps_data;
static gpsData_t prev_gps_data;
static gpsData_t last_valid_gpsdata;
static gpsData_t saved_valid_gpsdata;
static int is_set_last_gpsdata = 0;

static int flag_gps_fixed = 0;
static int flag_modem_utc_setted = 0;
static int flag_enabled_modem_time_driver = 0;
static int flag_gps_pipe_emul = 0;

pthread_mutex_t gps_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gps_prev_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gps_last_mutex = PTHREAD_MUTEX_INITIALIZER;

// ----------------------------------------------------------
// gps turn on / turn off Function
// ----------------------------------------------------------
int gps_addr_agps()
{
	// no support 
	return 0;
}

void gps_on(int type) 
{
	
	if ( type == GPS_BOOT_WARM )
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : WARM START");
		mds_api_gpsd_start(MDS_API_GPS_TOOLS__TYPE_WARM);
	}
	else if ( type == GPS_BOOT_COLD )
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : COLD START");
		mds_api_gpsd_start(MDS_API_GPS_TOOLS__TYPE_COLD);
	}
	else
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : WRONG START TYPE");
	}
}

#define GPS_RESET_REQ_CNT	3
void gps_reset(int type)
{
	static int gps_reset_cnt = 0;

	gps_reset_cnt ++ ;

	if ( gps_reset_cnt < 3 )
	{
		devel_webdm_send_log("GPS RESET SKIP..  [%d]/[%d]",gps_reset_cnt, GPS_RESET_REQ_CNT);
		
		return ;
	}

	if ( type == GPS_BOOT_WARM)
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : WARM RESET [%d]",gps_reset_cnt);
		mds_api_gpsd_reset(MDS_API_GPS_TOOLS__TYPE_WARM);
	}
	else if ( type == GPS_BOOT_COLD)
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : COLD RESET [%d]",gps_reset_cnt);
		mds_api_gpsd_reset(MDS_API_GPS_TOOLS__TYPE_COLD);
	}
	else
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : WRONG RESET TYPE [%d]",gps_reset_cnt);
	}
	gps_reset_cnt = 0;
	// shutdown 되는데 오래걸리니 이정도는 쉬어주도록하자.
	// sleep(10);
}

void gps_reset_immediately(int type)
{
	static int gps_reset_cnt = 0;

	gps_reset_cnt ++ ;

	if ( type == GPS_BOOT_WARM)
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : WARM RESET 2 [%d]",gps_reset_cnt);
		mds_api_gpsd_reset(MDS_API_GPS_TOOLS__TYPE_WARM);
	}
	else if ( type == GPS_BOOT_COLD)
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : COLD RESET 2 [%d]",gps_reset_cnt);
		mds_api_gpsd_reset(MDS_API_GPS_TOOLS__TYPE_COLD);
	}
	else
	{
		devel_webdm_send_log("GPSD UDP SEND CMD : WRONG RESET 2 TYPE [%d]",gps_reset_cnt);
	}
	// gps_reset_cnt = 0;
	// shutdown 되는데 오래걸리니 이정도는 쉬어주도록하자.
	// sleep(10);
}

void gps_off() {
	devel_webdm_send_log("GPSD UDP SEND CMD : DOWN");
	mds_api_gpsd_stop();
}

#ifdef MDS_FEATURE_USE_NMEA_PORT
void gps_clear(int fd) {
	char buf[100] = {0};
	int stime = tools_get_kerneltime();
	int n_try = 50;
	struct timeval tout;

	if(fd < 0)
	{
		return;
	}

	while(1)
	{
		fd_set fds;
		
		FD_ZERO(&fds);
		FD_SET(fd, &fds);

	       tout.tv_sec = 0;
		tout.tv_usec = 300000;

		if(n_try-- <= 0 || tools_get_kerneltime() - stime >= 2)
		{
			return;
		}

		if (select(fd+1, &fds, NULL, NULL, &tout) > 0)
		{
			if(FD_ISSET(fd, &fds))
				read(fd, buf, sizeof(buf));
		}
	}
}
#endif

int gps_set_pipe_for_emul(void)
{
	if(tools_check_exist_file(GPS_PIPE_PATH, 10) < 0)
	{
		return -1;
	}

#ifdef MDS_FEATURE_USE_NMEA_PORT
	flag_gps_pipe_emul = 1;
	snprintf(gps_dev_file, sizeof(gps_dev_file)-1, "%s", GPS_PIPE_PATH);
#endif
	return 0;
}

// ----------------------------------------------------------
// gps Parser
// ----------------------------------------------------------

int tok_next(char ** arg, char * buf, int len)
{
	int idx = 0;
	const char * ptr = *arg;

	while(*ptr)
	{
		if(*ptr == ',' || *ptr == '*' || *ptr == '\r' || *ptr == '\n') {
			break;
		}

		if(idx + 1 == len) {
			return 0;    // not enough buffer
		}

		buf[idx++] = *ptr++;
	}

	buf[idx++] = '\0';

	return idx;
}

int _gps_parse_time(char* buf, int* ret_hour, int* ret_min, int* ret_sec)
{
	*ret_hour = 0, *ret_min = 0, *ret_sec = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 6) {
		return GPS_PARSE_FALSE;
	}

	*ret_hour = ((buf[0] - '0') * 10) + (buf[1] - '0');
	*ret_min = ((buf[2] - '0') * 10) + (buf[3] - '0');
	*ret_sec = ((buf[4] - '0') * 10) + (buf[5] - '0');

	return GPS_PARSE_TRUE;
}

int _gps_parse_valid(char * buf, int* ret)
{
	*ret = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 1) {
		return GPS_PARSE_FALSE;
	}

	if(buf[0] == 'A') {
		*ret = 1;
	}

	return GPS_PARSE_TRUE;
}


int _gps_parse_date(char* buf, int* ret_year, int* ret_mon, int* ret_day)
{
	*ret_year = 0, *ret_mon = 0, *ret_day = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 6) {
		return GPS_PARSE_FALSE;
	}

	*ret_year = 2000 + ((buf[4] - '0') * 10) + (buf[5] - '0');
	*ret_mon = ((buf[2] - '0') * 10) + (buf[3] - '0');
	*ret_day = ((buf[0] - '0') * 10) + (buf[1] - '0');

	return GPS_PARSE_TRUE;
}

float _gps_parse_angle(char* buf, float *angle)
{
	*angle = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 3) {
		return GPS_PARSE_FALSE;
	}

	*angle = atof(buf);
	return GPS_PARSE_TRUE;
}

float _gps_parse_dop(char* buf, float *dop)
{
	*dop = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 1) {
		return GPS_PARSE_FALSE;
	}

	*dop = atof(buf);
	return GPS_PARSE_TRUE;
}

float _gps_parse_altitude(char* buf, float *altitude)
{
	*altitude = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 1) {
		return GPS_PARSE_FALSE;
	}

	*altitude = atof(buf);
	return GPS_PARSE_TRUE;
}


int _gps_parse_latitude(char * buf, float * pf)
{
	*pf = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 9) {
		return GPS_PARSE_FALSE;
	}

	*pf = ((buf[0] - '0') * 10) + (buf[1] - '0') + (atof(buf + 2) / 60.0f);

	return GPS_PARSE_TRUE;
}

int _gps_parse_indicator_south(char * buf, int* ret)
{
	*ret = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 1) {
		return GPS_PARSE_FALSE;
	}

	if(buf[0] == 'S') {
		*ret = 1;
	}

	return GPS_PARSE_TRUE;
}

int _gps_parse_indicator_west(char * buf, int* ret)
{
	*ret = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 1) {
		return GPS_PARSE_FALSE;
	}

	if(buf[0] == 'W') {
		*ret = 1;
	}

	return GPS_PARSE_TRUE;
}

int _gps_parse_longitude(char * buf, float * pf)
{
	*pf = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 10) {
		return GPS_PARSE_FALSE;
	}

	*pf = ((buf[0] - '0') * 100) + ((buf[1] - '0') * 10) + (buf[2] - '0') + (atof(buf + 3) / 60.0f);

	return GPS_PARSE_TRUE;

}

int _gps_parse_speed(char * buf, int* ret)
{
	float temp_speed;
	*ret = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 3) {
		return GPS_PARSE_FALSE;
	}
	temp_speed = atof(buf);
	temp_speed *= 1.852; //convert knot to km/s
	*ret = (int)temp_speed;
	return GPS_PARSE_TRUE;
}

int _gps_parse_satellites(char * buf, int* ret)
{
	*ret = 0;
	if(buf == NULL) {
		return GPS_PARSE_FALSE;
	}

	if(strlen(buf) < 1) {
		return GPS_PARSE_FALSE;
	}
	else if(strlen(buf) == 1) {
		*ret = buf[0] - '0';
	}
	else if(strlen(buf) == 2)
	{
		*ret = (buf[0] - '0') * 10 + buf[1] - '0';
	}
	else
	{
		return GPS_PARSE_FALSE;
	}
	return GPS_PARSE_TRUE;
}

// Our linux image didn't set timezone. so using localtime or gmtime is meaningless.
int _gps_utc_localtime(struct tm *utc, struct tm *gmt, int adj_hour)
{
	time_t utc_sec = 0;
	
	if(utc == NULL || gmt == NULL)
	{
		return GPS_PARSE_FALSE;
	}

	utc->tm_isdst = 0;
	utc_sec = mktime(utc)  - timezone;
	utc_sec += adj_hour * 60 * 60;
	gmtime_r(&utc_sec, gmt);
	
	return GPS_PARSE_TRUE;
}

int _gps_utc_sec_localtime(time_t utc_sec, struct tm *gmt, int adj_hour)
{
	if(gmt == NULL)
	{
		return GPS_PARSE_FALSE;
	}

	utc_sec += adj_hour * 60 * 60;
	gmtime_r(&utc_sec, gmt);
	
	return GPS_PARSE_TRUE;
}

void gps_parse(char* buff, int size)
{
	int j;
	int buff_p = 0;
	int tok_read_cnt;

	int utc_year = 0, utc_mon = 0, utc_day = 0, utc_hour = 0, utc_min = 0, utc_sec = 0;
	float cur_lat = 0, cur_long = 0;
	int cur_south = 0, cur_west = 0;
	int cur_satellite = 0, cur_active = 0;
	int cur_speed = 0;
	float cur_angle = 0;
	float cur_hdop = 0;
	float cur_altitude = 0;

	char tempbuf[32] = {};
	gpsData_t tmp_gps_data;

	static time_t prev_gps_time = 0;
	static int count_utc_refresh = 0;

	static int gps_deactive_time = 0;
    static int send_gps_active_log = 0;

	int gps_parse_max_try_cnt = 180;

	if(g_skip_gps_when_error > 0)
	{
		return;
	}

	// size check..
	if((size == 0) | (size > 2048)) {
		return;
	}

	while(buff_p < size)
	{
		if ( gps_parse_max_try_cnt-- <= 0 )
		{
			LOGE(LOG_TARGET, "GPS PARSER is too many try. break loop... \n");
			break;
		}
		
		tok_read_cnt = tok_next(&buff, tempbuf, sizeof(tempbuf));

		buff += tok_read_cnt;	// move buff pointer...
		buff_p += tok_read_cnt;  // check total read counter..

		// -------------------------------------------------------------------------
		// GPGGA Data Parsing
		// -------------------------------------------------------------------------
		if((strstr(tempbuf, "$GPGGA") != 0))
		{

			for(j = 0; j < 15; j++)
			{
				switch(j)
				{
					case 0:
						break;
					case 1:	// time
					{
						int hour, min, sec;

						if(_gps_parse_time(tempbuf, &hour, &min, &sec) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - Time : [%d:%d:%d]\r\n", hour, min, sec);
						}
						else
						{
							//printf("gps get Data Fail - Time \r\n");
						}
						break;
					}
					case 2:  // latitude..
					{
						float latitude;
						if(_gps_parse_latitude(tempbuf, &latitude) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - lat : [%f]\r\n",latitude);
						}
						else
						{
							//printf("gps get Data Fail - lat\r\n");
						}
						break;
					}
					case  3 : // N/S Indicator, 'N' or 'S'
					{
						int indicator_south;
						if(_gps_parse_indicator_south(tempbuf, &indicator_south) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - N/S indicator : ");
							if(indicator_south == 1)
							{
								//printf("South\r\n");
								// if south than.. latitude is minus value...
								//   ==> -latitude;
							}
							else
							{
								//printf("North\r\n");
								// if North than.. latitude is plus value...
								//   ==> +latitude;
							}
						}
						else
						{
							//printf("gps get Data Fail - N/S indicator \r\n");
						}
						break;
					}
					case  4 : // Logitude, dddmm.mmmm
					{
						float longitude;
						if(_gps_parse_longitude(tempbuf, &longitude) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - Longitude : [%f]\r\n",longitude);
						}
						else
						{
							//printf("gps get Data Fail - Longitude\r\n");
						}
						break;
					}
					case  5 : // E/W Indicator, 'E' or 'W'
					{
						int indicator_west;
						if(_gps_parse_indicator_west(tempbuf, &indicator_west) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - E/W indicator : ");
							if(indicator_west == 1)
							{
								//printf("west\r\n");
								// if west than.. Logitude is minus value...
								//   ==> -Logitude;
							}
							else
							{
								//printf("est\r\n");
								// if est than.. Logitude is plus value...
								//   ==> +Logitude;
							}
						}
						else
						{
							//printf("gps get Data Fail - E/W indicator\r\n");
						}
						break;
					}
					case  6 : // Position Fix Indicator, 0 : Fix not available, 1 : GPS fix, 2 : Differential GPS fix
						break;
					case  7 : // Satellites Used, Range 0 to 14
					{
						int satellites;
						if(_gps_parse_satellites(tempbuf, &satellites) == GPS_PARSE_TRUE)
						{
							cur_satellite = satellites;
						}
						else
						{
							//printf("gps get Data Fail - Satellites\r\n");
						}
						break;
					}
					case  8 : // HDOP(Horizontal Dilution of Precision)
					{
						float hdop;
						if(_gps_parse_dop(tempbuf, &hdop) == GPS_PARSE_TRUE)
						{
							cur_hdop = hdop;
						}
						else
						{
							cur_hdop = FLT_MAX;
							//printf("gps get Data Fail - Satellites\r\n");
						}
						break;
					}
					case  9 : // MSL Altitude, meters
					{
						float altitude;
						if(_gps_parse_altitude(tempbuf, &altitude) == GPS_PARSE_TRUE)
						{
							cur_altitude = altitude;
						}
						break;
					}
					case 10 : // Units, 'M', meters
						break;
					case 11 : // Geoidal Separation, meters
						break;
					case 12 : // Units, 'M', meters
						break;
					case 13 : // Age of Diff. Coff.
						break;
					case 14 : //
						break;
				}
				tok_read_cnt = tok_next(&buff, tempbuf, sizeof(tempbuf));

				buff += tok_read_cnt;	// move buff pointer...
				buff_p += tok_read_cnt;  // check total read counter..

			}
		}

		// -------------------------------------------------------------------------
		// GPRMC Data Parsing
		// -------------------------------------------------------------------------
		if((strstr(tempbuf, "$GPRMC") != 0))
		{
			for(j = 0; j < 13; j++)
			{
				switch(j)
				{
					case 0:
						break;
					case 1:	// time
					{
						int hour, min, sec;
						if(_gps_parse_time(tempbuf, &hour, &min, &sec) == GPS_PARSE_TRUE)
						{
							utc_hour = hour;
							utc_min = min;
							utc_sec = sec;
							//printf("gps get Data Success - Time : [%d:%d:%d]\r\n", hour, min, sec);
						}
						else
						{
							//printf("gps get Data Fail - Time \r\n");
						}
						break;
					}
					case 2:
					{
						int isvalid;
						if(_gps_parse_valid(tempbuf, &isvalid) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - active : ");
							if(isvalid == 1)
							{
								cur_active = 1;
								//printf("Active\r\n");
							}
							else
							{
								cur_active = 0;
								//printf("Invalid\r\n");
							}
						}
						break;
					}
					case  3 : // latitude..
					{
						float latitude;
						if(_gps_parse_latitude(tempbuf, &latitude) == GPS_PARSE_TRUE)
						{
							cur_lat = latitude;
							//printf("gps get Data Success - lat : [%f]\r\n",latitude);
						}
						else
						{
							//printf("gps get Data Fail - lat\r\n");
						}
						break;
					}
					case  4 : // N/S Indicator, 'N' or 'S'
					{
						int indicator_south;
						if(_gps_parse_indicator_south(tempbuf, &indicator_south) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - N/S indicator : ");
							if(indicator_south == 1)
							{
								//printf("South\r\n");
								cur_south = 1;
								// if south than.. latitude is minus value...
								//   ==> -latitude;
							}
							else
							{
								//printf("North\r\n");
								cur_south = 0;
								// if North than.. latitude is plus value...
								//   ==> +latitude;
							}
						}
						else
						{
							//printf("gps get Data Fail - N/S indicator \r\n");
						}
						break;
					}
					case  5 : // Logitude, dddmm.mmmm
					{
						float longitude;
						if(_gps_parse_longitude(tempbuf, &longitude) == GPS_PARSE_TRUE)
						{
							cur_long = longitude;
							//printf("gps get Data Success - Longitude : [%f]\r\n",longitude);
						}
						else
						{
							//printf("gps get Data Fail - Longitude\r\n");
						}
						break;
					}
					case  6 : // E/W Indicator, 'E' or 'W'
					{
						int indicator_west;
						if(_gps_parse_indicator_west(tempbuf, &indicator_west) == GPS_PARSE_TRUE)
						{
							//printf("gps get Data Success - E/W indicator : ");
							if(indicator_west == 1)
							{
								//printf("west\r\n");
								cur_west = 1;
								// if west than.. Logitude is minus value...
								//   ==> -Logitude;
							}
							else
							{
								cur_west = 0;
								//printf("est\r\n");
								// if est than.. Logitude is plus value...
								//   ==> +Logitude;
							}
						}
						else
						{
							//printf("gps get Data Fail - E/W indicator\r\n");
						}
						break;
					}
					case  7 :    // speed
					{
						int speed;
						if(_gps_parse_speed(tempbuf, &speed) == GPS_PARSE_TRUE)
						{
							cur_speed = speed;
							//printf("gps get Data Success - speed : ");
						}
						else
						{
							cur_speed = -1;
							//printf("gps get Data Fail - Speed\r\n");
						}
						break;
					}
					case  8 :    // bearing
					{
						float angle;
						if(_gps_parse_angle(tempbuf, &angle) == GPS_PARSE_TRUE)
						{
							cur_angle = angle;
							//printf("gps get Data Success - angle : %f", cur_angle);
						}
						break;
					}
					case  9 :   //UTC date
					{
						int year, mon, day;
						if(_gps_parse_date(tempbuf, &year, &mon, &day) == GPS_PARSE_TRUE)
						{
							utc_year = year;
							utc_mon = mon;
							utc_day = day;
						}
						else
						{
							//printf("gps get Data Fail - date\r\n");
						}
					}
					break;
					case 10 :
						break;
					case 11 :
						break;
					case 12 :
						break;

				}
				tok_read_cnt = tok_next(&buff, tempbuf, sizeof(tempbuf));

				buff += tok_read_cnt;	// move buff pointer...
				buff_p += tok_read_cnt;  // check total read counter..
			}
		}
	}

	memset((void *)&tmp_gps_data, 0, sizeof(gpsData_t));

	configurationBase_t *conf = get_config_base();

    // timve value init to systemtime
    {
        struct tm loc_time;
        tmp_gps_data.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
        _gps_utc_sec_localtime(tmp_gps_data.utc_sec, &loc_time, conf->gps.gps_time_zone);
        tmp_gps_data.year = loc_time.tm_year + 1900;
        tmp_gps_data.mon = loc_time.tm_mon + 1;
        tmp_gps_data.day = loc_time.tm_mday;
        tmp_gps_data.hour = loc_time.tm_hour;
        tmp_gps_data.min = loc_time.tm_min;
        tmp_gps_data.sec = loc_time.tm_sec;
    }

	if(cur_active)
	{
		struct tm utc_time, loc_time;

		utc_time.tm_year = utc_year - 1900;
		utc_time.tm_mon = utc_mon - 1;
		utc_time.tm_mday = utc_day;
		utc_time.tm_hour = utc_hour;
		utc_time.tm_min = utc_min;
		utc_time.tm_sec = utc_sec;
	
		utc_time.tm_isdst = 0;
		tmp_gps_data.utc_sec = mktime(&utc_time) - timezone;

		if(conf->gps.gps_time_zone)
		{
			_gps_utc_sec_localtime(tmp_gps_data.utc_sec, &loc_time, conf->gps.gps_time_zone);
			tmp_gps_data.year = loc_time.tm_year + 1900;
			tmp_gps_data.mon = loc_time.tm_mon + 1;
			tmp_gps_data.day = loc_time.tm_mday;
			tmp_gps_data.hour = loc_time.tm_hour;
			tmp_gps_data.min = loc_time.tm_min;
			tmp_gps_data.sec = loc_time.tm_sec;
		}
		else
		{
			tmp_gps_data.year = utc_year;
			tmp_gps_data.mon = utc_mon;
			tmp_gps_data.day = utc_day;
			tmp_gps_data.hour = utc_hour;
			tmp_gps_data.min = utc_min;
			tmp_gps_data.sec = utc_sec;

		}

		tmp_gps_data.satellite = cur_satellite;

		tmp_gps_data.active = cur_active;
		if(cur_speed == -1)
		{
			tmp_gps_data.speed = prev_gps_data.speed + emul_gps_speed;
		}
		else
		{
			tmp_gps_data.speed = cur_speed + emul_gps_speed;
		}

		if(cur_west)
		{
			cur_long *= -1;
		}
		if(cur_south)
		{
			cur_lat *= -1;
		}
		tmp_gps_data.lat = cur_lat + emul_gps_lat;
		tmp_gps_data.lon = cur_long + emul_gps_lon;
		tmp_gps_data.angle = cur_angle;
		
		tmp_gps_data.hdop = cur_hdop;
		tmp_gps_data.altitude = cur_altitude;

		count_utc_refresh = 0;
	}
	else if(flag_modem_utc_setted || flag_gps_fixed )
	{
		struct tm utc_time, loc_time;

		pthread_mutex_lock(&gps_prev_mutex);
		
		//adjust gps data
		if(flag_gps_fixed && conf->gps.adjust_gps == 1)
		{
			memcpy(&tmp_gps_data, &prev_gps_data, sizeof(tmp_gps_data));
			tmp_gps_data.active = 0;
			tmp_gps_data.speed = 0;
		}

		if(prev_gps_data.utc_sec == 0)
		{
			if(flag_enabled_modem_time_driver && flag_modem_utc_setted)
			{
				// tmp_gps_data.utc_sec = get_modem_time_utc_sec();
				tmp_gps_data.utc_sec = get_system_time_utc_sec(conf->gps.gps_time_zone);
				
				count_utc_refresh = 0;
			}
		}
		else
		{
			int is_set_modem_utc = 0;
			if(flag_enabled_modem_time_driver && flag_modem_utc_setted)
			{
				if(count_utc_refresh++ > TIME_REFRESH_UTC)
				{
					//time_t temp_mtime  = get_modem_time_utc_sec();
					time_t temp_mtime  = get_system_time_utc_sec(conf->gps.gps_time_zone);
					if(temp_mtime > prev_gps_data.utc_sec)
					{
						tmp_gps_data.utc_sec = temp_mtime;
						count_utc_refresh = 0;
						is_set_modem_utc = 1;
					}
					else
					{
						LOGT(LOG_TARGET, "Skip GPS : same UTC! prev:%u cur:%u\n", prev_gps_data.utc_sec, temp_mtime);
						pthread_mutex_unlock(&gps_prev_mutex);
						return;
					}

					LOGT(LOG_TARGET, "Refresh UTC %u\n", tmp_gps_data.utc_sec);
				}
			}

			if(is_set_modem_utc == 0)
			{
				/*
				int diff_time = tools_get_kerneltime() - prev_gps_time;
				if( diff_time >= 2 && prev_gps_time !=0)
				{
					tmp_gps_data.utc_sec = prev_gps_data.utc_sec + diff_time;
				}
				else
				{
					tmp_gps_data.utc_sec = ++prev_gps_data.utc_sec;
				}
				*/
				// 1초당 1개씩 들어오는것이 안맞는다. 때문에 강제 시간보정은 하지 않는다.
				tmp_gps_data.utc_sec  = get_system_time_utc_sec(conf->gps.gps_time_zone);
			}
		}
		
		pthread_mutex_unlock(&gps_prev_mutex);
		
		if(conf->gps.gps_time_zone)
		{
			_gps_utc_sec_localtime(tmp_gps_data.utc_sec, &loc_time, conf->gps.gps_time_zone);
			tmp_gps_data.year = loc_time.tm_year + 1900;
			tmp_gps_data.mon = loc_time.tm_mon + 1;
			tmp_gps_data.day = loc_time.tm_mday;
			tmp_gps_data.hour = loc_time.tm_hour;
			tmp_gps_data.min = loc_time.tm_min;
			tmp_gps_data.sec = loc_time.tm_sec;
		}
		else
		{
			gmtime_r(&(tmp_gps_data.utc_sec), &utc_time);
			tmp_gps_data.year = utc_time.tm_year + 1900;
			tmp_gps_data.mon = utc_time.tm_mon + 1;
			tmp_gps_data.day = utc_time.tm_mday;
			tmp_gps_data.hour = utc_time.tm_hour;
			tmp_gps_data.min = utc_time.tm_min;
			tmp_gps_data.sec = utc_time.tm_sec;
		}
	}

	if(_check_gps_data(&tmp_gps_data) < 0)
	{
		LOGE(LOG_TARGET, "Error : GPS Jump!\n");
		g_skip_gps_when_error = 3;
		
		return;
	}
	
	pthread_mutex_lock(&gps_mutex);
	memcpy(&cur_gps_data, &tmp_gps_data, sizeof(cur_gps_data));
	
	pthread_mutex_lock(&gps_prev_mutex);
	memcpy(&prev_gps_data, &cur_gps_data, sizeof(prev_gps_data));
	pthread_mutex_unlock(&gps_prev_mutex);
	
	pthread_mutex_unlock(&gps_mutex);

	if(cur_gps_data.active == 1)
	{
		flag_gps_fixed = 1;
	}

	prev_gps_time = tools_get_kerneltime();

	if(gps_dbg_msg_count++ >  GPS_DBG_MSG_SKIP_COUNT) 
	{
		LOGT(LOG_TARGET, "GPS Data : [%d/%02d/%02d] [%02d:%02d:%02d] [%f/%f] [%d/%d/%d]\n",
			 cur_gps_data.year, cur_gps_data.mon, cur_gps_data.day,
			 cur_gps_data.hour, cur_gps_data.min, cur_gps_data.sec,
			 cur_gps_data.lat, cur_gps_data.lon,
			 cur_gps_data.satellite, cur_gps_data.speed, cur_gps_data.active);
			 
		gps_dbg_msg_count = 0;
	}

    // dbug msg for common models
    if ( ( send_gps_active_log == 0 ) && ( cur_gps_data.active == 1) )
    {
        if ( ( nettool_get_state() == DEFINES_MDS_OK ) )
        {
            devel_webdm_send_log("[GPS TOOL] First GPS ACT [%d]", mds_api_gps_util_get_gps_ant());
            send_gps_active_log = 1;
        }
    }

#ifdef MDS_FEATURE_USE_GPS_DEACTIVE_RESET
    if ( ( power_get_ignition_status() == POWER_IGNITION_ON) && (cur_gps_data.active == 0) )
    {
        LOGE(LOG_TARGET, "[GPS TOOL] DEACT GPS : [%d]/[%d]\r\n",gps_deactive_time, MAX_GPS_DEACTIVE_CHK_TIME_SEC);
        gps_deactive_time++;
    }
    else
    {
        gps_deactive_time = 0;
    }

    if (gps_deactive_time > MAX_GPS_DEACTIVE_CHK_TIME_SEC )
    {
        devel_webdm_send_log("[GPS TOOL] GPS ACT FAIL.: COLD BOOT");
        gps_reset_immediately(GPS_BOOT_COLD);
        gps_deactive_time = 0;
    }
#endif
}

void gps_get_curr_data(gpsData_t* out)
{
	pthread_mutex_lock(&gps_mutex);
	memcpy(out, &cur_gps_data, sizeof(gpsData_t));
	pthread_mutex_unlock(&gps_mutex);
}

void gps_get_prev_data(gpsData_t* out)
{
	pthread_mutex_lock(&gps_prev_mutex);
	memcpy(out, &prev_gps_data, sizeof(gpsData_t));
	pthread_mutex_unlock(&gps_prev_mutex);
}

void gps_set_prev_data_utc(const time_t* in)
{
	pthread_mutex_lock(&gps_prev_mutex);
	prev_gps_data.utc_sec = *in;
	pthread_mutex_unlock(&gps_prev_mutex);
}


int gps_chk_valid_time(gpsData_t* gpsdata)
{
	static int last_sec = 0;
	int cur_sec = gpsdata->sec + (gpsdata->min*60) + (gpsdata->hour*60*60);
	int ret_val = -1;

	if ( cur_sec > last_sec )
	{
		ret_val = 1;
	}
	else
	{
		//LOGE(LOG_TARGET, "GPS INVALID TIME SEC : cur [%d] / last [%d]\n", cur_sec, last_sec);
		ret_val = -1;
	}
	
	last_sec = cur_sec;
	//last_daily_date_num = cur_daily_date_num = (gpsdata.year % 100)*10000 + gpsdata.mon*100 + gpsdata.day;
	return ret_val;
}

void gps_set_time_gpsData(int year, int mon, int day, int hour, int min, int sec, gpsData_t *gd)
{
	gd->year = year;
	gd->mon = mon;
	gd->day = day;
	gd->hour = hour;
	gd->min = min;
	gd->sec = sec;

	LOGT(LOG_TARGET, "%s %d %d %d %d %d %d\n", __FUNCTION__, year, mon, day, hour, min, sec);
}

int gps_start_utc_adjust(void)
{
	time_t time_utc = 0;
	configurationBase_t *conf = get_config_base();

	if(flag_gps_pipe_emul)
	{
		return 0;
	}

	//time_utc = get_modem_time_utc_sec();
	time_utc = get_system_time_utc_sec(conf->gps.gps_time_zone);
	if(time_utc == 0)
	{
		LOGT(LOG_TARGET, "modem utc fail\n");
		if(at_get_modemtime(&time_utc,0) < 0)
		{
			error_critical(eERROR_LOG, "get modem time error");
			return -1;
		}
		
		gps_set_prev_data_utc(&time_utc);
		LOGT(LOG_TARGET, "modem utc %d TZ:%d\n", time_utc, timezone);
	
		flag_enabled_modem_time_driver = 0;
	}
	else
	{
		LOGT(LOG_TARGET, "modem utc success\n");
		flag_enabled_modem_time_driver = 1;
	}

	flag_modem_utc_setted = 1;	
	return 0;
}

static int _check_distance(gpsData_t *gpsdata)
{
	static int is_prev_gps_valid = 0;
	static int num_gps_error = 0;
	int dist_m = 0;
	int time_sec = 0;
	int dist_diff = 0;
	gpsData_t last;
	configurationBase_t *conf = get_config_base();

	//distance check
	if(gpsdata->active == 0)
	{
		is_prev_gps_valid = 0;

		return 0;
	}

	gps_valid_data_get(&last);

	if(is_set_last_gpsdata == 0)
	{
		if(last.active == 1 && num_gps_error < 2)
		{
			dist_m = get_distance_m(last.lat, gpsdata->lat, last.lon, gpsdata->lon);
			if(dist_m < 0)
			{
				return 0;
			}
			
			if(dist_m > conf->gps.gps_err_first_dist_m)
			{
				num_gps_error++;
			
				LOGT(LOG_TARGET, "last:%f,%f cur:%f,%f\n",last.lat,last.lon ,gpsdata->lat, gpsdata->lon);

				error_critical(eERROR_LOG,"GPS : Position is jumped at first.(%d) %f,%f %d(km/s) %f,%f %d(km/s) %d(m)",
					num_gps_error, last.lat, last.lon, last.speed,
					gpsdata->lat, gpsdata->lon, gpsdata->speed, dist_m);
				devel_webdm_send_log("GPS : Position is jumped at first.(%d) %f,%f %d(km/s) %f,%f %d(km/s) %d(m)",
					num_gps_error, last.lat, last.lon, last.speed,
					gpsdata->lat, gpsdata->lon, gpsdata->speed, dist_m);
				
				gps_reset(GPS_BOOT_WARM);
				
				return -1;
			}
		}

		num_gps_error = 0;
	}

	if(is_set_last_gpsdata == 0 || is_prev_gps_valid == 1)
	{
		is_set_last_gpsdata = 1;
		is_prev_gps_valid = 1;
		gps_valid_data_set(gpsdata);
		crit_set_data_gps_write(gpsdata->lat, gpsdata->lon, gpsdata->utc_sec);
		return 0;
	}

	dist_m = get_distance_km(last.lat, gpsdata->lat, last.lon, gpsdata->lon) * 1000;
	if(dist_m < 0)
	{
		return 0;
	}
	
	if(dist_m < conf->gps.gps_err_ignore_dist_m)
	{
		printf("ignore : distance is smaller than %d\n", conf->gps.gps_err_ignore_dist_m);
		num_gps_error = 0;
		is_prev_gps_valid = 1;
		gps_valid_data_set(gpsdata);
		crit_set_data_gps_write(gpsdata->lat, gpsdata->lon, gpsdata->utc_sec);
		
		return 0;
	}
	
	time_sec = gpsdata->utc_sec - last.utc_sec;
	printf("dist %d, time %d\n", dist_m, time_sec);
	
	if(time_sec <= 0)
	{
		printf("error : time_sec is 0\n");
		num_gps_error = 0;
		is_prev_gps_valid = 1;
		gps_valid_data_set(gpsdata);
		crit_set_data_gps_write(gpsdata->lat, gpsdata->lon, gpsdata->utc_sec);
		
		return 0;
	}

	dist_diff = dist_m / time_sec;
	
	if(dist_diff >= (conf->gps.gps_err_dist_kms*1000/3600)  && num_gps_error < 3)
	{
		num_gps_error++;
	
		LOGT(LOG_TARGET, "last:%f,%f cur:%f,%f\n",last.lat,last.lon ,gpsdata->lat, gpsdata->lon);

		error_critical(eERROR_LOG,"GPS : Position is jumped on running.(%d) %f,%f %d(km/s) %f,%f %d(km/s) %d(s) %d(m)",
			num_gps_error, last.lat, last.lon, last.speed,
			gpsdata->lat, gpsdata->lon, gpsdata->speed, time_sec, dist_m);
		devel_webdm_send_log("GPS : Position is jumped on running.(%d) %f,%f %d(km/s) %f,%f %d(km/s) %d(s) %d(m)",
			num_gps_error, last.lat, last.lon, last.speed,
			gpsdata->lat, gpsdata->lon, gpsdata->speed, time_sec, dist_m);
		
		
		if(num_gps_error == 3)
		{
			gps_reset(GPS_BOOT_COLD);
			//gps_reset(GPS_BOOT_WARM);
		}
		else
		{
			gps_reset(GPS_BOOT_WARM);
		}
		


		is_prev_gps_valid = 0; //maybe useless

		return -1;
	}

	//inje tunnel's length is 10960m
	if(dist_m >= conf->gps.gps_err_max_dist_m && num_gps_error < 2)
	{
		num_gps_error++;
	
		LOGT(LOG_TARGET, "last:%f,%f cur:%f,%f\n",last.lat,last.lon ,gpsdata->lat, gpsdata->lon);

		error_critical(eERROR_LOG,"GPS : Position is jumped longer thran %dm. %f,%f %d(km/s) %f,%f %d(km/s) %d(s) %d(m)",
			conf->gps.gps_err_max_dist_m,last.lat, last.lon, last.speed,
			gpsdata->lat, gpsdata->lon, gpsdata->speed, time_sec, dist_m);
		devel_webdm_send_log("GPS : Position is jumped longer thran %dm. %f,%f %d(km/s) %f,%f %d(km/s) %d(s) %d(m)",
			conf->gps.gps_err_max_dist_m,last.lat, last.lon, last.speed,
			gpsdata->lat, gpsdata->lon, gpsdata->speed, time_sec, dist_m);
		
		gps_reset(GPS_BOOT_WARM);
		
		is_prev_gps_valid = 0; //maybe useless

		return -1;
	}

	num_gps_error = 0;
	is_prev_gps_valid = 1;
	gps_valid_data_set(gpsdata);
	crit_set_data_gps_write(gpsdata->lat, gpsdata->lon, gpsdata->utc_sec);
	
	return 0;
}

static int _check_speed(gpsData_t *gpsdata)
{
	configurationBase_t *conf = get_config_base();

	if(gpsdata->active == 0)
	{
		return 0;
	}

	if(gpsdata->speed >= conf->gps.gps_err_speed_kms)
	{
		error_critical(eERROR_LOG,"GPS : Gps speed is more than %dkm/s %f,%f %d(km/s)",
			conf->gps.gps_err_speed_kms, gpsdata->lat, gpsdata->lon, gpsdata->speed);
		devel_webdm_send_log("GPS : Gps speed is more than %dkm/s %f,%f %d(km/s)",
			conf->gps.gps_err_speed_kms, gpsdata->lat, gpsdata->lon, gpsdata->speed);
		
		gps_reset(GPS_BOOT_WARM);
		
		return -1;
	}

	return 0;
}

static int _check_gps_data(gpsData_t *gpsdata)
{
	configurationBase_t *conf = get_config_base();
	
	if(conf->gps.gps_err_enable == 0)
	{
		return 0;
	}
	
	if(_check_speed(gpsdata) < 0)
	{
		return -1;
	}
	
	if(_check_distance(gpsdata) < 0)
	{
		return -1;
	}
	
	return 0;
}

void gps_valid_data_get(gpsData_t *last)
{
	pthread_mutex_lock(&gps_last_mutex);
	memcpy(last, &last_valid_gpsdata, sizeof(gpsData_t));
	pthread_mutex_unlock(&gps_last_mutex);
}

void gps_valid_data_set(gpsData_t *last)
{
	pthread_mutex_lock(&gps_last_mutex);
	memcpy(&last_valid_gpsdata, last, sizeof(gpsData_t));
	pthread_mutex_unlock(&gps_last_mutex);
}

void gps_valid_data_clear(void)
{
	pthread_mutex_lock(&gps_last_mutex);
	memset(&last_valid_gpsdata, 0, sizeof(last_valid_gpsdata));
	pthread_mutex_unlock(&gps_last_mutex);
}

int gps_valid_data_write(void)
{
	configurationBase_t *conf = get_config_base();
	gpsData_t last;
	
	if(conf->gps.gps_err_enable == 0)
	{
		return 0;
	}

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
	return 0;
}

int gps_valid_data_read(void)
{
	configurationBase_t *conf = get_config_base();
	gpsData_t last;
	float crit_lat = 0, crit_lon = 0;
	unsigned int crit_utc = 0;
	int res_load_file = 0;
	
    int i = 3;

    memset(&last, 0x00, sizeof(last));

	if(conf->gps.gps_err_enable == 0)
	{
        LOGE(LOG_TARGET, "WARN : CRIT GPS DATA return fail 0\n");
        printf("WARN : CRIT GPS DATA return fail 0\r\n");
		return 0;
	}

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
	
	return 0;
}

void gps_restart_check_distance(void)
{
	is_set_last_gpsdata = 0;
}

