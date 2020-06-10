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

#include "mobileye_adas.h"
#include "mobileye_adas_mgr.h"
#include "mobileye_adas_protocol.h"
#include "mobileye_adas_tool.h"
#include "mobileye_adas_uart_util.h"


// ------------------------------------------
// settinngs..
// ------------------------------------------
int g_mobileye_adas_fd = MOBILEYE_ADAS_INVAILD_FD;

#define DEBUG_MSG_MOBILEYE_ADAS_UART

static char g_mobileye_adas_dev_path_str[64] = {0,};
static int g_mobileye_adas_dev_baudrate = 0;

static int _init_mobileye_adas_uart(char* dev, int baud , int *fd)
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

int mobileye_adas__uart_set_dev(char* dev_name)
{
    if ( ( dev_name != NULL ) && ( strlen(dev_name) > 0 ) )
        strcpy(g_mobileye_adas_dev_path_str, dev_name);
    else
        strcpy(g_mobileye_adas_dev_path_str, MOBILEYE_ADAS_DEV_DEFAULT_PATH);

    return 0;
}

int mobileye_adas__uart_set_baudrate(int baudrate)
{
    if ( baudrate > 0 )
        g_mobileye_adas_dev_baudrate = baudrate;
    else
        g_mobileye_adas_dev_baudrate = MOBILEYE_ADAS_DEV_DEFAULT_BAUDRATE;
    
    return 0;
}


int mobileye_adas__uart_init()
{
	int ret = 0;
	
	if ( g_mobileye_adas_fd <= 0 )
	{
		char tmp_mobileye_adas_dev_path_str[64] = {0,};
		int tmp_mobileye_adas_dev_baudrate = 0;

		if ( strlen(g_mobileye_adas_dev_path_str) > 0 )
			strcpy(tmp_mobileye_adas_dev_path_str, g_mobileye_adas_dev_path_str);
		else
			strcpy(tmp_mobileye_adas_dev_path_str, MOBILEYE_ADAS_DEV_DEFAULT_PATH);

		if ( g_mobileye_adas_dev_baudrate == 0 )
			tmp_mobileye_adas_dev_baudrate = g_mobileye_adas_dev_baudrate;
		else
			tmp_mobileye_adas_dev_baudrate = MOBILEYE_ADAS_DEV_DEFAULT_BAUDRATE;
		
		ret = _init_mobileye_adas_uart(tmp_mobileye_adas_dev_path_str, tmp_mobileye_adas_dev_baudrate, &g_mobileye_adas_fd);
	}
	else
	{
		return MOBILEYE_ADAS_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_mobileye_adas_fd = MOBILEYE_ADAS_INVAILD_FD;
		return MOBILEYE_ADAS_RET_FAIL;
	}
	
	printf("init mobileye_adas uart [%d], [%d]\r\n",ret, g_mobileye_adas_fd);
	
	return MOBILEYE_ADAS_RET_SUCCESS;
}


int mobileye_adas__uart_deinit()
{
	if (g_mobileye_adas_fd > 0)
		close(g_mobileye_adas_fd);
	
	g_mobileye_adas_fd = MOBILEYE_ADAS_INVAILD_FD;
	return 0;
}

int mobileye_adas__uart_get_fd()
{
	return g_mobileye_adas_fd;
}


int mobileye_adas__uart_init_stat()
{
	if ( g_mobileye_adas_fd <= 0)
	{
		return MOBILEYE_ADAS_RET_FAIL;
	}
	else
	{
		return MOBILEYE_ADAS_RET_SUCCESS;
	}
}


