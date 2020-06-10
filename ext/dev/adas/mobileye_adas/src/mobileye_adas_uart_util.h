<<<<<<< HEAD
#ifndef __MOBILEYE_ADAS_UTIL_H__
#define __MOBILEYE_ADAS_UTIL_H__

//#define MOBILEYE_ADAS_DEV_PATH	g_mobileye_adas_dev_path_str

int mobileye_adas__uart_set_dev(char* dev_name);
int mobileye_adas__uart_set_baudrate(int baudrate);
int mobileye_adas__uart_get_fd();
int mobileye_adas__uart_init_stat();

int mobileye_adas__uart_init();
int mobileye_adas__uart_deinit();
int mobileye_adas__uart_chk();

int mobileye_adas__uart_wait_read(int fd, char *buf, int buf_len, int ftime);


#endif

=======
#ifndef __MOBILEYE_ADAS_UTIL_H__
#define __MOBILEYE_ADAS_UTIL_H__

//#define MOBILEYE_ADAS_DEV_PATH	g_mobileye_adas_dev_path_str

int mobileye_adas__uart_set_dev(char* dev_name);
int mobileye_adas__uart_set_baudrate(int baudrate);
int mobileye_adas__uart_get_fd();
int mobileye_adas__uart_init_stat();

int mobileye_adas__uart_init();
int mobileye_adas__uart_deinit();
int mobileye_adas__uart_chk();

int mobileye_adas__uart_wait_read(int fd, char *buf, int buf_len, int ftime);


#endif

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
