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

#include "seco_obd_1.h"
#include "seco_obd_1_mgr.h"
#include "seco_obd_1_protocol.h"
#include "seco_obd_1_util.h"

// ------------------------------------------
// settinngs..
// ------------------------------------------
//#define DEBUG_MSG_OBD_UART

int g_obd_fd = SECO_OBD_INVAILD_FD;

int seco_obd_1_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, char* ret_cmd, const int retry_cnt);
int _init_seco_obd_1_uart(char* dev, int baud , int *fd);


// --------------------------------------------------------------------
// Ŀ�ǵ带 ������, ���� �޴� �Լ�.
// --------------------------------------------------------------------
// OBD+FWU+ 
//         NON 
// OBD+FWU+NON<
// OBD+FWU+NON=
// OBD+FWU+NON?
// OBD+FWU+ERR=xxxx
//#define DEBUG_MSG_OBD_UART
char debug_tmp_str[1024] = {0,};

int seco_obd_1_write_cmd_resp(const char* sec_obd_cmd1, const char* sec_obd_cmd2, const int cmd_type, const char* sec_obd_data, char* ret_buff, int* error_code)
{
//	unsigned char checksum_val = 0;
	
	char write_buff[SECO_MAX_WRITE_CMD_SIZE] = {0,};
    int write_buff_len = 0;
    
    char write_cmd_tok[SECO_MAX_WRITE_CMD_SIZE] = {0,};
    int write_cmd_tok_len = 0;
    
	char tmp_ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	
	// int write_len2 = 0;
	// int i = 0;
	
	int read_cnt = 0;
	// int obd_data_len_with_chksum = 0;
	
	int data_start_bit = 0;
	
	int ret = OBD_RET_FAIL;
	
    
	*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
	
    // 1. uart Ȯ��
	if (g_obd_fd == SECO_OBD_INVAILD_FD)
		return OBD_RET_FAIL;
	
    // 2. obd cmd ������
	while(*sec_obd_cmd1)
    {
        char ch = *(sec_obd_cmd1++);
		write_buff[write_buff_len++] = ch;
        write_cmd_tok[write_cmd_tok_len++] = ch;
    }
	
    while(*sec_obd_cmd2)
    {
        char ch = *(sec_obd_cmd2++);
		write_buff[write_buff_len++] = ch;
        write_cmd_tok[write_cmd_tok_len++] = ch;
    }
    
    switch (cmd_type)
    {
        case eCMD_TYPE_SETTING:
        {
            write_buff[write_buff_len++] = '=';
            break;
        }
        case eCMD_TYPE_GET_VALUE:
        {
            write_buff[write_buff_len++] = '?';
            break;
        }
        case eCMD_TYPE_INPUT_VALUE:
        {
            write_buff[write_buff_len++] = '<';
            break;
        }
        default:
		{
            *error_code = OBD_ERROR_CODE__UNKOWN_CODE;
	        return OBD_CMD_RET_ERROR;
		}
    }
    
    if ( sec_obd_data != NULL)
    {
        while(*sec_obd_data)
            write_buff[write_buff_len++] = *(sec_obd_data++);
    }
	// �������� 0d / 0a �� ������.
	write_buff[write_buff_len++] = 0x0D;
	write_buff[write_buff_len++] = 0x0A;
	

	#ifdef DEBUG_MSG_OBD_UART
	printf("write command buffer is [%s]\r\n", write_buff);
    printf("write command tok is [%s]\r\n", write_cmd_tok);
    #endif

	// 3. cmd �� ������ �޴´�.
	read_cnt = seco_obd_1_cmd_singleline_resp(g_obd_fd, write_buff, write_buff_len, tmp_ret_buff, 1);
	
    #ifdef DEBUG_MSG_OBD_UART
    memset(debug_tmp_str, 0x00, sizeof(debug_tmp_str));
    strcpy(debug_tmp_str, tmp_ret_buff);

    printf(" >> write response  is [%s] / [%d] \r\n", tmp_ret_buff, read_cnt);
    #endif
	
    //printf("tmp_ret_buff [%s]/[%d]\r\n", tmp_ret_buff,read_cnt);
	if (read_cnt == OBD_CMD_RET_TIMEOUT)
	{
		*error_code = OBD_ERROR_CODE__UART_READ_TIMEOUT;
		return OBD_CMD_RET_TIMEOUT;
	}
	
	if (read_cnt <= 0 )
	{
		printf("%s() - %d line : read fail..\r\n",__func__, __LINE__);
		return OBD_RET_FAIL;
	}
	
    // 4. TODO : error ó��
    { 
        char* err_cmd = strstr ( tmp_ret_buff , "ERR=");
        if (err_cmd != NULL)
        {
            *error_code = OBD_ERROR_CODE__KNOWN_CODE;
            // XXX+ERR= ���� ������ ���� ī��
            strcpy((char*)ret_buff, err_cmd+4);
            printf("%s() - %d line : err ret.. [%s]\r\n",__func__, __LINE__, ret_buff);
            return OBD_CMD_RET_ERROR;
        }
    }
	
	
    if ( ( strstr(write_buff, "OBD+SBR") == NULL ) && ( strstr(write_buff, "OBD+SRR") == NULL ) )
    {
        // 5. TODO : �ش� Ŀ�ǵ忡 ���� �������� Ȯ��
        if ( strncmp ( tmp_ret_buff , write_cmd_tok, strlen(write_cmd_tok)) != 0)
        {
            *error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
            printf("%s() - %d line : invalid return string.. [%s] / [%s] \r\n",__func__, __LINE__, tmp_ret_buff, write_cmd_tok);
            return OBD_CMD_RET_ERROR;
        }
    }
    else
    {
        int found_flag = 0;
        // 5. TODO : �ش� Ŀ�ǵ忡 ���� �������� Ȯ��
        if ( strncmp ( tmp_ret_buff , "OBD+SRR", strlen("OBD+SRR")) != 0)
        {
            *error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
            //printf("%s() - %d line : invalid return string.. sbr 1 [%s] / [%s] \r\n",__func__, __LINE__, tmp_ret_buff, write_cmd_tok);
            found_flag = 1;
        }

        if ( strncmp ( tmp_ret_buff , "OBD+SBR", strlen("OBD+SBR")) != 0)
        {
            *error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
            //printf("%s() - %d line : invalid return string.. sbr 1 [%s] / [%s] \r\n",__func__, __LINE__, tmp_ret_buff, write_cmd_tok);
            found_flag = 1;
        }

        if ( found_flag == 0 )
        {
            printf("%s() - %d line : invalid return string.. sbr 1 [%s] / [%s] \r\n",__func__, __LINE__, tmp_ret_buff, write_cmd_tok);
            return OBD_CMD_RET_ERROR;
        }
    }
	
	// ��������������, ���������� �����͸� ���� �޾ƿ°��̴�.
	// ȣ���ο��� ������ data �� �߶��� �Ѱ��ش�.
    data_start_bit = write_cmd_tok_len + 1;
	ret = strlen(tmp_ret_buff + data_start_bit);
	//printf("real data = [%d] / total data =[%d]\r\n", ret, read_cnt);
	
	if (ret > 0)
	{
		memcpy(ret_buff, tmp_ret_buff + data_start_bit, ret);
	}
	else
	{
		*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
		return OBD_RET_FAIL;
	}
	// printf("%s() - %d line \r\n",__func__, __LINE__);

    
	*error_code = OBD_ERROR_CODE__SUCCESS;
	return ret;
}

