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

#include "katech-obd.h"
#include "katech-obd-util.h"

// ------------------------------------------
// settinngs..
// ------------------------------------------
#define KATECH_OBD_INVAILD_FD		-44
#define OBD_UART_BUFF_SIZE 			1024
#define OBD_UART_WAIT_READ_SEC 		3
#define KATECH_MAX_WRITE_CMD_SIZE		512


int g_obd_fd = KATECH_OBD_INVAILD_FD;

int send_at_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, unsigned char* ret_cmd, const int retry_cnt);
int _init_uart_obd(char* dev, int baud , int *fd);

// --------------------------------------------------------------------
// obd �� ���ϰ��� ���ؼ� checksum �� üũ�Ѵ�.
// --------------------------------------------------------------------
static const unsigned short crc16tab[256]= {
	0x0000,0x1021,0x2042,0x3063,0x4084,
	0x50a5,0x60c6,0x70e7,0x8108,0x9129,
	0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,
	0xf1ef,0x1231,0x0210,0x3273,0x2252,
	0x52b5,0x4294,0x72f7,0x62d6,0x9339,
	0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,
	0xf3ff,0xe3de,0x2462,0x3443,0x0420,
	0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,
	0xf5cf,0xc5ac,0xd58d,0x3653,0x2672,
	0x1611,0x0630,0x76d7,0x66f6,0x5695,
	0x46b4,0xb75b,0xa77a,0x9719,0x8738,
	0xf7df,0xe7fe,0xd79d,0xc7bc,0x48c4,
	0x58e5,0x6886,0x78a7,0x0840,0x1861,
	0x2802,0x3823,0xc9cc,0xd9ed,0xe98e,
	0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,
	0x0a50,0x3a33,0x2a12,0xdbfd,0xcbdc,
	0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,
	0xab1a,0x6ca6,0x7c87,0x4ce4,0x5cc5,
	0x2c22,0x3c03,0x0c60,0x1c41,0xedae,
	0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,
	0x8d68,0x9d49,0x7e97,0x6eb6,0x5ed5,
	0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,
	0xaf3a,0x9f59,0x8f78,0x9188,0x81a9,
	0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,
	0xe16f,0x1080,0x00a1,0x30c2,0x20e3,
	0x5004,0x4025,0x7046,0x6067,0x83b9,
	0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,
	0xe37f,0xf35e,0x02b1,0x1290,0x22f3,
	0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,
	0xe54f,0xd52c,0xc50d,0x34e2,0x24c3,
	0x14a0,0x0481,0x7466,0x6447,0x5424,
	0x4405,0xa7db,0xb7fa,0x8799,0x97b8,
	0xe75f,0xf77e,0xc71d,0xd73c,0x26d3,
	0x36f2,0x0691,0x16b0,0x6657,0x7676,
	0x4615,0x5634,0xd94c,0xc96d,0xf90e,
	0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,
	0x08e1,0x3882,0x28a3,0xcb7d,0xdb5c,
	0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,
	0xbb9a,0x4a75,0x5a54,0x6a37,0x7a16,
	0x0af1,0x1ad0,0x2ab3,0x3a92,0xfd2e,
	0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,
	0x9de8,0x8dc9,0x7c26,0x6c07,0x5c64,
	0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,
	0xbfba,0x8fd9,0x9ff8,0x6e17,0x7e36,
	0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,
	0x1ef0
};

unsigned short int katech_obd_chk_checksum(const char buf[], const unsigned int len)
{
	volatile  unsigned int counter;
 	volatile  unsigned short int crc=0;
	
	for( counter = 0; counter < len; counter++)
	{
 		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *buf++)&0x00FF];
	}
	return(crc);
}

