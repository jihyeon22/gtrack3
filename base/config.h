#ifndef __BASE_CONFIG_H__
#define __BASE_CONFIG_H__

#include <dictionary.h>
#include <board/board_system.h>

#define INI_INVALID_KEY_STRING	NULL
#define INI_INVALID_KEY_INT		-1
#define INI_RETRY_CNT			3
#define NOTI_DEFAULT_PHONNUM "01030803161"
#define INI_MAX_SAVE_ARRAY		20

typedef struct configCommon configCommon_t;
struct configCommon {
	int bootstrap;
	int initial_turnoff;
	int first_pwr_status_on;
	int time_hold_power;
	int time_hold_ign;
	int log_poweroff;
	char sms_passwd[20];
	int alive_time_sec;
};

#ifdef USE_GPS_MODEL
typedef struct configGPS configGps_t;
struct configGPS {
	int gps_time_zone;
	int adjust_gps;
	int gps_err_enable;
	int gps_err_speed_kms;
	int gps_err_dist_kms;
	int gps_err_ignore_dist_m;
	int gps_err_first_dist_m;
	int gps_err_max_dist_m;
};

typedef struct configMileage configMileage_t;
struct configMileage {
	int enable;
	int daily_mileage;
};
#endif

typedef struct configWebdm configWebdm_t;
struct configWebdm {
	int enable;
	int tx_power;
	int tx_ignition;
	int tx_report;
	int tx_log;
};

typedef struct configNoti configNoti_t;
struct configNoti {
	int turn_on_sms_enable;
	char target_num[16];
};

typedef struct configurationBase configurationBase_t;
struct configurationBase {
	configCommon_t common;
#ifdef USE_GPS_MODEL
	configGps_t gps;
	configMileage_t mileage;
#endif
	configWebdm_t webdm;
	configNoti_t noti;
};

typedef struct iniData iniData_t;
struct iniData {
	char *name;
	char *msg;
};

configurationBase_t *load_config_base(void);
configurationBase_t *get_config_base(void);

int save_config(const char *name, char *msg);
int save_config_array(iniData_t *data);
int save_config_user(const char *name, char *msg);
int save_config_array_user(iniData_t *data);
int get_value_ini_int(dictionary *d, const char *key, int *var);
int get_value_ini_str(dictionary *d, const char *key, char *buff, const int buff_len);

#endif
