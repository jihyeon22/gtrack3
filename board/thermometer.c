#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <termios.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include "thermometer.h"

#include <logd_rpc.h>

#include <base/devel.h>
#define LOG_TARGET eSVC_COMMON


#if USE_TYPE_UT1
static therm_conf_t			g_therm_conf = 
{
	.device = DEV_THERM_PORT,
	.baudrate = BAUDRATE_UT1,
	.msg_type = eUT1,
};
#else
static therm_conf_t			g_therm_conf = 
{
	.device = DEV_THERM_PORT,
	.msg_type = eUnknown,
};
#endif

int _init_uart(char* dev, int baud , int *fd)
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
	//newtio.c_cc[VTIME] = vtime; // timeout 0.1�� ����
	//newtio.c_cc[VMIN] = vmin; // �ּ� n ���� ���� ������ ����
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(*fd, TCIFLUSH);
	tcsetattr(*fd, TCSANOW, &newtio);
	return 0;
}

void dump(char *buf, int len)
{
/*
	int i;
	for(i = 0; i < len; i++)
	{
		printf("%c", buf[i]);
	}
	printf("\n");
*/
}

char* strnchr(char *buf, int data, int len)
{
	int i;
	for(i = 0; i < len; i++)
	{
		if(buf[i] == data)
		{
			return &buf[i];
		}
	}
	return NULL;
}

int read_select(int fd, char *buf, int bufsize, int *len, int sec)
{
	int result;
	fd_set reads;
	struct timeval timeout;
	if(sec == 0)
	{
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
	}
	else
	{
		timeout.tv_sec = sec;
		timeout.tv_usec = 100000;
	}

	FD_ZERO(&reads);
	FD_SET(fd, &reads);
	result = select(fd + 1, &reads, NULL, NULL, &timeout);
	if(result > 0)
	{
		usleep(100000);
		*len = read(fd, buf, bufsize);
	}
	return result;
}

int check_type()
{
	char buf[MAX_THERM_BUF] = {0,};
	int thermal_fd = 0;
	int result;
	int len;

	_init_uart(g_therm_conf.device, BAUDRATE_UT1 , &thermal_fd);

	result = read_select(thermal_fd, buf, MAX_THERM_BUF, &len, 1);
	if(result > 0)
	{
		if(len == 10)
		{
			printf("[%s] UT1 PF-nA!\n", __FUNCTION__);
			close(thermal_fd);
			g_therm_conf.msg_type = eUT1;
			g_therm_conf.baudrate = BAUDRATE_UT1;
			return eUT1;
		}
		else if(len == 42)
		{
			printf("[%s] UT1 PF-3!\n", __FUNCTION__);
			close(thermal_fd);
			g_therm_conf.msg_type = eUT1;
			g_therm_conf.baudrate = BAUDRATE_UT1;
			return eUT1;
		}
	}

	close(thermal_fd);
/*
	_init_uart(DEV_THERM_PORT, BAUDRATE_UT3 , &thermal_fd);
	//write(thermal_fd, ">RT<", sizeof(">RT<"));
	write(thermal_fd, ">RT<", sizeof(">RT<"));
	result = read_select(thermal_fd, buf, MAX_THERM_BUF, &len, 3);
	if(result > 0)
	{
		if(len >= 2 && strstr(buf, "OK") != NULL)
		{
			printf("[%s] UT3 PF-A!\n", __FUNCTION__);
			close(thermal_fd);
			g_therm_conf.msg_type = eUT3;
			g_therm_conf.baudrate = BAUDRATE_UT3;
			return eUT3;
		}
		else
		{
			printf("[%s:%d] CAN'T RECOGNIZE DEVICE TYPE!\n", __FUNCTION__, __LINE__);
			close(thermal_fd);
			return -1;
		}
	}
	else
	{
		printf("[%s:%d] CAN'T RECOGNIZE DEVICE TYPE!\n", __FUNCTION__, __LINE__);
		close(thermal_fd);
		return -1;
	}
*/
	printf("[%s:%d] CAN'T RECOGNIZE TEMPERATURE DEVICE TYPE!\n", __FUNCTION__, __LINE__);
	return -1;
}

