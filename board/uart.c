#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>

#include <board/uart.h>
#include <logd_rpc.h>

int init_uart(const char* dev, const int baud)
{
	struct termios newtio;
	int fd = 0;
	
	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if(fd < 0) {
		LOGE(eSVC_BASE, "%s> uart dev '%s' open fail [%d]\n", __func__, dev, fd);
		return -1;
	}

	int val;
	val = fcntl(fd, F_GETFD, 0);
	val |= FD_CLOEXEC;
	fcntl(fd, F_SETFD, val);

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR; // non-parity
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD; // NO-rts/cts

	switch(baud)
	{
		case 115200 :
			newtio.c_cflag |= B115200;
			break;
		case 57600 :
			newtio.c_cflag |= B57600;
			break;
		case 38400 :
			newtio.c_cflag |= B38400;
			break;
		case 19200 :
			newtio.c_cflag |= B19200;
			break;
		case 9600 :
			newtio.c_cflag |= B9600;
			break;
		case 4800 :
			newtio.c_cflag |= B4800;
			break;
		case 2400 :
			newtio.c_cflag |= B2400;
			break;
		default :
			newtio.c_cflag |= B115200;
			break;
	}

	newtio.c_lflag = 0;
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1초 단위
	//newtio.c_cc[VMIN] = vmin; // 최소 n 문자 받을 때까진 대기
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
	
	return fd;
}

/*
int uart_flush(int fd, int ftimes, int ftimeu)
{
	fd_set reads, temps;
	struct timeval tout;
	int result;
	char ch;

	FD_ZERO(&reads);
	
	while (1) 
	{
		FD_SET(fd, &reads);
		temps = reads;
		tout.tv_sec = ftimes;
		tout.tv_usec = ftimeu;

		result = select(fd + 1, &temps, 0, 0, &tout);
		if (result == -1) {
			return result;
		}
		else if (result == 0) {
			break;
		}
		else
		{
			read(fd, &ch, 1);
		}
	}
}
*/

int uart_check(int fd, int ftime, int retry)
{
	fd_set reads, temps;
	struct timeval tout;
	int result = 0;
	char ch;
	int nread = 0;

	FD_ZERO(&reads);
	
	while (retry > 0) {
		retry--;
		FD_SET(fd, &reads);
		temps = reads;
		tout.tv_sec = ftime;
		tout.tv_usec = 0;

		result = select(fd + 1, &temps, 0, 0, &tout);
		if (result == -1) {
			break;
		}
		else if (result == 0) {
			break;
		} else {
			if(read(fd, &ch, 1) > 0)
				nread += 1;
		}
	}

	if(nread > 1) {
		result = 1;
	}
	return result;
}

//int uart_size_read(int fd, void *buf, size_t nbytes, int ftime)
int uart_size_read(int fd, unsigned char *buf, size_t nbytes, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;
	int idx;
	int nreadByte;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	idx = 0;
	nreadByte = nbytes;
	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if (result == -1) {
			LOGE(eSVC_BASE, "%s : %s : select error\n", __FILE__, __func__);
			break;
		} else if (result == 0) {
			result = idx;
			break;
		} else {
			//result = read(fd, (unsigned char *)&buf[idx], nreadByte);
			result = read(fd, &buf[idx], nreadByte);
			if(result < 0) {
				result = idx;
				break;
			} else {
				idx += result;
				nreadByte -= result;
				if(nreadByte <= 0){
					result = idx;
					break;
				}
			}
		}
	}

	return result;
}



//int uart_read(int fd, void *buf, size_t nbytes, int ftime)
int uart_read(int fd, unsigned char *buf, size_t nbytes, int ftime)
{
	fd_set reads;
	struct timeval tout;
	
	int result = 0;
	int nreadByte;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	nreadByte = nbytes;
	
	tout.tv_sec = ftime;
	tout.tv_usec = 0;
	
	result = select(fd + 1, &reads, 0, 0, &tout);
	
	if (result == -1) 
	{
		LOGE(eSVC_BASE, "%s : %s : select error\n", __FILE__, __func__);
	//	break;
	}
	else
	{
		//result = read(fd, (unsigned char *)buf, nreadByte);
		result = read(fd, buf, nreadByte);
	}
	
	//printf("uart_read ret [%d]\r\n",

	return result;
}


int uart_write(int fd, const void *buf, size_t nbytes)
{
	int result;

	result = write(fd, buf, nbytes);
	fflush( NULL );
	
//	printf("write uart [%s]\r\n",buf);
	return result;
}

void uart_close(int fd)
{
	close(fd);
}


