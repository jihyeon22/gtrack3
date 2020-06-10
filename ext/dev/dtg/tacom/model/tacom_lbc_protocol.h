#ifndef _TACOM_LBC_PROTOCOL_H_
#define _TACOM_LBC_PROTOCOL_H_

#pragma pack(push, 1)
struct tacom_lbc_hdr {
	char vehicle_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char registration_num[12];
	char business_license_num[10];
	char driver_code[18];
//	char modem_num[11];
}__attribute__((packed));
typedef struct tacom_lbc_hdr tacom_lbc_hdr_t;

struct tacom_lbc_data {
	char day_run_distance[4];
	char acumulative_run_distance[7];
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
	char acumulative_oil_usage[7];
	char residual_oil[3];
	char temperature_A[4];
	char temperature_B[4];
}__attribute__((packed));
typedef struct tacom_lbc_data tacom_lbc_data_t;
#pragma pack(pop)

#endif 