int _parse_temper(COMMON_UT1 *temper, TEMP_LOCAL_DATA *tempdata)
{
	dump((char *)temper, 4);
	//check status
	switch(temper->sign)
	{
		case  0 :
		case  1 :
		case '0':
		case '1':
			tempdata->status = eOK;
			 //success case
			 break;		
		case '9':
			if(memcmp((char *)temper, "9999", 4) != 0)
			{
				return -1;
			}
			tempdata->status = eNOK;
			break;
		case 'O':
			if(memcmp((char *)temper, "OOOO", 4) != 0 && memcmp((char *)temper, "OPEN", 4) != 0)
			{
				return -1;
			}
			tempdata->status = eOPEN;
			break;
		case 'S':
			if(memcmp((char *)temper, "SSSS", 4) != 0 && memcmp((char *)temper, "SHRT", 4) != 0)
			{
				return -1;
			}
			tempdata->status = eSHORT;
			break;
		case 'X':
			if(memcmp((char *)temper, "XXXX", 4) != 0)
			{
				return -1;
			}
			tempdata->status = eUNUSED;
			printf("UN USED data...XXXX\n");
			break;
		case 'N':
			if(memcmp((char *)temper, "NOUS", 4) != 0)
			{
				return -1;
			}
			tempdata->status = eUNUSED;
			printf("UN USED data...NOUS\n");
			break;
		default:
			printf("unknown data...\n");
			tempdata->status = eUNUSED;
			break;
	}

	if(tempdata->status != eOK)
		return 0;


	if(temper->data[0] <= 9)
	{
		tempdata->data = temper->data[0] * 100 + temper->data[1] * 10 + temper->data[2];
		if( (temper->data[0] * 100 + temper->data[1] * 10) > 500 ) {
			printf("abnormal temperature reading error, more than 500C\n");
			return -1;
		}

		if(temper->sign == '1' || temper->sign == 1)
			tempdata->data = tempdata->data * (-1);
	}
	else if(temper->data[0] >= '0' && temper->data[0] <= '9')
	{
		char num[4] = {0,};
		memcpy(num, temper->data, sizeof(temper->data));

		tempdata->data = atoi(num);
		if(temper->sign == '1' || temper->sign == 1)
			tempdata->data = tempdata->data * (-1);
	}
	else
	{
		printf("check value fail!\n");
		return -1;
	}
	printf("value %d\n", tempdata->data);

	return 0;
}

int parse_temper(char *buf, int len, THERMORMETER_DATA *therm)
{
	int i;
	int res = 0;
	switch(len)
	{
		case 10:
		{
			UT1_PFnA_10 *temp = (UT1_PFnA_10 *)buf;
			if(temp->stx != '>' || temp->etx != '<')
			{
				res = -1;
				break;
			}
			for(i = 0; i < 2; i++)
			{
				if(_parse_temper(&temp->temper[i], &therm->temper[i]) == -1)
				{
					res = -1;
				}
			}
			therm->temper[i++].status = eNOK;
			therm->temper[i].status = eNOK;
			therm->channel = CHANNEL2;
			break;
		}
		case 12:
		{
			UT1_PFA_12 *temp = (UT1_PFA_12 *)buf;
			if(temp->stx != '>' || temp->etx != '<')
			{
				res = -1;
				break;
			}
			for(i = 0; i < 2; i++)
			{
				if(_parse_temper(&temp->temper[i], &therm->temper[i]) == -1)
				{
					res = -1;
				}
				//alarm value
				therm->temper[i].alarm = temp->alarm[i];
			}
			therm->temper[i++].status = eNOK;
			therm->temper[i].status = eNOK;
			therm->channel = CHANNEL2;
			break;
		}
		case 18:
		{
			UT1_PFnA_18 *temp = (UT1_PFnA_18 *)buf;
			if(temp->stx != '>' || temp->etx != '<')
			{
				res = -1;
				break;
			}
			for(i = 0; i < 4; i++)
			{
				if(_parse_temper(&temp->temper[i], &therm->temper[i]) == -1)
				{
					res = -1;
				}
			}
			therm->channel = CHANNEL4;
			break;
		}
		case 42:
		{
			if(g_therm_conf.msg_type == eUT1) {
				UT1_PF3_42 *temp = (UT1_PF3_42 *)buf;
				if(temp->stx != '>' || temp->etx != '<')
				{
					res = -1;
					break;
				}
				for(i = 0; i < 2; i++)
				{
					if(_parse_temper(&temp->temper[i], &therm->temper[i]) == -1)
					{
						res = -1;
					}
				}
			}
			else if(g_therm_conf.msg_type == eUT3) {
				UT3_PFA_42 *temp = (UT3_PFA_42 *)buf;
				if(temp->stx != '>' || temp->etx != '<')
				{
					printf("[%s] init fail!\n", __FUNCTION__);
					res = -1;
					break;
				}
				for(i = 0; i < 2; i++)
				{
					if(_parse_temper(&temp->temper[i], &therm->temper[i]) == -1)
					{
						res = -1;
					}
				}
			}
			therm->temper[i++].status = eNOK;
			therm->temper[i].status = eNOK;
			therm->channel = CHANNEL2;
			break;
		}
		default:
		{
			res = -1;
		}
	}
	return res;
}

int init_therm()
{
	int res = 0;
	char buf[MAX_THERM_BUF] = {0,};
	int thermal_fd = 0;
	int result = 0;
	int len;
	if(g_therm_conf.msg_type == eUT1)
	{
		//UT1 device don't need initialization.
	}
	else if(g_therm_conf.msg_type == eUT3)
	{
		_init_uart(g_therm_conf.device, g_therm_conf.baudrate, &thermal_fd);
		write(thermal_fd, ">CL<", sizeof(">CL<"));
		result = read_select(thermal_fd, buf, MAX_THERM_BUF, &len, 5);
		if(result > 0)
		{
			if(len >= 3 && strstr(buf, ">CL") != NULL)
			{
				res = 0;
			}
			else
			{
				printf("[%s] init fail!\n", __FUNCTION__);
				res = -1;
			}
		}
		else
		{
			printf("[%s] init fail!\n", __FUNCTION__);
			res = -1;
		}
		close(thermal_fd);
	}
	return res;
}

