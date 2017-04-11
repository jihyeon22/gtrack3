/**
* @file tacom_choyoung_protocol.h
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-06-10
*/
#ifndef _TACOM_CHOYOUNG_PROTOCOL_H_
#define _TACOM_CHOYOUNG_PROTOCOL_H_

/* value of expression 'MAX_CY_DATA * MAX_CY_DATA_PACK' 
 * have to set over cy_setup.max_records_per_once's value.
 */
#define MAX_CY_DATA			10

#if defined(SERVER_MODEL_GTRS) || defined (SERVER_MODEL_NEOGNP)
	#define MAX_CY_DATA_PACK	300
#else
	#define MAX_CY_DATA_PACK	600
#endif

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2


#pragma pack(push, 1)

typedef struct {
	char packet_type[3];
	char data_length[2];
	char dtg_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char regist_num[12];
	char business_lsn[10];
	char driver_code[18];
	char reserved[16];
	char crc16[4];
	char seperator[2];
}__attribute__((packed))tacom_cy_hdr_t;

typedef struct {
	char packet_type[3];
	char data_length[2];
	char day_run_dist[4];
	char acumul_run_dist[7];
	char date_time[14];
	char speed[3];
	char rpm[4];
	char bs;
	char gps_fix_state;
	char gps_x[9];
	char gps_y[9];
	char azimuth[3];
	char accelation_x[5];
	char accelation_y[5];
	char status[2];
	char reserved[16];
	char crc16[4];
	char seperator[2];
}__attribute__((packed))tacom_cy_data_t;

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_cy_data_t buf[MAX_CY_DATA];
}__attribute__((packed))cy_data_pack_t;

#pragma pack(pop)

extern const struct tm_ops cy_ops;
extern struct tacom_setup cy_setup;

int cy_unreaded_records_num (TACOM *tm);
#endif /* _TACOM_CHOYOUNG_PROTOCOL_H_ */
