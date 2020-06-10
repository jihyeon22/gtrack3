#ifndef __GMMP_NETWORK_DEFINE_HEADER__
#define __GMMP_NETWORK_DEFINE_HEADER__

enum {
	SMS,
	TCP
}GMMP_CONTROL_INTERFACE_TYPE;

enum {
	GMMP_GW_REGISTRATION_REQUEST			= 0x01,
	GMMP_GW_REGISTRATION_RESPONSE,
	GMMP_GW_DE_REGISTRATION_REQUEST,
	GMMP_GW_DE_REGISTRATION_RESPONSE,

	GMMP_PROFILE_REQUEST,
	GMMP_PROFILE_RESPONSE,
	
	GMMP_DEVICE_REGISTRATION_REQUEST,
	GMMP_DEVICE_REGISTRATION_RESPONSE,
	GMMP_DEVICE_DE_REGISTRATION_REQUEST,
	GMMP_DEVICE_DE_REGISTRATION_RESPONSE,

	GMMP_PACKET_DELIVERY_REQUEST,
	GMMP_PACKET_DELIVERY_RESPONSE,

	GMMP_CONTROL_REQUEST,
	GMMP_CONTROL_RESPONSE,

	GMMP_HEARTBEAT_REQUEST,
	GMMP_HEARTBEAT_RESPONSE,
	
	GMMP_LONG_SENTENCE_REQUEST,
	GMMP_LONG_SENTENCE_RESPONSE,
	
	GMMP_FTP_INFO_REQUEST,
	GMMP_FTP_INFO_RESPONSE,
	
	GMMP_REMOTE_INFO_REQUEST,
	GMMP_REMOTE_INFO_RESPONSE,
	
	GMMP_CONTROL_NOTIFICATION,
	GMMP_CONTROL_NOTIFICATION_RESPONSE
}GMMP_MESSAGE_TYPE;

enum {
	COLLECT_DATA = 1,
	ALARM_DATA,
	EVVENT_DATA,
	ALARM_CLEAR
}GMMP_REPORT_TYPE;

enum {
	MEDIA_TYPE_MSG_HTTP = 0x50
}GMMP_MEDIA_TYPE;

enum {
	CTRL_RESET = 1,
	CTRL_TURN_OFF,
	CTRL_REPORT_ON,
	CTRL_REPORT_OFF,
	CTRL_TIME_SYNC,
	CTRL_PAUSE,
	CTRL_RESTART,
	CTRL_SIGNAL_POWER_CHECK,
	CTRL_DIAGNOSTIC,
	CTRL_RESERVED,
	CTRL_PROFILE_RESET,
	CTRL_STATUS_CHECK,
	CTRL_FW_DOWNLOAD,
	CTRL_FW_UPGRADE,
	CTRL_APP_DOWNLOAD,
	CTRL_APP_UPDATE,
	CTRL_REMOTE_ACCESS,
}GMMP_CONTROL_TYPE;

enum {
	GW_REG_REQUEST_PACKET_DUMP,
	GW_REG_RESPONSE_PACKET_DUMP,
	GW_PROFILE_REQUEST_PACKET_DUMP,
	GW_PROFILE_RESPONSE_PACKET_DUMP,
	GW_DELIVERY_REQUEST_PACKET_DUMP,
	GW_DELIVERY_RESPONSE_PACKET_DUMP,
	GW_CONTROL_REQUEST_PACKET_DUMP,
	GW_CONTROL_REQUEST_SMS_PACKET_DUMP,
	GW_CONTROL_RESPONSE_PACKET_DUMP,
	GW_LONG_SENTENCE_REQUEST_PACKET_DUMP,
	GW_CONTROL_NOTIFICATION_PACKET_DUMP,
	GW_CONTROL_NOTIFICATION_RESPONSE_PACKET_DUMP,
	GW_FTP_REQUEST_PACKET_DUMP,
	GW_FTP_RESPONSE_PACKET_DUMP,
	GW_REMOTE_REQUEST_PACKET_DUMP,
	GW_REMOTE_RESPONSE_PACKET_DUMP,
	GW_HEARTBEAT_REQUEST_PACKET_DUMP,
	GW_HEARTBEAT_RESPONSE_PACKET_DUMP,
	GW_DEREG_REQUEST_PACKET_DUMP,
	GW_DEREG_RESPONSE_PACKET_DUMP,
}GMMP_DUMP_TYPE;

#pragma pack(push, 1)
//GMMP common header
typedef struct {
	char	version;
	short	msg_length; //packet total length;
	char	msg_type;
	int		time_stamp;
	short	total_count;
	short	current_count;
	char	auth_id[16];
	char	auth_key[16];
	unsigned	int		transaction_id;
	char	reserved1;
	char	reserved2;
}__attribute__((packed))GMMP_COMMON_HEADER;

//GMMP gateway registration request packet header
typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char manufacture_id[16];
}__attribute__((packed))GMMP_GW_REQUEST_HDR;

