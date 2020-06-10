#ifndef __MODEL_CONFIG_H__
#define __MODEL_CONFIG_H__

/*===========================================================================================*/
typedef struct configModel configModel_t;
struct configModel
{
	char report_ip[40];
	int report_port;
	char ftp_ip[40];
	char ftp_port[10];
	char ftp_id[20];
	char ftp_pw[20];
	int report_interval_keyon;
	int report_interval_keyoff;
	int collect_interval_keyon;
	int collect_interval_keyoff;
	int tempature_enable;
	int tcp_connect_retry_count;
	int tcp_send_retry_count;
	int tcp_receive_timeout_secs;

	int dist_filter_enable;
	int dist_filter_value;
	int sat_filter_enable;
	int sat_filter_value;
	int hdop_filter_enable;
	int hdop_filter_value;
	int speed_filter_enable;
	int speed_filter_value;

	char model_name[11];
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

int load_config_base_webdm(void);

#endif
