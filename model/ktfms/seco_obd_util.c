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

#include "seco_obd.h"
#include "seco_obd_util.h"

#define DEBUG_AS_CMD
#ifdef DEBUG_AS_CMD
#include "kt_fms_packet.h"
#endif
// ------------------------------------------
// settinngs..
// ------------------------------------------
#define SECO_OBD_INVAILD_FD		-44
#define OBD_UART_BUFF_SIZE 		1024
#define OBD_UART_WAIT_READ_SEC 	3
#define SECO_MAX_WRITE_CMD_SIZE	512


int g_obd_fd = SECO_OBD_INVAILD_FD;

int send_obd_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, unsigned char* ret_cmd, const int retry_cnt);
int _init_uart_obd(char* dev, int baud , int *fd);

// --------------------------------------------------------------------
// obd �� ���ϰ��� ���ؼ� checksum �� üũ�Ѵ�.
// --------------------------------------------------------------------
static int seco_obd_chk_checksum(unsigned char sec_obd_data[], unsigned char data_start_bit)
{
	int i = 0;
	int checksum_bit = 0;
	unsigned char checksum_value = 0;
	
	checksum_bit = sec_obd_data[data_start_bit-1] + data_start_bit - 1;
	
	for (i = data_start_bit ; i < checksum_bit; i++)
	{
		checksum_value += sec_obd_data[i];
	}
	
	//printf("checksum => compute : [%d] / return : [%d]\r\n", checksum_value, sec_obd_data[checksum_bit]);
	
	if ( checksum_value == sec_obd_data[checksum_bit])
	{
		return OBD_RET_SUCCESS;
	}
	else
	{
		return OBD_RET_FAIL;
	}
}

// --------------------------------------------------------------------
// Ŀ�ǵ带 ������, ���� �޴� �Լ�.
// --------------------------------------------------------------------
int seco_obd_write_cmd_resp(char* sec_obd_cmd, unsigned char* sec_obd_data, int obd_data_len, unsigned char* ret_buff, int* error_code)
{
	unsigned char checksum_val = 0;
	
	char write_buff[SECO_MAX_WRITE_CMD_SIZE] = {0,};
	char tmp_ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int write_len = 0;
	int i = 0;
	
	int read_cnt = 0;
	int obd_data_len_with_chksum = 0;
	
	int data_start_bit = 0;
	
	int ret = OBD_RET_FAIL;
	
	*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
	
	if (g_obd_fd == SECO_OBD_INVAILD_FD)
		return OBD_RET_FAIL;
	
	while(*sec_obd_cmd)
		write_buff[write_len++] = *(sec_obd_cmd++);
	
	// �� ���̴� check sum 1 byte ���� �����Ѵ�.
	obd_data_len_with_chksum = obd_data_len + 1;
	
	// ���� ���̰� 256 �� �Ѿ�� data len �� 2����Ʈ�� ������ ǥ��
	if (obd_data_len < 256)
	{
		write_buff[write_len++] = obd_data_len_with_chksum;
		data_start_bit = eSECO_OBD_DATA1_IDX;
	}
	else
	{
		write_buff[write_len++] = ((obd_data_len_with_chksum >> 8) & 0xff);
		write_buff[write_len++] = (obd_data_len_with_chksum & 0xff);
		data_start_bit = eSECO_OBD_DATA2_IDX;
	}
	
	// ���������� ī��
	for(i=0; i < obd_data_len; i++)
	{
		write_buff[write_len++] = sec_obd_data[i];
	}
	
	// �������� check sum ����
	for(i = 0 ; i < obd_data_len ; i++)
	{
		checksum_val += sec_obd_data[i];
	}	
	
	// check sum ī��
	write_buff[write_len++] = checksum_val;
	
	// �������� 0d / 0a �� ������.
	write_buff[write_len++] = 0x0D;
	write_buff[write_len++] = 0x0A;
	
	/*
	for(i = 0 ; i < write_len ; i++ )
	{
		printf("[%d]:[0x%02x]\r\n",i, write_buff[i]);
	}
	*/
	
	// at cmd �� ������ �޴´�.
	read_cnt = send_obd_cmd_singleline_resp(g_obd_fd, write_buff, write_len, tmp_ret_buff, 3);
	
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
	
	// ���� �߻��� ó��
	if (strncmp ( tmp_ret_buff , "<AF", 3) == 0)
	{
		// ���� ���� ������ ���̴� 8�̾����Ѵ�.
		if (read_cnt == 8)
		{
			int code = 0;
			
			// char to int
			code += (tmp_ret_buff[3]-48);
			code += (tmp_ret_buff[4]-48)*10;
			code += (tmp_ret_buff[5]-48)*100;
			
			*error_code = code;
			
			printf("error code is [%d]\r\n",  *error_code);
#ifdef DEBUG_AS_CMD
			if (strncmp ( tmp_ret_buff , "<AS", 3) == 0)
			{
				g_last_dev_stat.last_set_gender_err_code = code;
			}
#endif
		}
		else
		{
			*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
		}
		return OBD_CMD_RET_ERROR;
	}
	
	// �ش� Ŀ�ǵ忡 ���� �������� Ȯ��
	if (strncmp ( tmp_ret_buff , sec_obd_cmd, strlen(sec_obd_cmd)) != 0)
	{
		*error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
		return OBD_CMD_RET_ERROR;
	}
	
	// ������������ checksum Ȯ��
	if ( seco_obd_chk_checksum(tmp_ret_buff, data_start_bit) != OBD_RET_SUCCESS )
	{
		return OBD_CMD_RET_CHECK_SUM_FAIL;
	}
	
	// ��������������, ���������� �����͸� ���� �޾ƿ°��̴�.
	// ȣ���ο��� ������ data �� �߶��� �Ѱ��ش�.
	ret =  (int)tmp_ret_buff[data_start_bit-1]-1;
	
  	// no ret data.
	if (ret == 0 )
	{
		*error_code = OBD_ERROR_CODE__NO_DATA_RET;
		return OBD_RET_SUCCESS;
	}

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



int seco_obd_init()
{
	int ret = 0;
	
	if ( g_obd_fd <= 0)
	{
		ret = _init_uart_obd(OBD_DEV_PATH, 115200, &g_obd_fd);
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

void seco_obd_deinit()
{
	if (g_obd_fd > 0)
		close(g_obd_fd);
	
	g_obd_fd = SECO_OBD_INVAILD_FD;
	
}

int is_sec_obd_init()
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
int _init_uart_obd(char* dev, int baud , int *fd)
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

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
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



int send_obd_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, unsigned char* ret_cmd, const int retry_cnt)
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
			ret = OBD_CMD_RET_TIMEOUT;
			continue;
		}
		
		write_cmd = 0;
		
		// 2. Ŀ�ǵ� ���ϵɶ� ���� ���ٸ�.
		memset(tmp_buffer, 0x00, sizeof(tmp_buffer));
		cur_read_cnt = _wait_read(fd, tmp_buffer, OBD_UART_BUFF_SIZE, OBD_UART_WAIT_READ_SEC) ;
		
		if(cur_read_cnt < 0)
		{
			printf("<atd> dbg : read timeout fail.. retry\r\n");
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
			
			memcpy(ret_cmd, result_buffer, total_read_cnt);
			break;
		}
		
	}
	
	//close(fd);
	
	return ret;
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

