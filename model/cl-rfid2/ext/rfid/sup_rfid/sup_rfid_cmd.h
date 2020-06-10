#ifndef __SUP_RFID_CMD_H__
#define __SUP_RFID_CMD_H__

#define SUP_RFID_DEV_DEFAULT_PATH          "/dev/ttyHSL1"
#define SUP_RFID_DEV_DEFAULT_BAUDRATE      9600

#define SUP_RFID_RET_SUCCESS                0
#define SUP_RFID_RET_FAIL                   -1
#define SUP_RFID_CMD_RET_ERROR              -2
#define SUP_RFID_CMD_UART_INIT_FAIL         -3
#define SUP_RFID_CMD_RET_CHECK_SUM_FAIL     -4
#define SUP_RFID_CMD_RET_TIMEOUT            -5
#define SUP_RFID_CMD_RET_INVALID_COND       -999

#define MAX_SUP_RFID_RET_BUFF_SIZE     1024

#define MAX_READ_SUP_RFID_UART_INVAILD_CHK  30

int sup_rfid__mgr_init(char* dev_name, int baud_rate);

int sup_rfid__cmd_broadcast_msg_start();
int sup_rfid__cmd_broadcast_msg_stop();


int sup_rfid__uart_set_dev(char* dev_name);
int sup_rfid__uart_set_baudrate(int baudrate);
int sup_rfid__uart_get_fd();
int sup_rfid__uart_init_stat();

int sup_rfid__uart_init();
int sup_rfid__uart_deinit();
int sup_rfid__uart_chk();

int sup_rfid__uart_wait_read(int fd, char *buf, int buf_len, int ftime);



#endif // __SUP_RFID_CMD_H__
