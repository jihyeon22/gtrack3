#ifndef __DTG_LOOP_ASCII_DATA_FORMAT_DEFINE_HEADER__
#define __DTG_LOOP_ASCII_DATA_FORMAT_DEFINE_HEADER__

#pragma pack(push, 1)

typedef struct {
	char  dtg_model[20];		// model no
	char  vehicle_id_num[17];			// car no : ��������
	char  vehicle_type[2];			// car type : ��������
	char  regist_num[12];		// car regist no : ������ ��������
	char  business_num[10];	// company regist no : ��������������
	char  driver_code[18];		// driver code : ������ ����
	char seperator;
}__attribute__((packed))tacom_loop_hdr_t;

typedef struct {
	char  day_run_dist[4];		//1�������Ÿ�
	char  acumul_run_dist[7];  //���������Ÿ�
	char  date_time[14];       //�����߻��Ͻ�
	char  speed[3];            //�ӵ�
	char  rpm[4];              //RPM
	char  bs;               //�극��ũ��ȣ
	char  gps_x[9];            //������ġ (X)
	char  gps_y[9];            //������ġ (Y)
	char  azimuth[3];          //������	
	char  accelation_x[6];     //���ӵ� (Vx)
	char  accelation_y[6];     //���ӵ� (Vy)
	char  status[2];           //�����ڵ�
	                                                
	char  day_oil_usage[7];        //�������뷮	
	char  cumul_oil_usage[10];        //�������뷮	
	char  temper_a[4];                              
	char  temper_b[4];                              
	char seperator;
}__attribute__((packed))tacom_loop_data_t;

#pragma pack(pop)

/* value of expression 'MAX_LOOP_DATA * MAX_LOOP_DATA_PACK' 
 * have to set over loop_setup.max_records_per_once's value.
 */
#define MAX_LOOP_DATA		10
#if defined(SERVER_MODEL_GTRS) || defined (SERVER_MODEL_NEOGNP)
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