// --------------------------------------------------------------------
// Ŀ�ǵ带 ������, ���� �޴� �Լ�.
// --------------------------------------------------------------------
int katech_obd_write_cmd_resp(unsigned char* katech_obd_cmd, unsigned char* katech_obd_data, int obd_data_len, unsigned char* ret_buff, int* error_code)
{
//	unsigned char checksum_val = 0;
	
	unsigned char write_buff[KATECH_MAX_WRITE_CMD_SIZE] = {0,};
	char tmp_ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	
	int write_len = 0;
	int i = 0;
	
	int read_cnt = 0;
//	int obd_data_len_with_chksum = 0;
	
	int data_start_bit = 0;
    int chksum_target_len = 0;
	
	int ret = OBD_RET_FAIL;
	
	*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
	
	if (g_obd_fd == KATECH_OBD_INVAILD_FD)
		return OBD_RET_FAIL;
	
	// make cmd..
	write_len = make_obd_cmd(katech_obd_cmd, strlen(katech_obd_cmd), write_buff);
	
    // x on / x off
    //write_buff[0] = 0x06;
    
	// debug
	for(i = 0 ; i < write_len ; i++ )
	{
		//printf("write cmd [%d]:[0x%02x]\r\n",i, write_buff[i]);
	}
    
    //write_buff[i++] = '\n';
    
	// at cmd �� ������ �޴´�.
	read_cnt = send_at_cmd_singleline_resp(g_obd_fd, write_buff, i, tmp_ret_buff, 4);
	
	if (read_cnt == OBD_CMD_RET_TIMEOUT)
	{
		*error_code = OBD_ERROR_CODE__UART_READ_TIMEOUT;
		return OBD_CMD_RET_TIMEOUT;
	}
	
	if ( read_cnt <= 0 )
	{
		printf("%s() - %d line : read fail..\r\n",__func__, __LINE__);
		return OBD_RET_FAIL;
	}
	
    //  - ���䰪�� '<' + 'Ŀ�ǵ�' + '������' + 'checksum' + '>'
    
	// �ش� Ŀ�ǵ忡 ���� �������� Ȯ��
	if ( strncmp ( tmp_ret_buff+1 , katech_obd_cmd, strlen(katech_obd_cmd)) != 0)
	{
		*error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
		return OBD_CMD_RET_ERROR;
	}
	
	// ������������ checksum Ȯ��
    //data_start_bit = 1 + strlen(katech_obd_cmd);
    data_start_bit = 1;
    
    chksum_target_len = read_cnt;
    chksum_target_len -= data_start_bit;
    chksum_target_len -= 3; // checksum 2byte + '>'

    //printf("%s() : chksum_target_len [%d] / [%d] \r\n", __func__, chksum_target_len, read_cnt);
	
    // --------------------
    // check checksum
    // --------------------
    { 
        unsigned char check_sum_byte[2] = {0,};
        unsigned short int check_sum =  katech_obd_chk_checksum(tmp_ret_buff + data_start_bit, chksum_target_len);
        
        memcpy(&check_sum_byte, &check_sum, 2);
        
        // ������ 2��° ĳ����
        if ( check_sum_byte[0] != tmp_ret_buff[read_cnt-2])
        {
            *error_code = OBD_CMD_RET_CHECK_SUM_FAIL;
            printf("%s() - %d line : error chk sum fail .. \r\n",__func__, __LINE__);
            return OBD_RET_FAIL;
        }
        
        // ������ 3��° ĳ����
        if ( check_sum_byte[1] != tmp_ret_buff[read_cnt-3] )
        {
            *error_code = OBD_CMD_RET_CHECK_SUM_FAIL;
            printf("%s() - %d line : error chk sum fail .. \r\n",__func__, __LINE__);
            return OBD_RET_FAIL;
        }
	}
    
	// �������������, ���������� �����͸� ��� �޾ƿ°��̴�.
	// ȣ��ο��� ������ data �� �߶� �Ѱ��ش�.
    data_start_bit = 1 + strlen(katech_obd_cmd);
	ret = read_cnt - data_start_bit - 3;    
    
  	// no ret data.
	if ( ret == 0 )
	{
		*error_code = OBD_ERROR_CODE__NO_DATA_RET;
        //printf("%s() - %d line : error chk OBD_ERROR_CODE__NO_DATA_RET .. \r\n",__func__, __LINE__);
		return OBD_RET_SUCCESS;
	}

    if ( ret < 0 )
    {
        *error_code = OBD_ERROR_CODE__NOT_VAILD_CMD_RET;
        //printf("%s() - %d line : error chk OBD_ERROR_CODE__NO_DATA_RET .. \r\n",__func__, __LINE__);
		return OBD_RET_FAIL;
    }
	//printf("real data = [%d] / total data =[%d]\r\n", ret, read_cnt);
	
	if (ret > 0)
	{
		memcpy(ret_buff, tmp_ret_buff + data_start_bit, ret);
	}
	else
	{
		*error_code = OBD_ERROR_CODE__UNKOWN_CODE;
        printf("%s() - %d line : error chk OBD_ERROR_CODE__UNKOWN_CODE .. \r\n",__func__, __LINE__);
		return OBD_RET_FAIL;
	}

    // printf("%s() - %d line : error chk SUCCESS .. \r\n",__func__, __LINE__);
	*error_code = OBD_ERROR_CODE__SUCCESS;
	return ret;
}