#include "seco_obd.h"
#include "seco_obd_util.h"

#define DEBUG_AS_CMD
#ifdef DEBUG_AS_CMD
#include "kt_fms_packet.h"
#endif
// ------------------------------------------
// settinngs..
// ------------------------------------------
#define SECO_OBD_INVAILD_FD		-44
#define OBD_UART_BUFF_SIZE 		1024
#define OBD_UART_WAIT_READ_SEC 	3
#define SECO_MAX_WRITE_CMD_SIZE	512


int g_obd_fd = SECO_OBD_INVAILD_FD;

int send_obd_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, unsigned char* ret_cmd, const int retry_cnt);
int _init_uart_obd(char* dev, int baud , int *fd);

// --------------------------------------------------------------------
// obd �� ���ϰ��� ���ؼ� checksum �� üũ�Ѵ�.
// --------------------------------------------------------------------
static int seco_obd_chk_checksum(unsigned char sec_obd_data[], unsigned char data_start_bit)
{
	int i = 0;
	int checksum_bit = 0;
	unsigned char checksum_value = 0;
	
	checksum_bit = sec_obd_data[data_start_bit-1] + data_start_bit - 1;
	
	for (i = data_start_bit ; i < checksum_bit; i++)
	{
		checksum_value += sec_obd_data[i];
	}
	
	//printf("checksum => compute : [%d] / return : [%d]\r\n", checksum_value, sec_obd_data[checksum_bit]);
	
	if ( checksum_value == sec_obd_data[checksum_bit])
	{
		return OBD_RET_SUCCESS;
	}
	else
	{
		return OBD_RET_FAIL;
	}
}

