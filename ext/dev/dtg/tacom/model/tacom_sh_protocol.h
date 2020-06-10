#ifndef _TACOM_SH_PROTOCOL_H_
#define _TACOM_SH_PROTOCOL_H_

/* 신흥 헤더 */
struct tacom_sh_hdr {
	char  dtg_model[20];
	char  vehicle_id_num[17];
	char  vehicle_type[2];
	char  regist_num[12];
	char  business_num[10];
	char  driver_code[18];
	char seperator;
}__attribute__((packed));
typedef struct tacom_sh_hdr	tacom_sh_hdr_t;

/* 신흥 데이터 */
struct tacom_sh_data {
	char day_run_distance[4];			//일일주행거리
	char cumulative_run_distance[7];	//누적주행거리
	char date_time[14];					//정보발생일시
	char speed[3];						//속도
	char rpm[4];						//rpm
	char bs;							//브레이크신호
	char gps_x[9];						//
	char gps_y[9];
	char azimuth[3];
	char accelation_x[6];
	char accelation_y[6];
	char status[2];
	char cumulative_oil_usage[7];
	char residual_oil[3];
	char temperature_A[4];
	char temperature_B[4];
	char seperator;
}__attribute__((packed));
typedef struct tacom_sh_data tacom_sh_data_t;

struct tacom_config {
	int config_flag;
	char vehicle_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char registration_num[12];
	char business_license_num[10];
	char driver_code[18];
	char speed_compensation[75];
	char rpm_compensation[75];
	char cumulative_compensation[9];
	char mileage_compensation[9];
	char fuel_compensation[7];
};
typedef struct tacom_config tacom_config_t;

/* value of expression 'MAX_SH_DATA * MAX_SH_DATA_PACK' 
 * have to set over sh_setup.max_records_per_once's value.
 */
#define MAX_SH_DATA		10
#define MAX_SH_DATA_PACK	600

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_sh_data_t buf[MAX_SH_DATA];
}sh_data_pack_t;

extern const struct tm_ops sh_ops;
extern struct tacom_setup sh_setup;

#endif