int katech_obd_init()
{
	int ret = 0;
	
	if ( g_obd_fd <= 0)
	{
		ret = _init_uart_obd(OBD_DEV_PATH, 115200, &g_obd_fd);
	}
	else
	// �̹� �����������.
	{
		
		return OBD_RET_SUCCESS;
	}
	
	if ( ret != 0 )
	{
		g_obd_fd = KATECH_OBD_INVAILD_FD;
		return OBD_RET_FAIL;
	}
	
	printf("init obd uart [%d], [%d]\r\n",ret, g_obd_fd);
	
	return OBD_RET_SUCCESS;
	
}

void katech_obd_deinit()
{
	if (g_obd_fd > 0)
		close(g_obd_fd);
	
	g_obd_fd = KATECH_OBD_INVAILD_FD;
	
}

int is_katech_obd_init()
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
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1?? ????
	//newtio.c_cc[VMIN] = vmin; // ??? n ???? ???? ?????? ????
	newtio.c_cc[VTIME] = 0;
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
		//printf("_wait_read [%s]\r\n", buf);
		if ( read_cnt <= 0)
			return -1;

		break; //success
	}

	return read_cnt;
}



int send_at_cmd_singleline_resp(int fd, const char* write_buf, const int cmd_len, unsigned char* ret_cmd, const int retry_cnt)
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
		
		// 2. Ŀ�ǵ� ���ϵɶ� ���� ��ٸ�.
		memset(tmp_buffer, 0x00, sizeof(tmp_buffer));
		cur_read_cnt = _wait_read(fd, tmp_buffer, OBD_UART_BUFF_SIZE, OBD_UART_WAIT_READ_SEC) ;
		
		//printf("cur_read_cnt is [%d]\r\n",cur_read_cnt);
		
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
		
		if (result_buffer[total_read_cnt-1] == '>')
		{
			// ������� ������ ���������� ������� ã�Ҵ�.
//			int i = 0;
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



int make_obd_cmd(const char cmd[], const int cmd_len, unsigned char outbuff[])
{
	int i = 0;
	
	unsigned char cmd_buff[128] = {0,};
	int obd_cmd_idx = 0;
	
	unsigned char check_sum_byte[2] = {0,};
	unsigned short int check_sum = katech_obd_chk_checksum(cmd, cmd_len);
	
	//printf("check_sum is [%x]\r\n", check_sum);
	memcpy(&check_sum_byte, &check_sum, 2);
	
	cmd_buff[obd_cmd_idx++] = '<';
	
	for ( i = 0; i < cmd_len; i++ )
	{
		cmd_buff[obd_cmd_idx++] = cmd[i];
	}
	
	cmd_buff[obd_cmd_idx++] = check_sum_byte[1];
	cmd_buff[obd_cmd_idx++] = check_sum_byte[0];
	
	cmd_buff[obd_cmd_idx++] = '>';
	
	for ( i = 0; i < obd_cmd_idx; i++ )
	{
		//printf("cmd_buff[%d] = [%02x]\r\n", i, cmd_buff[i]);
	}
	
	memcpy(outbuff, cmd_buff, obd_cmd_idx);
	
	
	return obd_cmd_idx;
}


