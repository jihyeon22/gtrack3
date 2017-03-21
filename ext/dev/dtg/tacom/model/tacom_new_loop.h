/**
* @file tacom_new_loop.h
* @brief 
* @author jwrho
* @version 
* @date 2015-04-06
*/
#ifndef _TACOM_NEW_LOOP_PROTOCOL_H_
#define _TACOM_NEW_LOOP_PROTOCOL_H_

/* value of expression 'MAX_LOOP_DATA * MAX_LOOP_DATA_PACK' 
 * have to set over loop_setup.max_records_per_once's value.
 */
#define MAX_LOOP2_DATA			10
#ifdef SERVER_MODEL_GTRS
	#define MAX_LOOP2_DATA_PACK		300
#else
	#define MAX_LOOP2_DATA_PACK		600
#endif

#define DATA_PACK_EMPTY			0
#define DATA_PACK_AVAILABLE		1
#define DATA_PACK_FULL			2


#pragma pack(push, 1)

typedef struct {
	char start_packet; //0x3c
	char dtg_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char regist_num[12];
	char business_lsn[10];
	char driver_code[18];
	char end_packet; //0x3e
}__attribute__((packed))tacom_loop2_hdr_t;

typedef struct {
	char start_packet; //0x3c
	unsigned int day_run_dist;
	unsigned int acumul_run_dist;
	unsigned short year;
	unsigned char mon;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char msec; //new add
	unsigned char speed;
	unsigned short rpm;
	char bs;
	unsigned int gps_x;
	unsigned int gps_y;
	unsigned short azimuth;
	short accelation_x;
	short accelation_y;
	unsigned char status;
	unsigned int day_oil_usage;
	unsigned int acumul_oil_usage;
	char csum;
	//short temp_a;
	char end_packet; //0x3e
}__attribute__((packed))tacom_loop2_data_t;

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_loop2_data_t buf[MAX_LOOP2_DATA];
}__attribute__((packed))loop_data2_pack_t;

#pragma pack(pop)

extern const struct tm_ops loop2_ops;
extern struct tacom_setup loop2_setup;

int loop2_unreaded_records_num (TACOM *tm);
//static int valid_check(char *buf, int size);
#endif /* _TACOM_CHOYOUNG_PROTOCOL_H_ */
