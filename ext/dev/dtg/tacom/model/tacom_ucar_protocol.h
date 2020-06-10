#ifndef __DTG_UCAR_DATA_FORMAT_DEFINE_HEADER__
#define __DTG_UCAR_DATA_FORMAT_DEFINE_HEADER__

#pragma pack(push, 1)
////////////단말정보//////////////
typedef struct {
	char  start_mark;
	char  data_id[4];
	char  dtg_model[20];
	char  vehicle_id_num[17];
	char  vehicle_type[2];
	char  regist_num[12];
	char  business_num[10];
	char  driver_code[18];
	char crc[4];
	char end_mark[3];
}__attribute__((packed))tacom_ucar_hdr_t;

typedef struct {
	char  start_mark;
	char  data_id[4];
	char  day_run_dist[4];
	char  acumul_run_dist[7];
	char  date_time[14];
	char  speed[3];
	char  rpm[4];
	char  bs;
	char  gps_x[9];
	char  gps_y[9];
	char  azimuth[3];
	char  accelation_x[6];
	char  accelation_y[6];
	char  status[2];
	                                                
	char  day_oil_usage[7];
	char  cumul_oil_usage[10];
	char  temper_a[5];
	char  temper_b[5];
	char  residual_oil[3];
	char crc[4];
	char end_mark[3];
}__attribute__((packed))tacom_ucar_data_t;

/* value of expression 'MAX_UCAR_DATA * MAX_UCAR_DATA_PACK' 
 * have to set over ucar_setup.max_records_per_once's value.
 */
#define MAX_UCAR_DATA		10
#define MAX_UCAR_DATA_PACK	1200

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_ucar_data_t buf[MAX_UCAR_DATA];
}ucar_data_pack_t;

extern const struct tm_ops ucar_ops;
extern struct tacom_setup ucar_setup;

#pragma pack(pop)

#endif //__DTG_UCAR_DATA_FORMAT_DEFINE_HEADER__

