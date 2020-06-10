#ifndef __MODEL_CONFIG_H__
#define __MODEL_CONFIG_H__

/*===========================================================================================*/
typedef struct configModel configModel_t;
struct configModel
{
	char report_ip[40];
	int report_port;
	char request_ip[40];
	int request_port;
	int collect_interval_keyon;
	int collect_interval_keyoff;
	int report_interval_keyon;
	int report_interval_keyoff;
	int tcp_connect_retry_count;
	int tcp_send_retry_count;
	int tcp_receive_retry_count;
	int tcp_timeout_secs;
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

int get_collection_interval();
int get_report_interval();
int set_collection_interval(int sec);
int set_report_interval(int sec);
int get_server_ip(char* buff);
int set_server_ip(const char* buff);
int get_server_port();
int set_server_port(int port);

#endif