int set_therm_device(char *dev, int len_dev)
{
	if(len_dev >= 64)
	{
		printf("ERROR : [%s] Device name is too long.\n", __FUNCTION__);
	
		return -1;
	}

	snprintf(g_therm_conf.device, sizeof(g_therm_conf.device)-1, "%s",  dev);

	return 0;
}

int get_therm(THERMORMETER_DATA *therm)
{
	int res = 0;
	char buf[MAX_THERM_BUF] = {0,};
	int thermal_fd = 0;
	int result;
	int len;

	static int thermal_sensor_err_send_info = 0;
	static int thermal_sensor_success_send_info = 0;

	if(g_therm_conf.msg_type == eUT1)
	{
		_init_uart(g_therm_conf.device, g_therm_conf.baudrate, &thermal_fd);
		result = read_select(thermal_fd, buf, MAX_THERM_BUF, &len, 1);
		if(result > 0)
		{
			
			//printf("[%s] %s\n", __FUNCTION__, buf);
			if(parse_temper(buf, len, therm) == -1)
			{
				
				printf("[%s:%d] Parse error!\n", __FUNCTION__, __LINE__);
				res = -1;

				// -----------------------------------------------
				// debug info for thermal sensor
				// -----------------------------------------------
				{
					char debug_msg[64] = {0,};
					int i = 0;
					int length = 0;

					length += sprintf(debug_msg+length, "THERM SENSOR ERR (UT1) : ");

					for ( i = 0 ; i < len ; i ++)
					{
						length += sprintf(debug_msg+length, "0x%02x ", buf[i]);
						if (length > (64 - 7) ) // buffer overrun skip
							break;
					}

					LOGE(LOG_TARGET, debug_msg);

					if ( thermal_sensor_err_send_info == 0 ) // only one send..
					{
						devel_webdm_send_log( debug_msg );
						thermal_sensor_err_send_info = 1;
					}
				}
			}
			else
			{
				if ( thermal_sensor_success_send_info == 0 )
				{
					devel_webdm_send_log( "THERM SENSOR GET SUCCESS (UT1)" );
					thermal_sensor_success_send_info = 1;
				}
			}
		}
		else
		{
			printf("[%s] select timeout!\n", __FUNCTION__);
			res = -1;
		}
		close(thermal_fd);
	}
	else if(g_therm_conf.msg_type == eUT3)
	{
		_init_uart(g_therm_conf.device, g_therm_conf.baudrate, &thermal_fd);
		write(thermal_fd, ">OK<", sizeof(">OK<"));
		result = read_select(thermal_fd, buf, MAX_THERM_BUF, &len, 1);
		if(result > 0)
		{
			
			if(parse_temper(buf, len, therm) == -1)
			{
				printf("[%s:%d] Parse error!\n", __FUNCTION__, __LINE__);
				res = -1;

				// -----------------------------------------------
				// debug info for thermal sensor
				// -----------------------------------------------
				{
					char debug_msg[64] = {0,};
					int i = 0;
					int length = 0;

					length += sprintf(debug_msg+length, "THERM SENSOR ERR (UT3) : ");

					for ( i = 0 ; i < len ; i ++)
					{
						length += sprintf(debug_msg+length, "0x%02x ", buf[i]);
						if (length > (64 - 7) ) // buffer overrun skip
							break;
					}

					LOGE(LOG_TARGET, debug_msg);

					if ( thermal_sensor_err_send_info == 0 ) // only one send..
					{
						devel_webdm_send_log( debug_msg );
						thermal_sensor_err_send_info = 1;
					}
				}
			}
			else
			{
				if ( thermal_sensor_success_send_info == 0 )
				{
					devel_webdm_send_log( "THERM SENSOR GET SUCCESS (UT3)" );
					thermal_sensor_success_send_info = 1;
				}
			}
			
		}
		else
		{
			printf("[%s] select timeout!\n", __FUNCTION__);
			res = -1;
		}
		close(thermal_fd);
	}
	else
	{
		res = -1;
	}
	return res;
}


int get_tempature(THERMORMETER_DATA	*thermometer)
{
	int ret;
	//int i;
	//int channel = 0;
	static int flag_success_once = 0;
	
	memset(thermometer, 0x00, sizeof(THERMORMETER_DATA));

#if USE_TYPE_UT1
#else
	int type;
	type = check_type();
	if(type < 0) {
		if(flag_success_once == 1) {
			type = check_type();
			if(type < 0) {
				return -1;
			}
		} else	{
			return -1;
		}
	}

	ret = init_therm();
	if(ret < 0)
		return -1;
#endif

	ret = get_therm(thermometer);
	if(ret < 0) {
		if(flag_success_once == 1) {
			ret = get_therm(thermometer);
			if(ret < 0) {
				return -1;
			}
		}
		else {
			return -1;
		}
	}

	flag_success_once = 1;
	return 0;
}