char g_obd_dev_path_str[64] = {0,};
int g_obd_dev_baudrate = 0;

int seco_obd_1_init()
{
	int ret = 0;
	
	if ( g_obd_fd <= 0 )
	{
		char tmp_obd_dev_path_str[64] = {0,};
		int tmp_obd_dev_baudrate = 0;

		if ( strlen(g_obd_dev_path_str) > 0 )
			strcpy(tmp_obd_dev_path_str, g_obd_dev_path_str);
		else
			strcpy(tmp_obd_dev_path_str, OBD_DEV_DEFAULT_PATH);

		if ( g_obd_dev_baudrate == 0 )
			tmp_obd_dev_baudrate = g_obd_dev_baudrate;
		else
			tmp_obd_dev_baudrate = OBD_DEV_DEFAULT_BAUDRATE;
		
		ret = _init_seco_obd_1_uart(tmp_obd_dev_path_str, tmp_obd_dev_baudrate, &g_obd_fd);
	}
	else
	// �̹� ������������.
	{
		
		return OBD_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_obd_fd = SECO_OBD_INVAILD_FD;
		return OBD_RET_FAIL;
	}
	
	printf("init obd uart [%d], [%d]\r\n",ret, g_obd_fd);
	
	return OBD_RET_SUCCESS;
	
}

