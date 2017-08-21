/**
* @file tacom_innocar_protocol.h
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-06-10
*/
#ifndef _TACOM_INNNOCAR_PROTOCOL_H_
#define _TACOM_INNNOCAR_PROTOCOL_H_

#include "tacom/tools/taco_store.h"
#include "tacom/tools/mdt_store.h"

#pragma pack(push, 1)

typedef struct {
	unsigned char packet_header;
	unsigned char device_id;
	unsigned short data_length;
	char cmd[2];
	char regist_num[12];
	char vehicle_type;
	char driver_name[10];
	char driver_code[18];
	char vehicle_id_num[17];
	unsigned short k_factor;
	char rpm_factor;
	char company_name[12];
	char business_lsn[10];
	unsigned long odo;
	unsigned long oil_usage;
	char weight1;
	char weight2;
	char reserved[4];
	unsigned char checksum;
	unsigned char packet_end;
}__attribute__((packed))tacom_inno_sv_t;

typedef struct {
	unsigned char packet_header;
	unsigned char device_id;
	unsigned short data_length;
	char cmd[2];
	char regist_num[12];
	char vehicle_type;
	char driver_name[10];
	char driver_code[18];
	char vehicle_id_num[17];
	unsigned short k_factor;
	char rpm_factor;
	char company_name[12];
	char business_lsn[10];
	unsigned long odo;
	char weight1;
	char reserved[4];
	unsigned char checksum;
	unsigned char packet_end;
}__attribute__((packed))tacom_inno_old_sv_t;


typedef struct {
	unsigned char packet_header;
	unsigned char device_id;
	unsigned short data_length;
	char cmd[2];
	char regist_num[12];
	char driver_name[10];
	char driver_code[18];
	char vehicle_id_num[17];
	char company_name[12];
	char business_lsn[10];
	char dtg_model[20];
	char type_approval_num[10];
	char serial_num[14];
	char reserved0[3];
	char vehicle_type[2];
	char reserved1[25];
	char keyon_time[6];
	char reserved2[3];
	char reserved3[84];
	unsigned char checksum;
	unsigned char packet_end;
}__attribute__((packed))tacom_inno_hdr_t;

typedef struct {
	unsigned char packet_header;
	unsigned char device_id;
	unsigned short data_length;
	char cmd[2];
	unsigned char data_t_y;
	unsigned char data_t_mon;
	unsigned char data_t_d;
	unsigned char data_t_h;
	unsigned char data_t_min;
	unsigned char data_t_s;
	unsigned char data_t_cs;
	unsigned long trip_dist;		// 100m
	unsigned long cumul_dist;		// 100m
	unsigned long day_dist;			// 100m
	unsigned long trip_time;		// second
	unsigned long trip_oil_use;		// 0.05L/bit
	unsigned char speed;
	unsigned short rpm;
	unsigned char gps_status;
	unsigned char gps_y;
	unsigned char gps_mon;
	unsigned char gps_d;
	unsigned char gps_h;
	unsigned char gps_min;
	unsigned char gps_s;
	unsigned long latitude;  /* ???? */
	unsigned long longitude; /* ?? */
	unsigned short azimuth;
	unsigned char gps_speed;
	short accelation_x;
	short accelation_y;
	unsigned char ex_input;
	unsigned char status;
	unsigned long cumul_oil_usage;
	unsigned long dalily_oil_usage;
	char weight1;
	char weight2;
	unsigned char checksum;
	unsigned char packer_end;
}__attribute__((packed))tacom_inno_data_t;

#pragma pack(pop)

/* value of expression 'MAX_INNO_DATA * MAX_INNO_DATA_PACK' 
 * have to set over inno_setup.max_records_per_once's value.
 */
#if defined(SERVER_MODEL_GTRS) || defined (SERVER_MODEL_NEOGNP) || defined(SERVER_MODEL_MORAM)
	#define MAX_INNO_DATA_PACK	         300
#elif defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	#define MAX_INNO_DATA_PACK	         300  //EA : 10
	//#define MAX_INNO_MDT_DATA_PACK	     1000 //EA : 3 
	#define MAX_INNO_MDT_DATA_PACK	     3000 //EA : 1
#else
	#define MAX_INNO_DATA_PACK	         600
#endif


#pragma pack(push, 1)
typedef struct {
	unsigned char packet[sizeof(tacom_inno_data_t)];
}__attribute__((packed))tacom_dtg_data_type_t;

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_dtg_data_type_t buf[MAX_ONE_DATA_COUNT];
}__attribute__((packed))data_store_pack_t;

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_dtg_data_type_t buf[MDT_MAX_ONE_DATA_COUNT];
}__attribute__((packed))mdt_store_pack_t;
#pragma pack(pop)

extern const struct tm_ops inno_ops;
extern struct tacom_setup inno_setup;
int data_convert(tacom_std_data_t *std_data, unsigned char *dtg_data, int debug_flag);

#endif /* _TACOM_INNNOCAR_PROTOCOL_H_ */
