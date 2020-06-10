#ifndef __MODEL_CONFIG_H__
#define __MODEL_CONFIG_H__

/*===========================================================================================*/
typedef struct configModel configModel_t;
struct configModel
{
	char report_ip[40];
	int report_port;
	/*
	char request_ip[40];
	int request_port;

	int collect_interval_keyon;
	int collect_interval_keyoff;
	*/
	int report_interval_keyon;
	int report_interval_keyoff;
	
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

#define DEFAULT_USER_CFG_SERVER_IP 		"118.220.16.91"
#define DEFAULT_USER_CFG_SERVER_PORT	50002

#define DEFAULT_USER_CFG_KEY_ON_COLLECT_INTERVAL	60
#define DEFAULT_USER_CFG_KEY_ON_SEND_INTERVAL		60

#define DEFAULT_USER_CFG_KEY_OFF_COLLECT_INTERVAL	360
#define DEFAULT_USER_CFG_KEY_OFF_SEND_INTERVAL		360

#define DEFAULT_USER_CFG_MODEL_NAME	"skyan1"

int get_user_cfg_report_ip(char* buff);
int get_user_cfg_report_port(int* port);
int get_user_cfg_keyon_interval();
int get_user_cfg_keyoff_interval();


int set_user_cfg_report_ip(char* buff);
int set_user_cfg_report_port(int port);
int set_user_cfg_keyon_interval(int sec);
int set_user_cfg_keyoff_interval(int sec);

#endif