// --------------------------------------------------------------------
// Ŀ�ǵ带 ������, ���� �޴� �Լ�.
// --------------------------------------------------------------------
int seco_obd_write_cmd_resp(char* sec_obd_cmd, unsigned char* sec_obd_data, int obd_data_len, unsigned char* ret_buff, int* error_code)
{
	unsigned char checksum_val = 0;
	
	char write_buff[SECO_MAX_WRITE_CMD_SIZE] = {0,};
	char tmp_ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int write_len = 0;
	int i = 0;
	
	int read_cnt = 0;
	int obd_data_len_with_chksum = 0;
	
	int data_start_bit = 0;
	
	int ret = OBD_RET_FAIL;
	
	*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
	
	if (g_obd_fd == SECO_OBD_INVAILD_FD)
		return OBD_RET_FAIL;
	
	while(*sec_obd_cmd)
		write_buff[write_len++] = *(sec_obd_cmd++);
	
	// �� ���̴� check sum 1 byte ���� �����Ѵ�.
	obd_data_len_with_chksum = obd_data_len + 1;
	
	// ���� ���̰� 256 �� �Ѿ�� data len �� 2����Ʈ�� ������ ǥ��
	if (obd_data_len < 256)
	{
		write_buff[write_len++] = obd_data_len_with_chksum;
		data_start_bit = eSECO_OBD_DATA1_IDX;
	}
	else
	{
		write_buff[write_len++] = ((obd_data_len_with_chksum >> 8) & 0xff);
		write_buff[write_len++] = (obd_data_len_with_chksum & 0xff);
		data_start_bit = eSECO_OBD_DATA2_IDX;
	}
	
	// ���������� ī��
	for(i=0; i < obd_data_len; i++)
	{
		write_buff[write_len++] = sec_obd_data[i];
	}
	
	// �������� check sum ����
	for(i = 0 ; i < obd_data_len ; i++)
	{
		checksum_val += sec_obd_data[i];
	}	
	
	// check sum ī��
	write_buff[write_len++] = checksum_val;
	
	// �������� 0d / 0a �� ������.
	write_buff[write_len++] = 0x0D;
	write_buff[write_len++] = 0x0A;
	
	/*
	for(i = 0 ; i < write_len ; i++ )
	{
		printf("[%d]:[0x%02x]\r\n",i, write_buff[i]);
	}
	*/
	
	// at cmd �� ������ �޴´�.
	read_cnt = send_obd_cmd_singleline_resp(g_obd_fd, write_buff, write_len, tmp_ret_buff, 3);
	
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
	
	// ���� �߻��� ó��
	if (strncmp ( tmp_ret_buff , "<AF", 3) == 0)
	{
		// ���� ���� ������ ���̴� 8�̾����Ѵ�.
		if (read_cnt == 8)
		{
			int code = 0;
			
			// char to int
			code += (tmp_ret_buff[3]-48);
			code += (tmp_ret_buff[4]-48)*10;
			code += (tmp_ret_buff[5]-48)*100;
			
			*error_code = code;
			
			printf("error code is [%d]\r\n",  *error_code);
#ifdef DEBUG_AS_CMD
			if (strncmp ( tmp_ret_buff , "<AS", 3) == 0)
			{
				g_last_dev_stat.last_set_gender_err_code = code;
			}
#endif
		}
		else
		{
			*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
		}
		return OBD_CMD_RET_ERROR;
	}
	
	// �ش� Ŀ�ǵ忡 ���� �������� Ȯ��
	if (strncmp ( tmp_ret_buff , sec_obd_cmd, strlen(sec_obd_cmd)) != 0)
	{
		*error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
		return OBD_CMD_RET_ERROR;
	}
	
	// ������������ checksum Ȯ��
	if ( seco_obd_chk_checksum(tmp_ret_buff, data_start_bit) != OBD_RET_SUCCESS )
	{
		return OBD_CMD_RET_CHECK_SUM_FAIL;
	}
	
	// ��������������, ���������� �����͸� ���� �޾ƿ°��̴�.
	// ȣ���ο��� ������ data �� �߶��� �Ѱ��ش�.
	ret =  (int)tmp_ret_buff[data_start_bit-1]-1;
	
  	// no ret data.
	if (ret == 0 )
	{
		*error_code = OBD_ERROR_CODE__NO_DATA_RET;
		return OBD_RET_SUCCESS;
	}

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



int seco_obd_init()
{
	int ret = 0;
	
	if ( g_obd_fd <= 0)
	{
		ret = _init_uart_obd(OBD_DEV_PATH, 115200, &g_obd_fd);
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

void seco_obd_deinit()
{
	if (g_obd_fd > 0)
		close(g_obd_fd);
	
	g_obd_fd = SECO_OBD_INVAILD_FD;
	
}

int is_sec_obd_init()
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
int _init_uart_obd(char* dev, int baud , int *fd)
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

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
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



int send_obd_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, unsigned char* ret_cmd, const int retry_cnt)
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
			ret = OBD_CMD_RET_TIMEOUT;
			continue;
		}
		
		write_cmd = 0;
		
		// 2. Ŀ�ǵ� ���ϵɶ� ���� ���ٸ�.
		memset(tmp_buffer, 0x00, sizeof(tmp_buffer));
		cur_read_cnt = _wait_read(fd, tmp_buffer, OBD_UART_BUFF_SIZE, OBD_UART_WAIT_READ_SEC) ;
		
		if(cur_read_cnt < 0)
		{
			printf("<atd> dbg : read timeout fail.. retry\r\n");
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
			
			memcpy(ret_cmd, result_buffer, total_read_cnt);
			break;
		}
		
	}
	
	//close(fd);
	
	return ret;
}





>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
