<<<<<<< HEAD
#ifndef __SECO_OBD_1_H__
#define __SECO_OBD_1_H__

#define MAX_RETRY_GET_TRIP_DATA_FACTOR  3

#define OBD_RET_SUCCESS 			0
#define OBD_RET_FAIL				-1
#define OBD_CMD_RET_ERROR			-2
#define OBD_CMD_UART_INIT_FAIL		-3
#define OBD_CMD_RET_CHECK_SUM_FAIL	-4
#define OBD_CMD_RET_TIMEOUT			-5
#define OBD_CMD_RET_CMD_CNT_ERR		-6
#define OBD_CMD_RET_INVALID_COND	-999

typedef struct seco_cmd_data_bulk_type
{
    int idx;
    float data;
}__attribute__((packed))SECO_CMD_DATA_BULK_TYPE_T;

#define OBD_CMD_SRR_TA1_MAX_CNT     22

// TA1 COMMAND
typedef enum {
    eOBD_CMD_SRR_TA1_CLV = 0, // 01
    eOBD_CMD_SRR_TA1_MAP    , // 02
    eOBD_CMD_SRR_TA1_RPM    , // 03
    eOBD_CMD_SRR_TA1_SPD    , // 04
	eOBD_CMD_SRR_TA1_MAF    , // 05
	eOBD_CMD_SRR_TA1_TPA    , // 06
	eOBD_CMD_SRR_TA1_BS1    , // 07
    eOBD_CMD_SRR_TA1_BAV    , // 08
    eOBD_CMD_SRR_TA1_APD    , // 09
    eOBD_CMD_SRR_TA1_EFR    , // 10
    eOBD_CMD_SRR_TA1_EAT    , // 11
    eOBD_CMD_SRR_TA1_CED    , // 12
    eOBD_CMD_SRR_TA1_AED    , // 13
    eOBD_CMD_SRR_TA1_EFT    , // 14
    eOBD_CMD_SRR_TA1_COT    , // 15
    eOBD_CMD_SRR_TA1_ATS    , // 16
    eOBD_CMD_SRR_TA1_TB1    , // 17
    eOBD_CMD_SRR_TA1_EGR    , // 18
    eOBD_CMD_SRR_TA1_EGE    , // 19
    eOBD_CMD_SRR_TA1_EST    , // 20
    eOBD_CMD_SRR_TA1_BRO    , // 21
    eOBD_CMD_SRR_TA1_ABT    , // 22
    eOBD_CMD_SRR_TA1_MAX_CNT
} SECO_CMD_SRR_TA1_IDX;

typedef struct seco_cmd_data_srr_ta1
{
    SECO_CMD_DATA_BULK_TYPE_T obd_data[OBD_CMD_SRR_TA1_MAX_CNT];
}__attribute__((packed))SECO_CMD_DATA_SRR_TA1_T;

// --------------------------------------

#define OBD_CMD_SRR_TA2_MAX_CNT     4
typedef enum {
    eOBD_CMD_SRR_TA2_ERT = 0 , // 01
    eOBD_CMD_SRR_TA2_DTC     , // 02
    eOBD_CMD_SRR_TA2_DTM     , // 03
    eOBD_CMD_SRR_TA2_DTS     , // 04
    eOBD_CMD_SRR_TA2_MAX_CNT
} SECO_CMD_SRR_TA2_IDX;


typedef struct seco_cmd_data_srr_ta2
{
    SECO_CMD_DATA_BULK_TYPE_T obd_data[OBD_CMD_SRR_TA2_MAX_CNT];
}__attribute__((packed))SECO_CMD_DATA_SRR_TA2_T;

typedef struct seco_cmd_data_vehicle_info
{
    char car_maker[128];
    //int  car_year_of_production;
    char car_gas_type[64];
    int  car_cc;
    int  car_cylinder;
    //char  car_nick_name[64];
    //char car_grade[64];
}__attribute__((packed))SECO_CMD_DATA_VEHICLE_INFO_T;


// ----------------------------------------

int init_seco_obd_mgr(char* dev_name, int baud_rate, int (*p_bmsg_proc)(int argc, char* argv[]));

int get_seco_obd_1_serial(char* buff);
int get_seco_obd_1_ver(char* buff);
int get_seco_obd_1_fueltype(char* buff);

int set_seco_obd_1_total_distance(int total_distance);

int start_seco_obd_1_broadcast_msg(int interval_sec, char* factor_list);
int stop_seco_obd_1_broadcast_msg();

int get_seco_obd_cmd_ta1(SECO_CMD_DATA_SRR_TA1_T* p_ta1_buff);
int get_seco_obd_cmd_ta2(SECO_CMD_DATA_SRR_TA2_T* p_ta2_buff);
int set_obd_auto_poweroff_sec(int sec);
int get_obd_total_distance();
int get_obd_total_fuel_usage();

int get_obd_vehicle_info(SECO_CMD_DATA_VEHICLE_INFO_T* info);
// ------------------------------------------------------



#endif // __SECO_OBD_1_H__


