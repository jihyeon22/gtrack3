#ifndef _STANDARD_PROTOCOL_H_
#define _STANDARD_PROTOCOL_H_

#pragma pack(push, 1)
struct tacom_std_hdr {
	char vehicle_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char registration_num[12];
	char business_license_num[10];
	char driver_code[18];
#if defined(DEVICE_MODEL_INNOCAR) || defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
	char dtg_fw_ver[8];
#endif
}__attribute__((packed));
typedef struct tacom_std_hdr tacom_std_hdr_t;

struct tacom_std_data {
	char day_run_dist[4];
	char cumul_run_dist[7];
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
	/* extended */
	char day_oil_usage[9];
	char cumulative_oil_usage[9];
	char temperature_A[5];
	char temperature_B[5];
	char residual_oil[7];
#if defined(DEVICE_MODEL_INNOCAR) || defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
	unsigned short k_factor;
	char rpm_factor;
	char weight1;
	char weight2;
#endif
}__attribute__((packed));
typedef struct tacom_std_data tacom_std_data_t;
#pragma pack(pop)

#endif 
