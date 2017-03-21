/**
* @file tacom_daesin_protocol.h
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-22
*/

#ifndef _TACOM_DAESIN_PROTOCOL_H_
#define _TACOM_DAESIN_PROTOCOL_H_

#pragma pack(push, 1)

typedef struct {
	char start_m;
	char dtg_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char regist_num[12];
	char business_num[10];
	char driver_code[18];
	char crc16_d[2];
	char end_m;
}__attribute__((packed))tacom_daesin_hdr_t;

typedef struct {
	char start_m;
	unsigned short day_dist;
	unsigned long all_dist;
	unsigned char year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char m_sec;
	unsigned char speed;
	unsigned short rpm;
	unsigned char bs;
	unsigned long longitude;
	unsigned long latitude;
	unsigned short azimuth;
	short accelation_x;
	short accelation_y;
	unsigned char status;
	unsigned long day_oil;
	unsigned long all_oil;
	short temp_a;
	short temp_b;
	unsigned long residual_oil;
	char crc16_d[2];
	char end_m;
}__attribute__((packed))tacom_daesin_data_t;

#pragma pack(pop)

/* value of expression 'MAX_DAESIN_DATA * MAX_DAESIN_DATA_PACK' 
 * have to set over daesin_setup.max_records_per_once's value.
 */
#define MAX_DAESIN_DATA		10
#ifdef SERVER_MODEL_GTRS
	#define MAX_DAESIN_DATA_PACK	400
#else
	#define MAX_DAESIN_DATA_PACK	900
#endif

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_daesin_data_t buf[MAX_DAESIN_DATA];
}daesin_data_pack_t;

extern const struct tm_ops daesin_ops;
extern struct tacom_setup daesin_setup;

#endif /* _TACOM_DAESIN_PROTOCOL_H_ */