=======
#ifndef __SECO_OBD_1_H__
#define __SECO_OBD_1_H__

#define MAX_RETRY_GET_TRIP_DATA_FACTOR  3

#define OBD_RET_SUCCESS 			0
#define OBD_RET_FAIL				-1
#define OBD_CMD_RET_ERROR			-2
#define OBD_CMD_UART_INIT_FAIL		-3
#define OBD_CMD_RET_CHECK_SUM_FAIL	-4
#define OBD_CMD_RET_TIMEOUT			-5
#define OBD_CMD_RET_CMD_CNT_ERR		-6
#define OBD_CMD_RET_INVALID_COND	-999

typedef struct seco_cmd_data_bulk_type
{
    int idx;
    float data;
}__attribute__((packed))SECO_CMD_DATA_BULK_TYPE_T;

#define OBD_CMD_SRR_TA1_MAX_CNT     22

// TA1 COMMAND
typedef enum {
    eOBD_CMD_SRR_TA1_CLV = 0, // 01
    eOBD_CMD_SRR_TA1_MAP    , // 02
    eOBD_CMD_SRR_TA1_RPM    , // 03
    eOBD_CMD_SRR_TA1_SPD    , // 04
	eOBD_CMD_SRR_TA1_MAF    , // 05
	eOBD_CMD_SRR_TA1_TPA    , // 06
	eOBD_CMD_SRR_TA1_BS1    , // 07
    eOBD_CMD_SRR_TA1_BAV    , // 08
    eOBD_CMD_SRR_TA1_APD    , // 09
    eOBD_CMD_SRR_TA1_EFR    , // 10
    eOBD_CMD_SRR_TA1_EAT    , // 11
    eOBD_CMD_SRR_TA1_CED    , // 12
    eOBD_CMD_SRR_TA1_AED    , // 13
    eOBD_CMD_SRR_TA1_EFT    , // 14
    eOBD_CMD_SRR_TA1_COT    , // 15
    eOBD_CMD_SRR_TA1_ATS    , // 16
    eOBD_CMD_SRR_TA1_TB1    , // 17
    eOBD_CMD_SRR_TA1_EGR    , // 18
    eOBD_CMD_SRR_TA1_EGE    , // 19
    eOBD_CMD_SRR_TA1_EST    , // 20
    eOBD_CMD_SRR_TA1_BRO    , // 21
    eOBD_CMD_SRR_TA1_ABT    , // 22
    eOBD_CMD_SRR_TA1_MAX_CNT
} SECO_CMD_SRR_TA1_IDX;

typedef struct seco_cmd_data_srr_ta1
{
    SECO_CMD_DATA_BULK_TYPE_T obd_data[OBD_CMD_SRR_TA1_MAX_CNT];
}__attribute__((packed))SECO_CMD_DATA_SRR_TA1_T;

// --------------------------------------

#define OBD_CMD_SRR_TA2_MAX_CNT     4
typedef enum {
    eOBD_CMD_SRR_TA2_ERT = 0 , // 01
    eOBD_CMD_SRR_TA2_DTC     , // 02
    eOBD_CMD_SRR_TA2_DTM     , // 03
    eOBD_CMD_SRR_TA2_DTS     , // 04
    eOBD_CMD_SRR_TA2_MAX_CNT
} SECO_CMD_SRR_TA2_IDX;


typedef struct seco_cmd_data_srr_ta2
{
    SECO_CMD_DATA_BULK_TYPE_T obd_data[OBD_CMD_SRR_TA2_MAX_CNT];
}__attribute__((packed))SECO_CMD_DATA_SRR_TA2_T;

typedef struct seco_cmd_data_vehicle_info
{
    char car_maker[128];
    //int  car_year_of_production;
    char car_gas_type[64];
    int  car_cc;
    int  car_cylinder;
    //char  car_nick_name[64];
    //char car_grade[64];
}__attribute__((packed))SECO_CMD_DATA_VEHICLE_INFO_T;


// ----------------------------------------

int init_seco_obd_mgr(char* dev_name, int baud_rate, int (*p_bmsg_proc)(int argc, char* argv[]));

int get_seco_obd_1_serial(char* buff);
int get_seco_obd_1_ver(char* buff);
int get_seco_obd_1_fueltype(char* buff);

int set_seco_obd_1_total_distance(int total_distance);

int start_seco_obd_1_broadcast_msg(int interval_sec, char* factor_list);
int stop_seco_obd_1_broadcast_msg();

int get_seco_obd_cmd_ta1(SECO_CMD_DATA_SRR_TA1_T* p_ta1_buff);
int get_seco_obd_cmd_ta2(SECO_CMD_DATA_SRR_TA2_T* p_ta2_buff);
int set_obd_auto_poweroff_sec(int sec);
int get_obd_total_distance();
int get_obd_total_fuel_usage();

int get_obd_vehicle_info(SECO_CMD_DATA_VEHICLE_INFO_T* info);
// ------------------------------------------------------



#endif // __SECO_OBD_1_H__


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