//GMMP gateway registration response packet header
typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	unsigned char result_code;
}__attribute__((packed))GMMP_GW_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
}__attribute__((packed))GMMP_DEREGISTRATION_REQUEST_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	unsigned char result_code;
}__attribute__((packed))GMMP_DEREGISTRATION_RESPONSE_HDR;

//GMMP gateway profile request packet header
typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
}__attribute__((packed))GMMP_PROFILE_REQUEST_HDR;

//GMMP gateway profile reponse packet header
typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char result_code;
	int heartbeat_period;
	int reporting_period;
	int report_offset;
	int response_timeout;
	char model[32];
	char firmware_version[16];
	char software_version[16];
	char hardware_version[16];
}__attribute__((packed))GMMP_PROFILE_RESPONSE_HDR;


//GMMP packet delivery request packet header
typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	char report_type;
	char media_type;
	unsigned char message_body[2048];
}__attribute__((packed))GMMP_DELIVERY_REQUEST_HDR;

//GMMP packet delivery response packet header
typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char result_code;
	int offset_time;
}__attribute__((packed))GMMP_DELIVERY_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	union {
		unsigned char message_body[2048];
		int time_stamp;
		struct{
			char filename[32];
			char app_update_command;
		} app_update_request;
	};
}__attribute__((packed))GMMP_CONTROL_REQUEST_HDR; 

typedef struct {
	unsigned int omp_transaction_id;
	unsigned char control_type;
	char long_sentence_flag;
	char device_id[16];
	union {
		unsigned char message_body[39];
		int time_stamp;
		struct{
			char filename[32];
			char app_update_command;
		} app_update_request;
	};
}__attribute__((packed))GMMP_CONTROL_REQUEST_SMS_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
}__attribute__((packed))GMMP_CONTROL_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
}__attribute__((packed))GMMP_HEARTBEAT_REQUEST_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
}__attribute__((packed))GMMP_HEARTBEAT_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
}__attribute__((packed))GMMP_LONG_SENTENCE_REQUEST_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
	unsigned char message_body[2048];
}__attribute__((packed))GMMP_LONG_SENTENCE_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
}__attribute__((packed))GMMP_FTP_INFO_REQUEST_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
	char ftp_server_ip[16];
	unsigned short ftp_server_port;
	char ftp_server_id[16];
	char ftp_server_password[16];
	char file_path[64];
	char file_name[32];
	char file_hash[32];
	unsigned int file_size;
}__attribute__((packed))GMMP_FTP_INFO_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
}__attribute__((packed))GMMP_REMOTE_INFO_REQUEST_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
	char remote_server_ip[16];
	unsigned short remote_server_port;
	char remote_server_id[16];
	char remote_server_password[16];
	unsigned int accessible_start_time;
	unsigned int accessible_end_time;
}__attribute__((packed))GMMP_REMOTE_INFO_RESPONSE_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
	union {
		unsigned char message_body[2048];
		int signal_power;
		char diag_status;
		struct {
			char on_off;
			char run_pause;
		} status_check;
		struct {
			int start_time;
			int end_time;
			char result;			
		} response_type1;
	};
}__attribute__((packed))GMMP_CONTROL_NOTIFICATION_HDR;

typedef struct {
	GMMP_COMMON_HEADER gmmp_common;
	char domain_code[10];
	char gw_id[16];
	char device_id[16];
	unsigned char control_type;
	unsigned char result_code;
}__attribute__((packed))GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR;

#define PHONE_NUMBER_LEN 12

typedef struct type_GMMP_DATA_HEADER{
	char phone_number[PHONE_NUMBER_LEN];
	int msg_type;
	char ip[16];
}__attribute__((packed))GMMP_CIP_DATA_HEADER;

typedef struct type_GMMP_DATA{
	GMMP_CIP_DATA_HEADER head;
	unsigned char data[1];
}__attribute__((packed))GMMP_CIP_DATA_PACKET;

#pragma pack(pop)

//int send_gw_regi(GMMP_GW_RESPONSE_HDR *p_gw_res);
int send_gw_regi(
				char *domain_id, 
				char *manufacture_id, 
				GMMP_GW_RESPONSE_HDR *p_gw_res);
int send_gw_profile(char *gw_id, char *dev_id, char *auth_key, GMMP_PROFILE_RESPONSE_HDR *p_gw_profile_res);
int send_delivery_packet(char report_type,char media_type,unsigned char *data,int data_len);
int send_control_response(int transaction_id,unsigned char control_type);
int send_ftp_info_request(unsigned char control_type,int transaction_id,GMMP_FTP_INFO_RESPONSE_HDR *gw_ftp_res);
int send_remote_request(int transaction_id,GMMP_REMOTE_INFO_REQUEST_HDR *gw_remote_res);
int send_control_notification(char control_type,int transaction_id,int message_size,int result,GMMP_CONTROL_NOTIFICATION_HDR *gw_control_noti,
GMMP_CONTROL_NOTIFICATION_RESPONSE_HDR *gw_control_noti_res);
#endif
