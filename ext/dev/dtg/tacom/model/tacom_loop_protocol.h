#ifndef __DTG_LOOP_ASCII_DATA_FORMAT_DEFINE_HEADER__
#define __DTG_LOOP_ASCII_DATA_FORMAT_DEFINE_HEADER__

#pragma pack(push, 1)

typedef struct {
	char  dtg_model[20];		// model no
	char  vehicle_id_num[17];			// car no : 順대번獒
	char  vehicle_type[2];			// car type : 順량晳梧
	char  regist_num[12];		// car regist no : 猩동順 등뤄번獒
	char  business_num[10];	// company regist no : 사업猩등뤄번獒
	char  driver_code[18];		// driver code : 운盛猩 掖드
	char seperator;
}__attribute__((packed))tacom_loop_hdr_t;

typedef struct {
	char  day_run_dist[4];		//1일주행거리
	char  acumul_run_dist[7];  //누적주행거리
	char  date_time[14];       //정보발생일시
	char  speed[3];            //속도
	char  rpm[4];              //RPM
	char  bs;               //브레이크신호
	char  gps_x[9];            //차량위치 (X)
	char  gps_y[9];            //차량위치 (Y)
	char  azimuth[3];          //방위각	
	char  accelation_x[6];     //가속도 (Vx)
	char  accelation_y[6];     //가속도 (Vy)
	char  status[2];           //상태코드
	                                                
	char  day_oil_usage[7];        //유류사용량	
	char  cumul_oil_usage[10];        //유류사용량	
	char  temper_a[4];                              
	char  temper_b[4];                              
	char seperator;
}__attribute__((packed))tacom_loop_data_t;

#pragma pack(pop)

/* value of expression 'MAX_LOOP_DATA * MAX_LOOP_DATA_PACK' 
 * have to set over loop_setup.max_records_per_once's value.
 */
#define MAX_LOOP_DATA		10
#ifdef SERVER_MODEL_GTRS
	#define MAX_LOOP_DATA_PACK	600		/* 200 min */
#else
	#define MAX_LOOP_DATA_PACK	900	/* 200 min */
#endif

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_loop_data_t buf[MAX_LOOP_DATA];
}loop_data_pack_t;

extern const struct tm_ops loop_ops;
extern struct tacom_setup loop_setup;

#endif //__DTG_LOOP_ASCII_DATA_FORMAT_DEFINE_HEADER__