int seco_obd_get_fd()
{
	return g_obd_fd;
}

void seco_obd_1_deinit()
{
	if (g_obd_fd > 0)
		close(g_obd_fd);
	
	g_obd_fd = SECO_OBD_INVAILD_FD;
	
}

int is_sec_obd_1_init()
{
	if ( g_obd_fd <= 0)
	{
		return OBD_RET_FAIL;
	}
	else
	{
		return OBD_RET_SUCCESS;
	}
}

// ----------------------------------------------------
// uart util
// ----------------------------------------------------
int _init_seco_obd_1_uart(char* dev, int baud , int *fd)
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

// ---------------------------------------------------------
// at cmd util
// ---------------------------------------------------------

static int _wait_read(int fd, char *buf, int buf_len, int ftime)
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



int seco_obd_1_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, char* ret_cmd, const int retry_cnt)
{
	int ret = OBD_RET_FAIL;
	
	int write_cnt1 = 0;
	int write_cnt2 = 0;
	
	char tmp_buffer[OBD_UART_BUFF_SIZE] = {0,};
	unsigned char result_buffer[OBD_UART_BUFF_SIZE] = {0,};
	
	int write_cmd = 1;
	int retry = 0;
	
	int cur_read_cnt = 0;
	int total_read_cnt = 0;
	
//	char *p_ret_cmd = NULL;

//	int i = 0;
	
	if ( fd <= 0 )
		return OBD_RET_FAIL;
	
	write_cnt1 = cmd_len;

	seco_obd_1_mutex_lock();
	// Ŀ�ǵ带 ������ ����.
	while(retry++ < retry_cnt) 
	{
		// 1. Ŀ�ǵ带 ������.
		if ( write_cmd )
		{
			
			//printf("<obd> dbg : > [%s] (%d)\r\n", write_buf, write_cnt1);
			
			write_cnt2 = write(fd, write_buf, write_cnt1);
		}
		
		if ( write_cnt2 != write_cnt1 )
		{
			
			printf("<obd> dbg : write fail.. retry (%d),(%d)\r\n",write_cnt2,write_cnt1);
			ret = OBD_CMD_RET_TIMEOUT;
			continue;
		}
		
		write_cmd = 0;
		
		// 2. Ŀ�ǵ� ���ϵɶ� ���� ���ٸ�.
		memset(tmp_buffer, 0x00, sizeof(tmp_buffer));
		cur_read_cnt = _wait_read(fd, tmp_buffer, OBD_UART_BUFF_SIZE, OBD_UART_WAIT_READ_SEC) ;
		
		if(cur_read_cnt < 0)
		{
			printf("<obd> dbg : read timeout fail.. retry\r\n");
			ret = OBD_CMD_RET_TIMEOUT;
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
	seco_obd_1_mutex_unlock();

	return ret;
}





