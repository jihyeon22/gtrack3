#ifndef __MOBILEYE_ADAS_1_MGR_H__
#define __MOBILEYE_ADAS_1_MGR_H__

void mobileye_adas__mgr_mutex_lock();
void mobileye_adas__mgr_mutex_unlock();

#define MOBILEYE_ADAS_INVAILD_FD              -44
#define MOBILEYE_ADAS_UART_BUFF_SIZE          1024
#define MOBILEYE_ADAS_UART_READ_TIMEOUT_SEC   3
#define MOBILEYE_ADAS_UART_WRITE_CMD_SIZE     512
#define MOBILEYE_ADAS_UART_INIT_TRY_CNT       3

#define MOBILEYE_ADAS_UART_READ_THREAD_TIMEOUT 1

#define MOBILEYE_ADAS_UART_BROAD_CAST_CHK_MSG "$ME"

#endif
