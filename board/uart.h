#ifndef __BOARD_UART_H__
#define __BOARD_UART_H__

int init_uart(const char* dev, const int baud);
void uart_flush(int fd, int ftimes, int ftimeu);
int uart_check(int fd, int ftime, int retry);
//int uart_size_read(int fd, void *buf, size_t nbytes, int ftime);
int uart_size_read(int fd, unsigned char *buf, size_t nbytes, int ftime);
//int uart_read(int fd, void *buf, size_t nbytes, int ftime);
int uart_read(int fd, unsigned char *buf, size_t nbytes, int ftime);
int uart_write(int fd, const void *buf, size_t nbytes);
void uart_close(int fd);

#endif

