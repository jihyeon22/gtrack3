<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "cs_motion_tools.h"
#include "cs_motion_cmd.h"


// ------------------------------------------
// settinngs..
// ------------------------------------------
int g_cs_motion_fd = CS_MOTION_INVAILD_FD;

#define DEBUG_MSG_CS_MOTION_UART

static char g_cs_motion_dev_path_str[64] = {0,};
static int g_cs_motion_dev_baudrate = 0;

static int _init_cs_motion_uart(char* dev, int baud , int *fd)
{
	struct termios newtio;
	*fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if(*fd < 0) {
		printf("%s> uart dev '%s' open fail [%d]\n", __func__, dev, *fd);
		return -1;
	}

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
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1?? ????
	//newtio.c_cc[VMIN] = vmin; // ??? n ???? ???? ?????? ????
	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 0;
	tcflush(*fd, TCIFLUSH);
	tcsetattr(*fd, TCSANOW, &newtio);
    
	return 0;
}

int cs_motion__uart_set_dev(char* dev_name)
{
    if ( ( dev_name != NULL ) && ( strlen(dev_name) > 0 ) )
        strcpy(g_cs_motion_dev_path_str, dev_name);
    else
        strcpy(g_cs_motion_dev_path_str, CS_MOTION_DEV_DEFAULT_PATH);

    return 0;
}

int cs_motion__uart_set_baudrate(int baudrate)
{
    if ( baudrate > 0 )
        g_cs_motion_dev_baudrate = baudrate;
    else
        g_cs_motion_dev_baudrate = CS_MOTION_DEV_DEFAULT_BAUDRATE;
    
    return 0;
}


int cs_motion__uart_init()
{
	int ret = 0;
	
	if ( g_cs_motion_fd <= 0 )
	{
		char tmp_cs_motion_dev_path_str[64] = {0,};
		int tmp_cs_motion_dev_baudrate = 0;

		if ( strlen(g_cs_motion_dev_path_str) > 0 )
			strcpy(tmp_cs_motion_dev_path_str, g_cs_motion_dev_path_str);
		else
			strcpy(tmp_cs_motion_dev_path_str, CS_MOTION_DEV_DEFAULT_PATH);

		if ( g_cs_motion_dev_baudrate == 0 )
			tmp_cs_motion_dev_baudrate = g_cs_motion_dev_baudrate;
		else
			tmp_cs_motion_dev_baudrate = CS_MOTION_DEV_DEFAULT_BAUDRATE;
		
		ret = _init_cs_motion_uart(tmp_cs_motion_dev_path_str, tmp_cs_motion_dev_baudrate, &g_cs_motion_fd);
	}
	else
	{
		return CS_MOTION_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_cs_motion_fd = CS_MOTION_INVAILD_FD;
		return CS_MOTION_RET_FAIL;
	}
	
	printf("init cs_motion uart [%d], [%d]\r\n",ret, g_cs_motion_fd);
	
	return CS_MOTION_RET_SUCCESS;
}


int cs_motion__uart_deinit()
{
	if (g_cs_motion_fd > 0)
		close(g_cs_motion_fd);
	
	g_cs_motion_fd = CS_MOTION_INVAILD_FD;
	return 0;
}

int cs_motion__uart_get_fd()
{
	return g_cs_motion_fd;
}


int cs_motion__uart_init_stat()
{
	if ( g_cs_motion_fd <= 0)
	{
		return CS_MOTION_RET_FAIL;
	}
	else
	{
		return CS_MOTION_RET_SUCCESS;
	}
}


