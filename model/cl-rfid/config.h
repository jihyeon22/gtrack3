#ifndef __MODEL_CONFIG_H__
#define __MODEL_CONFIG_H__

#include <base/config.h>

/*===========================================================================================*/
typedef struct configModel configModel_t;
struct configModel
{
	char report_ip[40];
	int report_port;
	char request_ip[40];
	int request_port;
	char request_rfid[40];
	int request_rfid_port;
	int tcp_connect_retry_count;
	int tcp_send_retry_count;
	int tcp_receive_retry_count;
	int tcp_timeout_secs;
	char model_name[11];
	int max_packet;
	int interval_time;
	int stop_time;
	int section_10kms;
	int section_20kms;
	int section_30kms;
	int section_40kms;
	int section_50kms;
	int section_60kms;
	int section_70kms;
	int section_80kms;
	int section_90kms;
	int section_100kms;
	int section_110kms;
	int use_kjtec_rfid;
	char use_kjtec_dev[64];
	int use_thermal_sensor;
	char use_thermal_dev[64];
};

typedef struct configurationModel configurationModel_t;
struct configurationModel
{
	configModel_t model;
};
/*===========================================================================================*/

configurationModel_t* load_config_model(void);
configurationModel_t* get_config_model(void);
configurationModel_t* load_config_user(void);
void load_config_base_default(void);
void load_config_model_default(void);
void load_config_user_default(void);

#endif