int mobileye_adas__uart_chk()
{
    int i = 0;
    
	for(i = 0; i < MOBILEYE_ADAS_UART_INIT_TRY_CNT ; i++)
	{
		if (mobileye_adas__uart_init() == MOBILEYE_ADAS_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (mobileye_adas__uart_init_stat() == MOBILEYE_ADAS_RET_FAIL)
	{
		return MOBILEYE_ADAS_CMD_UART_INIT_FAIL;
	}

    return MOBILEYE_ADAS_RET_SUCCESS;
}

int mobileye_adas__uart_wait_read(int fd, char *buf, int buf_len, int ftime)
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


#if 0
int mobileye_adas_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, char* ret_cmd, const int retry_cnt)
{
	int ret = MOBILEYE_ADAS_RET_FAIL;
	
	int write_cnt1 = 0;
	int write_cnt2 = 0;
	
	char tmp_buffer[MOBILEYE_ADAS_UART_BUFF_SIZE] = {0,};
	unsigned char result_buffer[MOBILEYE_ADAS_UART_BUFF_SIZE] = {0,};
	
	int write_cmd = 1;
	int retry = 0;
	
	int cur_read_cnt = 0;
	int total_read_cnt = 0;
	
//	char *p_ret_cmd = NULL;

//	int i = 0;
	
	if ( fd <= 0 )
		return MOBILEYE_ADAS_RET_FAIL;
	
	write_cnt1 = cmd_len;

	mobileye_adas__mgr_mutex_lock();
	// Ŀ�ǵ带 ������ ����.
	while(retry++ < retry_cnt) 
	{
		// 1. Ŀ�ǵ带 ������.
		if ( write_cmd )
		{
			
			//printf("<atd> dbg : > [%s] (%d)\r\n", write_buf, write_cnt1);
			
			write_cnt2 = write(fd, write_buf, write_cnt1);
		}
		
		if ( write_cnt2 != write_cnt1 )
		{
			
			printf("<atd> dbg : write fail.. retry (%d),(%d)\r\n",write_cnt2,write_cnt1);
			ret = MOBILEYE_ADAS_CMD_RET_TIMEOUT;
			continue;
		}
		
		write_cmd = 0;
		
		// 2. Ŀ�ǵ� ���ϵɶ� ���� ���ٸ�.
		memset(tmp_buffer, 0x00, sizeof(tmp_buffer));
		cur_read_cnt = mobileye_adas__uart_wait_read(fd, tmp_buffer, MOBILEYE_ADAS_UART_BUFF_SIZE, MOBILEYE_ADAS_UART_READ_TIMEOUT_SEC) ;
		
		if(cur_read_cnt < 0)
		{
			printf("<atd> dbg : read timeout fail.. retry\r\n");
			ret = MOBILEYE_ADAS_CMD_RET_TIMEOUT;
			continue;
		}
		
		
		memcpy(result_buffer + total_read_cnt, tmp_buffer, cur_read_cnt);
		total_read_cnt += cur_read_cnt;
		
		if (total_read_cnt < 2)
			continue;
		
		if ((result_buffer[total_read_cnt-1] == 0x0A) && (result_buffer[total_read_cnt-2] = 0x0D))
		{
			// �������� ������ ���������� �������� ã�Ҵ�.
			
			ret = total_read_cnt;
			
			/*
			printf("read success! =========================== \r\n");
			for(i = 0 ; i < total_read_cnt ; i++ )
			{	
				char ch = result_buffer[i];
				
				if ((i > 0) && ((i%10)==0))
					printf("\r\n");
				//printf("[0x%02x(%c)]",result_buffer[i], result_buffer[i]);
				if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'))
					printf("[0x%02x('%c')] ",result_buffer[i], result_buffer[i]);
				else
					printf("[0x%02x()] ",result_buffer[i]);
			}
			printf("========================================== \r\n");
			*/
			
			memcpy(ret_cmd, result_buffer, total_read_cnt-2);
			break;
		}
		
	}
	
	//close(fd);
	mobileye_adas__mgr_mutex_unlock();

	return ret;
}
#endif




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

#include "mobileye_adas.h"
#include "mobileye_adas_mgr.h"
#include "mobileye_adas_protocol.h"
#include "mobileye_adas_tool.h"
#include "mobileye_adas_uart_util.h"


// ------------------------------------------
// settinngs..
// ------------------------------------------
int g_mobileye_adas_fd = MOBILEYE_ADAS_INVAILD_FD;

#define DEBUG_MSG_MOBILEYE_ADAS_UART

static char g_mobileye_adas_dev_path_str[64] = {0,};
static int g_mobileye_adas_dev_baudrate = 0;

static int _init_mobileye_adas_uart(char* dev, int baud , int *fd)
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

int mobileye_adas__uart_set_dev(char* dev_name)
{
    if ( ( dev_name != NULL ) && ( strlen(dev_name) > 0 ) )
        strcpy(g_mobileye_adas_dev_path_str, dev_name);
    else
        strcpy(g_mobileye_adas_dev_path_str, MOBILEYE_ADAS_DEV_DEFAULT_PATH);

    return 0;
}

int mobileye_adas__uart_set_baudrate(int baudrate)
{
    if ( baudrate > 0 )
        g_mobileye_adas_dev_baudrate = baudrate;
    else
        g_mobileye_adas_dev_baudrate = MOBILEYE_ADAS_DEV_DEFAULT_BAUDRATE;
    
    return 0;
}


int mobileye_adas__uart_init()
{
	int ret = 0;
	
	if ( g_mobileye_adas_fd <= 0 )
	{
		char tmp_mobileye_adas_dev_path_str[64] = {0,};
		int tmp_mobileye_adas_dev_baudrate = 0;

		if ( strlen(g_mobileye_adas_dev_path_str) > 0 )
			strcpy(tmp_mobileye_adas_dev_path_str, g_mobileye_adas_dev_path_str);
		else
			strcpy(tmp_mobileye_adas_dev_path_str, MOBILEYE_ADAS_DEV_DEFAULT_PATH);

		if ( g_mobileye_adas_dev_baudrate == 0 )
			tmp_mobileye_adas_dev_baudrate = g_mobileye_adas_dev_baudrate;
		else
			tmp_mobileye_adas_dev_baudrate = MOBILEYE_ADAS_DEV_DEFAULT_BAUDRATE;
		
		ret = _init_mobileye_adas_uart(tmp_mobileye_adas_dev_path_str, tmp_mobileye_adas_dev_baudrate, &g_mobileye_adas_fd);
	}
	else
	{
		return MOBILEYE_ADAS_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_mobileye_adas_fd = MOBILEYE_ADAS_INVAILD_FD;
		return MOBILEYE_ADAS_RET_FAIL;
	}
	
	printf("init mobileye_adas uart [%d], [%d]\r\n",ret, g_mobileye_adas_fd);
	
	return MOBILEYE_ADAS_RET_SUCCESS;
}


int mobileye_adas__uart_deinit()
{
	if (g_mobileye_adas_fd > 0)
		close(g_mobileye_adas_fd);
	
	g_mobileye_adas_fd = MOBILEYE_ADAS_INVAILD_FD;
	return 0;
}

int mobileye_adas__uart_get_fd()
{
	return g_mobileye_adas_fd;
}


int mobileye_adas__uart_init_stat()
{
	if ( g_mobileye_adas_fd <= 0)
	{
		return MOBILEYE_ADAS_RET_FAIL;
	}
	else
	{
		return MOBILEYE_ADAS_RET_SUCCESS;
	}
}


int mobileye_adas__uart_chk()
{
    int i = 0;
    
	for(i = 0; i < MOBILEYE_ADAS_UART_INIT_TRY_CNT ; i++)
	{
		if (mobileye_adas__uart_init() == MOBILEYE_ADAS_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (mobileye_adas__uart_init_stat() == MOBILEYE_ADAS_RET_FAIL)
	{
		return MOBILEYE_ADAS_CMD_UART_INIT_FAIL;
	}

    return MOBILEYE_ADAS_RET_SUCCESS;
}

int mobileye_adas__uart_wait_read(int fd, char *buf, int buf_len, int ftime)
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


#if 0
int mobileye_adas_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, char* ret_cmd, const int retry_cnt)
{
	int ret = MOBILEYE_ADAS_RET_FAIL;
	
	int write_cnt1 = 0;
	int write_cnt2 = 0;
	
	char tmp_buffer[MOBILEYE_ADAS_UART_BUFF_SIZE] = {0,};
	unsigned char result_buffer[MOBILEYE_ADAS_UART_BUFF_SIZE] = {0,};
	
	int write_cmd = 1;
	int retry = 0;
	
	int cur_read_cnt = 0;
	int total_read_cnt = 0;
	
//	char *p_ret_cmd = NULL;

//	int i = 0;
	
	if ( fd <= 0 )
		return MOBILEYE_ADAS_RET_FAIL;
	
	write_cnt1 = cmd_len;

	mobileye_adas__mgr_mutex_lock();
	// Ŀ�ǵ带 ������ ����.
	while(retry++ < retry_cnt) 
	{
		// 1. Ŀ�ǵ带 ������.
		if ( write_cmd )
		{
			
			//printf("<atd> dbg : > [%s] (%d)\r\n", write_buf, write_cnt1);
			
			write_cnt2 = write(fd, write_buf, write_cnt1);
		}
		
		if ( write_cnt2 != write_cnt1 )
		{
			
			printf("<atd> dbg : write fail.. retry (%d),(%d)\r\n",write_cnt2,write_cnt1);
			ret = MOBILEYE_ADAS_CMD_RET_TIMEOUT;
			continue;
		}
		
		write_cmd = 0;
		
		// 2. Ŀ�ǵ� ���ϵɶ� ���� ���ٸ�.
		memset(tmp_buffer, 0x00, sizeof(tmp_buffer));
		cur_read_cnt = mobileye_adas__uart_wait_read(fd, tmp_buffer, MOBILEYE_ADAS_UART_BUFF_SIZE, MOBILEYE_ADAS_UART_READ_TIMEOUT_SEC) ;
		
		if(cur_read_cnt < 0)
		{
			printf("<atd> dbg : read timeout fail.. retry\r\n");
			ret = MOBILEYE_ADAS_CMD_RET_TIMEOUT;
			continue;
		}
		
		
		memcpy(result_buffer + total_read_cnt, tmp_buffer, cur_read_cnt);
		total_read_cnt += cur_read_cnt;
		
		if (total_read_cnt < 2)
			continue;
		
		if ((result_buffer[total_read_cnt-1] == 0x0A) && (result_buffer[total_read_cnt-2] = 0x0D))
		{
			// �������� ������ ���������� �������� ã�Ҵ�.
			
			ret = total_read_cnt;
			
			/*
			printf("read success! =========================== \r\n");
			for(i = 0 ; i < total_read_cnt ; i++ )
			{	
				char ch = result_buffer[i];
				
				if ((i > 0) && ((i%10)==0))
					printf("\r\n");
				//printf("[0x%02x(%c)]",result_buffer[i], result_buffer[i]);
				if((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'))
					printf("[0x%02x('%c')] ",result_buffer[i], result_buffer[i]);
				else
					printf("[0x%02x()] ",result_buffer[i]);
			}
			printf("========================================== \r\n");
			*/
			
			memcpy(ret_cmd, result_buffer, total_read_cnt-2);
			break;
		}
		
	}
	
	//close(fd);
	mobileye_adas__mgr_mutex_unlock();

	return ret;
}
#endif




>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