int cs_motion__uart_chk()
{
    int i = 0;
    
	for(i = 0; i < CS_MOTION_UART_INIT_TRY_CNT ; i++)
	{
		if (cs_motion__uart_init() == CS_MOTION_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (cs_motion__uart_init_stat() == CS_MOTION_RET_FAIL)
	{
		return CS_MOTION_CMD_UART_INIT_FAIL;
	}

    return CS_MOTION_RET_SUCCESS;
}

int cs_motion__uart_wait_read(int fd, char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;

	int read_cnt = 0;
	
	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return -1;
		
		read_cnt = read(fd, buf, buf_len);
		
		if ( read_cnt <= 0)
			return -1;

		break; //success
	}

	return read_cnt;
}

=======
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "cs_motion_tools.h"
#include "cs_motion_cmd.h"


// ------------------------------------------
// settinngs..
// ------------------------------------------
int g_cs_motion_fd = CS_MOTION_INVAILD_FD;

#define DEBUG_MSG_CS_MOTION_UART

static char g_cs_motion_dev_path_str[64] = {0,};
static int g_cs_motion_dev_baudrate = 0;

static int _init_cs_motion_uart(char* dev, int baud , int *fd)
{
	struct termios newtio;
	*fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);

	if(*fd < 0) {
		printf("%s> uart dev '%s' open fail [%d]\n", __func__, dev, *fd);
		return -1;
	}

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
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1?? ????
	//newtio.c_cc[VMIN] = vmin; // ??? n ???? ???? ?????? ????
	newtio.c_cc[VTIME] = 1;
	newtio.c_cc[VMIN] = 0;
	tcflush(*fd, TCIFLUSH);
	tcsetattr(*fd, TCSANOW, &newtio);
    
	return 0;
}

int cs_motion__uart_set_dev(char* dev_name)
{
    if ( ( dev_name != NULL ) && ( strlen(dev_name) > 0 ) )
        strcpy(g_cs_motion_dev_path_str, dev_name);
    else
        strcpy(g_cs_motion_dev_path_str, CS_MOTION_DEV_DEFAULT_PATH);

    return 0;
}

int cs_motion__uart_set_baudrate(int baudrate)
{
    if ( baudrate > 0 )
        g_cs_motion_dev_baudrate = baudrate;
    else
        g_cs_motion_dev_baudrate = CS_MOTION_DEV_DEFAULT_BAUDRATE;
    
    return 0;
}


int cs_motion__uart_init()
{
	int ret = 0;
	
	if ( g_cs_motion_fd <= 0 )
	{
		char tmp_cs_motion_dev_path_str[64] = {0,};
		int tmp_cs_motion_dev_baudrate = 0;

		if ( strlen(g_cs_motion_dev_path_str) > 0 )
			strcpy(tmp_cs_motion_dev_path_str, g_cs_motion_dev_path_str);
		else
			strcpy(tmp_cs_motion_dev_path_str, CS_MOTION_DEV_DEFAULT_PATH);

		if ( g_cs_motion_dev_baudrate == 0 )
			tmp_cs_motion_dev_baudrate = g_cs_motion_dev_baudrate;
		else
			tmp_cs_motion_dev_baudrate = CS_MOTION_DEV_DEFAULT_BAUDRATE;
		
		ret = _init_cs_motion_uart(tmp_cs_motion_dev_path_str, tmp_cs_motion_dev_baudrate, &g_cs_motion_fd);
	}
	else
	{
		return CS_MOTION_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_cs_motion_fd = CS_MOTION_INVAILD_FD;
		return CS_MOTION_RET_FAIL;
	}
	
	printf("init cs_motion uart [%d], [%d]\r\n",ret, g_cs_motion_fd);
	
	return CS_MOTION_RET_SUCCESS;
}


int cs_motion__uart_deinit()
{
	if (g_cs_motion_fd > 0)
		close(g_cs_motion_fd);
	
	g_cs_motion_fd = CS_MOTION_INVAILD_FD;
	return 0;
}

int cs_motion__uart_get_fd()
{
	return g_cs_motion_fd;
}


int cs_motion__uart_init_stat()
{
	if ( g_cs_motion_fd <= 0)
	{
		return CS_MOTION_RET_FAIL;
	}
	else
	{
		return CS_MOTION_RET_SUCCESS;
	}
}


int cs_motion__uart_chk()
{
    int i = 0;
    
	for(i = 0; i < CS_MOTION_UART_INIT_TRY_CNT ; i++)
	{
		if (cs_motion__uart_init() == CS_MOTION_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (cs_motion__uart_init_stat() == CS_MOTION_RET_FAIL)
	{
		return CS_MOTION_CMD_UART_INIT_FAIL;
	}

    return CS_MOTION_RET_SUCCESS;
}

int cs_motion__uart_wait_read(int fd, char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;

	int read_cnt = 0;
	
	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return -1;
		
		read_cnt = read(fd, buf, buf_len);
		
		if ( read_cnt <= 0)
			return -1;

		break; //success
	}

	return read_cnt;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
