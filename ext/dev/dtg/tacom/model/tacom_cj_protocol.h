#ifndef _TACOM_CJ_PROTOCOL_H_
#define _TACOM_CJ_PROTOCOL_H_

struct tacom_cj_hdr {
	char  dtg_model[20];
	char  vehicle_id_num[17];
	char  vehicle_type[2];
	char  regist_num[12];
	char  business_num[10];
	char  driver_code[18];
	char seperator;
}__attribute__((packed));
typedef struct tacom_cj_hdr	tacom_cj_hdr_t;

struct tacom_cj_data {
	char day_run_distance[4];
	char cumulative_run_distance[7];
	char date_time[14];
	char speed[3];
	char rpm[4];
	char bs;
	char gps_x[9];
	char gps_y[9];
	char azimuth[3];
	char accelation_x[6];
	char accelation_y[6];
	char status[2];
	char cumulative_oil_usage[7];
	char residual_oil[3];
	char temperature_A[4];
	char temperature_B[4];
//	char seperator;
}__attribute__((packed));
typedef struct tacom_cj_data tacom_cj_data_t;

/* value of expression 'MAX_CJ_DATA * MAX_CJ_DATA_PACK' 
 * have to set over cj_setup.max_records_per_once's value.
 */
#define MAX_CJ_DATA		10

#if defined(SERVER_MODEL_GTRS) || defined (SERVER_MODEL_NEOGNP) || defined(SERVER_MODEL_MORAM)
	#define MAX_CJ_DATA_PACK	1200
#else
	#define MAX_CJ_DATA_PACK	2400
#endif

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_cj_data_t buf[MAX_CJ_DATA];
}cj_data_pack_t;

extern const struct tm_ops cj_ops;
extern struct tacom_setup cj_setup;

#endif
