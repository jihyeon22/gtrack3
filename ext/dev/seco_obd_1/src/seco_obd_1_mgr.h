#ifndef __SECO_OBD_1_MGR_H__
#define __SECO_OBD_1_MGR_H__

void seco_obd_1_mutex_lock();
void seco_obd_1_mutex_unlock();

#define SECO_OBD_INVAILD_FD		-44
#define OBD_UART_BUFF_SIZE 		1024
#define OBD_UART_WAIT_READ_SEC 	3
#define SECO_MAX_WRITE_CMD_SIZE	512
#define MAX_OBD_UART_INIT_TRY_CNT	3

#define SECO_OBD_1_READ_THREAD_TIMEOUT 1



#endif

