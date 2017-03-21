#ifndef _TACOM_STD_PROTOCOL_H_
#define _TACOM_STD_PROTOCOL_H_

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
	char day_run_distance[4];			// 일일 주행거리
	char cumulative_run_distance[7];	// 누적주행거리
	char date_time[14];					// 데이터발생 일시
	char speed[3];						// 속도
	char rpm[4];						// RPM
	char bs;							// 브레이크 신호
	char gps_x[9];						// 차량위치 (X)
	char gps_y[9];						// 차량위치 (Y)
	char azimuth[3];					// 방위각
	char accelation_x[6];				// 가속도 (Vx)
	char accelation_y[6];				// 가속도 (Vy)
	char status[2];						// 상태코드
	/* extended */
	char day_oil_usage[9];				// 일일 유류사용량
	char cumulative_oil_usage[9];		// 누적 유류 사용량
	char temperature_A[5];
	char temperature_B[5];
	char residual_oil[7];				// 유류잔량
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
