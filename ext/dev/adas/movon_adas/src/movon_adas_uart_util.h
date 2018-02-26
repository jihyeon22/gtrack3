#ifndef __MOVON_ADAS_UTIL_H__
#define __MOVON_ADAS_UTIL_H__

//#define MOVON_ADAS_DEV_PATH	g_movon_adas_dev_path_str

int movon_adas__uart_set_dev(char* dev_name);
int movon_adas__uart_set_baudrate(int baudrate);
int movon_adas__uart_get_fd();
int movon_adas__uart_init_stat();

int movon_adas__uart_init();
int movon_adas__uart_deinit();
int movon_adas__uart_chk();

int movon_adas__uart_wait_read(int fd, char *buf, int buf_len, int ftime);


#endif

