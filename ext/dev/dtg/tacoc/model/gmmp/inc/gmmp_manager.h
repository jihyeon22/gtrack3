<<<<<<< HEAD
#ifndef GMMP_DATA_MANAGER_H_
#define GMMP_DATA_MANAGER_H_

#define DEFAULT_NETWORK_TIME	60	//30sec

typedef struct {
	char			AuthKey[16]; //서버에서 받은 값.(Auth ID는 MSISDN 혹은 MAC Address로 Auth Key와 Auth ID는 다름)
	char			GwId[16];
	char			DeviceId[16];
	int				HeartbeatPeriod;
	int				ReportPeriod;
	int				ReportOffset;
	int				ResponseTimerout;
	char			Model[32];
	char			FirmwareVer[16];
	char			SoftwareVer[16];
	char			HardwareVer[16];
}DEVICE_PROFILE_INFO;

typedef struct {
	char			server_ip[20];
	int				server_port;
	unsigned char	report_type;
	unsigned char	media_type;
	char			manufacture_id[16];
	char			profile_path[512];
	char			DomainCode[10];
	char			AuthID[16];
	int				transaction_id;
	int				delivery_on;
}DEVICE_CONF_INFO;

typedef struct {
	DEVICE_CONF_INFO	device_conf_dat;
	DEVICE_PROFILE_INFO device_profile_dat;
}GMMP_DATA_MANAGER;


void gmmp_manager_init();

void gmmp_set_server_ip(char *ip);
char* gmmp_get_server_ip();

void gmmp_set_server_port(int port);
int gmmp_get_server_port();

void gmmp_set_domain_id(char *svc_id);
char *gmmp_get_domain_id();

void gmmp_set_manufacture_id(char *manufactureID);
char *gmmp_get_manufacture_id();

void gmmp_set_network_time_out(int sec);
int gmmp_get_network_time_out();

void gmmp_set_profile_path(char *profile_path);
char *gmmp_get_profile_path();

void gmmp_set_auth_id(char *auth_id);
char *gmmp_get_auth_id();

void gmmp_set_auth_key(char *auth_key);
char *gmmp_get_auth_key();

void gmmp_set_gw_id(char *gw_id);
char* gmmp_get_gw_id();

void gmmp_set_transaction_id(int tid);
int gmmp_get_transaction_id();
int gmmp_get_transaction_id_not_increase();

void gmmp_set_reporting_period(int reporting_period);
int gmmp_get_reporting_period();

void gmmp_set_delivery_state(int value);
int gmmp_get_delivery_state();

int gmmp_get_time_stamp();

int read_profile_contents(char *profile_path);
int write_profile_contents(DEVICE_PROFILE_INFO profile, char *profile_path);

#endif
=======
#ifndef GMMP_DATA_MANAGER_H_
#define GMMP_DATA_MANAGER_H_

#define DEFAULT_NETWORK_TIME	60	//30sec

typedef struct {
	char			AuthKey[16]; //서버에서 받은 값.(Auth ID는 MSISDN 혹은 MAC Address로 Auth Key와 Auth ID는 다름)
	char			GwId[16];
	char			DeviceId[16];
	int				HeartbeatPeriod;
	int				ReportPeriod;
	int				ReportOffset;
	int				ResponseTimerout;
	char			Model[32];
	char			FirmwareVer[16];
	char			SoftwareVer[16];
	char			HardwareVer[16];
}DEVICE_PROFILE_INFO;

typedef struct {
	char			server_ip[20];
	int				server_port;
	unsigned char	report_type;
	unsigned char	media_type;
	char			manufacture_id[16];
	char			profile_path[512];
	char			DomainCode[10];
	char			AuthID[16];
	int				transaction_id;
	int				delivery_on;
}DEVICE_CONF_INFO;

typedef struct {
	DEVICE_CONF_INFO	device_conf_dat;
	DEVICE_PROFILE_INFO device_profile_dat;
}GMMP_DATA_MANAGER;


void gmmp_manager_init();

void gmmp_set_server_ip(char *ip);
char* gmmp_get_server_ip();

void gmmp_set_server_port(int port);
int gmmp_get_server_port();

void gmmp_set_domain_id(char *svc_id);
char *gmmp_get_domain_id();

void gmmp_set_manufacture_id(char *manufactureID);
char *gmmp_get_manufacture_id();

void gmmp_set_network_time_out(int sec);
int gmmp_get_network_time_out();

void gmmp_set_profile_path(char *profile_path);
char *gmmp_get_profile_path();

void gmmp_set_auth_id(char *auth_id);
char *gmmp_get_auth_id();

void gmmp_set_auth_key(char *auth_key);
char *gmmp_get_auth_key();

void gmmp_set_gw_id(char *gw_id);
char* gmmp_get_gw_id();

void gmmp_set_transaction_id(int tid);
int gmmp_get_transaction_id();
int gmmp_get_transaction_id_not_increase();

void gmmp_set_reporting_period(int reporting_period);
int gmmp_get_reporting_period();

void gmmp_set_delivery_state(int value);
int gmmp_get_delivery_state();

int gmmp_get_time_stamp();

int read_profile_contents(char *profile_path);
int write_profile_contents(DEVICE_PROFILE_INFO profile, char *profile_path);

#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
