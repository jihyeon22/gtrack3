#ifndef __SECO_OBD_1_H__
#define __SECO_OBD_1_H__

#define OBD_RET_SUCCESS 			0
#define OBD_RET_FAIL				-1
#define OBD_CMD_RET_ERROR			-2
#define OBD_CMD_UART_INIT_FAIL		-3
#define OBD_CMD_RET_CHECK_SUM_FAIL	-4
#define OBD_CMD_RET_TIMEOUT			-5
#define OBD_CMD_RET_CMD_CNT_ERR		-6
#define OBD_CMD_RET_INVALID_COND	-999


int init_seco_obd_mgr(char* dev_name, int baud_rate, int (*p_bmsg_proc)(int argc, char* argv[]));

int get_seco_obd_1_serial(char* buff);
int get_seco_obd_1_ver(char* buff);
int get_seco_obd_1_fueltype(char* buff);

int start_seco_obd_1_broadcast_msg(int interval_sec, char* factor_list);
int stop_seco_obd_1_broadcast_msg();

#endif // __SECO_OBD_1_H__


