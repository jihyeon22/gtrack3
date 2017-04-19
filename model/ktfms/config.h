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
	// ----------------------------------------
	char user_business_no[13+1];
	char car_vin[17+1];
	char car_no[12+1];
	char driver_id[20+1];
	//char company_id[32+1];
	char url_path[64+1];
	// -----------------------------------------
	int use_obd;

	char model_name[20+1];
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
int set_collection_interval(int sec);
int get_report_interval();
int set_report_interval(int sec);

// ----------------------------------------
//  for ktfms spec
// ----------------------------------------
int get_server_ip(char* buff);
int set_server_ip(const char* buff);
int get_server_port();
int set_server_port(int port);



int set_server_port(int port);
//int get_network_param(transferSetting_t* network_param);
int get_car_info_user_business_no(unsigned char* user_business_no);
int save_car_info_user_business_no(unsigned char* user_business_no);
int get_car_info_car_vin(unsigned char* car_vin);
int save_car_info_car_vin(unsigned char* car_vin);
int get_car_info_car_no(unsigned char* car_no);
int save_car_info_car_no(unsigned char* car_no);
int get_car_info_driver_id(unsigned char* driver_id);
int save_car_info_driver_id(unsigned char* driver_id);
int get_car_info_company_id(unsigned char* company_id);
int set_car_info_company_id(unsigned char* company_id);
int get_car_info_url_path(unsigned char* url_path);
int set_car_info_url_path(unsigned char* url_path);

int get_use_obd_device();

#endif
