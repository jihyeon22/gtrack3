<<<<<<< HEAD
#ifndef __KATECH_OBD_H__
#define __KATECH_OBD_H__

#define MAX_OBD_UART_INIT_TRY_CNT	3

#define OBD_RET_SUCCESS 			0
#define OBD_RET_FAIL				-1
#define OBD_CMD_RET_ERROR			-2
#define OBD_CMD_UART_INIT_FAIL		-3
#define OBD_CMD_RET_CHECK_SUM_FAIL	-4
#define OBD_CMD_RET_TIMEOUT			-5
#define OBD_CMD_RET_INVALID_COND	-999


#define OBD_ERROR_CODE__DEFAULT_ERR				0
#define OBD_ERROR_CODE__PACKET_TYPE_ERR			1
#define OBD_ERROR_CODE__UNKOWN_CMD				2
#define OBD_ERROR_CODE__PKT_CHECKSUM_ERR		3
#define OBD_ERROR_CODE__UNSUPPORT_FUNC			4
#define OBD_ERROR_CODE__NOT_INIT_VECHICLE		100
#define OBD_ERROR_CODE__NOT_INIT_OBD			101
#define OBD_ERROR_CODE__DETECT_RESTART			102
#define OBD_ERROR_CODE__FIRM_UP_FAIL_CAR_ON		200

#define OBD_ERROR_CODE__NOT_TRANS_DATA_ADDR		300
#define OBD_ERROR_CODE__NOT_TRANS_DATA			400
#define OBD_ERROR_CODE__FLASH_ERASE				500
#define OBD_ERROR_CODE__SET_GENDER_SPEC			600
#define OBD_ERROR_CODE__GET_GENDER_SPEC			700
#define OBD_ERROR_CODE__GET_TIME				800
#define OBD_ERROR_CODE__GET_DTC_CODE			900

#define OBD_ERROR_CODE__SUCCESS					-99
#define OBD_ERROR_CODE__UNKOWN_CODE				-1
#define OBD_ERROR_CODE__NOT_VAILD_CMD_RET		-2
#define OBD_ERROR_CODE__UART_READ_TIMEOUT		-3
#define OBD_ERROR_CODE__NO_DATA_RET				-4

#define MAX_RET_BUFF_SIZE 1024

int is_katech_obd_init();
int chk_obd_port();
int get_obd_info();
int get_obd_info();

int get_obd_data_cnt(int* data1_cnt, int* data2_cnt);

int get_obd_data_1(unsigned char obd_buff[200]);
int delete_obd_data_1();

int get_obd_data_2(unsigned char obd_buff[100]);
int delete_obd_data_2();


#endif

=======
#ifndef __KATECH_OBD_H__
#define __KATECH_OBD_H__

#define MAX_OBD_UART_INIT_TRY_CNT	3

#define OBD_RET_SUCCESS 			0
#define OBD_RET_FAIL				-1
#define OBD_CMD_RET_ERROR			-2
#define OBD_CMD_UART_INIT_FAIL		-3
#define OBD_CMD_RET_CHECK_SUM_FAIL	-4
#define OBD_CMD_RET_TIMEOUT			-5
#define OBD_CMD_RET_INVALID_COND	-999


#define OBD_ERROR_CODE__DEFAULT_ERR				0
#define OBD_ERROR_CODE__PACKET_TYPE_ERR			1
#define OBD_ERROR_CODE__UNKOWN_CMD				2
#define OBD_ERROR_CODE__PKT_CHECKSUM_ERR		3
#define OBD_ERROR_CODE__UNSUPPORT_FUNC			4
#define OBD_ERROR_CODE__NOT_INIT_VECHICLE		100
#define OBD_ERROR_CODE__NOT_INIT_OBD			101
#define OBD_ERROR_CODE__DETECT_RESTART			102
#define OBD_ERROR_CODE__FIRM_UP_FAIL_CAR_ON		200

#define OBD_ERROR_CODE__NOT_TRANS_DATA_ADDR		300
#define OBD_ERROR_CODE__NOT_TRANS_DATA			400
#define OBD_ERROR_CODE__FLASH_ERASE				500
#define OBD_ERROR_CODE__SET_GENDER_SPEC			600
#define OBD_ERROR_CODE__GET_GENDER_SPEC			700
#define OBD_ERROR_CODE__GET_TIME				800
#define OBD_ERROR_CODE__GET_DTC_CODE			900

#define OBD_ERROR_CODE__SUCCESS					-99
#define OBD_ERROR_CODE__UNKOWN_CODE				-1
#define OBD_ERROR_CODE__NOT_VAILD_CMD_RET		-2
#define OBD_ERROR_CODE__UART_READ_TIMEOUT		-3
#define OBD_ERROR_CODE__NO_DATA_RET				-4

#define MAX_RET_BUFF_SIZE 1024

int is_katech_obd_init();
int chk_obd_port();
int get_obd_info();
int get_obd_info();

int get_obd_data_cnt(int* data1_cnt, int* data2_cnt);

int get_obd_data_1(unsigned char obd_buff[200]);
int delete_obd_data_1();

int get_obd_data_2(unsigned char obd_buff[100]);
int delete_obd_data_2();


#endif

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
