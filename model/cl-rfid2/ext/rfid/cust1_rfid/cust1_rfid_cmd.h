#ifndef __CUST1_RFID_CMD_H__
#define __CUST1_RFID_CMD_H__

#define CUST1_RFID_DEV_DEFAULT_PATH          "/dev/ttyHSL1"
#define CUST1_RFID_DEV_DEFAULT_BAUDRATE      9600

#define CUST1_RFID_RET_SUCCESS                0
#define CUST1_RFID_RET_FAIL                   -1
#define CUST1_RFID_CMD_RET_ERROR              -2
#define CUST1_RFID_CMD_UART_INIT_FAIL         -3
#define CUST1_RFID_CMD_RET_CHECK_SUM_FAIL     -4
#define CUST1_RFID_CMD_RET_TIMEOUT            -5
#define CUST1_RFID_CMD_RET_INVALID_COND       -999

#define MAX_CUST1_RFID_RET_BUFF_SIZE     1024

#define MAX_READ_CUST1_RFID_UART_INVAILD_CHK  30

int cust1_rfid__mgr_init(char* dev_name, int baud_rate);

int cust1_rfid__cmd_broadcast_msg_start();
int cust1_rfid__cmd_broadcast_msg_stop();


int cust1_rfid__uart_set_dev(char* dev_name);
int cust1_rfid__uart_set_baudrate(int baudrate);
int cust1_rfid__uart_get_fd();
int cust1_rfid__uart_init_stat();

int cust1_rfid__uart_init();
int cust1_rfid__uart_deinit();
int cust1_rfid__uart_chk();

int cust1_rfid__uart_wait_read(int fd, char *buf, int buf_len, int ftime);



#endif // __CUST1_RFID_CMD_H__
