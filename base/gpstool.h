#ifndef __BASE_GPSTOOL_H__
#define __BASE_GPSTOOL_H__

#include <time.h>

#define GPS_PARSE_FALSE -1
#define GPS_PARSE_TRUE  0

#define GPS_MAX_BUFF_SIZE	2048

#define GPS_INVALID_HANDLE		(0xFFABCDEF)

//#define GPS_TYPE_SGPS 1
//#define GPS_TYPE_AGPS 2

#define GPS_BOOT_WARM 1
#define GPS_BOOT_COLD 2

#ifdef MDS_FEATURE_USE_GPS_DEACTIVE_RESET
#define MAX_GPS_DEACTIVE_CHK_TIME_SEC	600
int gps_get_deact_cnt();
int gps_set_deact_cnt(int sec);
#endif

#define GPS_PIPE_PATH "/var/tmp/gps"

#define TIME_REFRESH_UTC 60

typedef struct gpsData gpsData_t;
struct gpsData {
	unsigned short year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char satellite;
	float	lat;			// latitude
	float	lon;			// longitude
	int speed;			// speed(km/s)
	unsigned char active;
	float angle;
	time_t utc_sec;
	float hdop;
	float altitude;  // meter.. new filed (170913)
};

void gps_on(int type);
void gps_reset(int type);
void gps_off();
void gps_clear(int fd);
int gps_set_pipe_for_emul(void);
void gps_parse(char* buff, int size);
void gps_get_curr_data(gpsData_t* out);
void gps_get_prev_data(gpsData_t* out);
void gps_set_prev_data_utc(const time_t* in);
int gps_chk_valid_time(gpsData_t* gpsdata);

void gps_set_time_gpsData(int year, int mon, int day, int hour, int min, int sec, gpsData_t *gd);
int gps_start_utc_adjust(void);
void gps_valid_data_get(gpsData_t *last);
void gps_valid_data_set(gpsData_t *last);
int gps_valid_data_write(void);
int gps_valid_data_read(void);
void gps_restart_check_distance(void);

#define GPS_ANT_CHK_INTERVAL_CNT	10
#define GPS_ANT_CHK_CALL_API_CNT	6		// 6 * 10 => 60sec

void gps_ant_chk(void);

extern int gps_fd;
extern char gps_dev_file[32];
extern int g_skip_gps_when_error;

extern float emul_gps_lat;
extern float emul_gps_lon;
extern int emul_gps_speed;


#define CUR_GPS_STAT__ON     1
#define CUR_GPS_STAT__OFF    0

int get_cur_gps_stat();
int set_cur_gps_stat(int stat);

#endif

