<<<<<<< HEAD
#ifndef __CS_MOTION_CMD_H__
#define __CS_MOTION_CMD_H__

#define CS_MOTION_DEV_DEFAULT_PATH          "/dev/ttyHSL2"
#define CS_MOTION_DEV_DEFAULT_BAUDRATE      9600

#define CS_MOTION_RET_SUCCESS                0
#define CS_MOTION_RET_FAIL                   -1
#define CS_MOTION_CMD_RET_ERROR              -2
#define CS_MOTION_CMD_UART_INIT_FAIL         -3
#define CS_MOTION_CMD_RET_CHECK_SUM_FAIL     -4
#define CS_MOTION_CMD_RET_TIMEOUT            -5
#define CS_MOTION_CMD_RET_INVALID_COND       -999

#define MAX_CS_MOTION_RET_BUFF_SIZE     1024

#define MAX_READ_CS_MOTION_UART_INVAILD_CHK  30

int cs_motion__mgr_init(char* dev_name, int baud_rate);

int cs_motion__cmd_broadcast_msg_start();
int cs_motion__cmd_broadcast_msg_stop();


int cs_motion__uart_set_dev(char* dev_name);
int cs_motion__uart_set_baudrate(int baudrate);
int cs_motion__uart_get_fd();
int cs_motion__uart_init_stat();

int cs_motion__uart_init();
int cs_motion__uart_deinit();
int cs_motion__uart_chk();

int cs_motion__uart_wait_read(int fd, char *buf, int buf_len, int ftime);



#endif // __CS_MOTION_CMD_H__
=======
#ifndef __CS_MOTION_CMD_H__
#define __CS_MOTION_CMD_H__

#define CS_MOTION_DEV_DEFAULT_PATH          "/dev/ttyHSL2"
#define CS_MOTION_DEV_DEFAULT_BAUDRATE      9600

#define CS_MOTION_RET_SUCCESS                0
#define CS_MOTION_RET_FAIL                   -1
#define CS_MOTION_CMD_RET_ERROR              -2
#define CS_MOTION_CMD_UART_INIT_FAIL         -3
#define CS_MOTION_CMD_RET_CHECK_SUM_FAIL     -4
#define CS_MOTION_CMD_RET_TIMEOUT            -5
#define CS_MOTION_CMD_RET_INVALID_COND       -999

#define MAX_CS_MOTION_RET_BUFF_SIZE     1024

#define MAX_READ_CS_MOTION_UART_INVAILD_CHK  30

int cs_motion__mgr_init(char* dev_name, int baud_rate);

int cs_motion__cmd_broadcast_msg_start();
int cs_motion__cmd_broadcast_msg_stop();


int cs_motion__uart_set_dev(char* dev_name);
int cs_motion__uart_set_baudrate(int baudrate);
int cs_motion__uart_get_fd();
int cs_motion__uart_init_stat();

int cs_motion__uart_init();
int cs_motion__uart_deinit();
int cs_motion__uart_chk();

int cs_motion__uart_wait_read(int fd, char *buf, int buf_len, int ftime);



#endif // __CS_MOTION_CMD_H__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
