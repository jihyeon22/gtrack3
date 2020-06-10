#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <base/sender.h>
#include <base/watchdog.h>
#include <board/uart.h>

#include <mdt800/packet.h>

#include "thread-keypad.h"
#include "debug.h"
#include "logd/logd_rpc.h"

static int _init_keypad(void);
static void _deinit_keypad(void);
static int _process_keypad(int num);
static int _ack_keypad(char *buf);
static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime);

#define ACK_DELAY_MS	50000
#define TOKEN_START_KEY	">KEY:"
#define TOKEN_END_KEY	"<"
#define LEN_READ_BUFF	128
#define LEN_VALUE		2

static int flag_run_thread_keypad = 1;
static int fd_keypad = -1;

#define KEY_PAD_ECHO__CHK_SERVER_RESP	
#define KEY_PAD_ECHO__MAX_WAIT_CNT		30

void thread_keypad(void)
{
	unsigned char buf[LEN_READ_BUFF] = {0};
	unsigned char resp_buf[LEN_READ_BUFF] = {0};
	unsigned char *pStart = NULL, *pEnd = NULL;
	int bool_receive_data = 0;

	int todo_chk_num = -1;
	int cur_key_num = -1;

	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	_init_keypad();

	while(flag_run_thread_keypad)
	{
		int len = 0;

		watchdog_set_cur_ktime(eWdNet2);
		
		memset(buf, 0, sizeof(buf));
		len = _wait_read(fd_keypad, buf, sizeof(buf), 1);

		if ( len > 0 )
			LOGI(LOG_TARGET, "[KEYPAD] READ DATA > [%s] [%d]\r\n", buf, len);
		
		
		if(len <= 0)
		{
#ifndef KEY_PAD_ECHO__CHK_SERVER_RESP
			if(bool_receive_data == 1)
			{
				_ack_keypad(resp_buf);
				bool_receive_data = 0;
			}
#else
			{
				char echo_str[128] = {0,};
				if ( todo_chk_num < 0 )
				{
					//printf(" key pad result >> skip !\r\n");
					continue;
				}

				LOGI(LOG_TARGET, "[KEYPAD] KEY PAD CHK RESULT : START >> NUM [%d]\r\n", todo_chk_num);

				if (keypad_server_result__get_result(todo_chk_num, echo_str) == KEY_RESULT_TRUE )
				{
					LOGI(LOG_TARGET, "[KEYPAD] KEY PAD CHK RESULT : SUCCESS >> NUM [%d]\r\n", todo_chk_num);
					_ack_keypad(echo_str);
					todo_chk_num = -1;
				}
				else
					LOGE(LOG_TARGET, "[KEYPAD] KEY PAD CHK RESULT : FAIL >> NUM [%d]\r\n", todo_chk_num);
			}
#endif
			continue;

		}

		
		pStart = buf;
		memcpy(resp_buf, buf, LEN_READ_BUFF);

		while(1)
		{
			char value[LEN_VALUE+1] = {0};
			int num_value = 0;
			
			pStart = strstr(pStart,TOKEN_START_KEY);
			if(pStart == NULL)
				break;

			pStart += sizeof(TOKEN_START_KEY) - 1;
			if(pStart >= buf + sizeof(buf))
				break;
			
			pEnd = strstr(pStart,TOKEN_END_KEY);
			if(pEnd == NULL)
				break;

			if(pEnd - pStart > LEN_VALUE || pEnd - pStart < 0)
				break;

			memcpy(value,  pStart, pEnd - pStart);

			num_value = atoi(value);

			_process_keypad(num_value);			

			cur_key_num = num_value;
			if ( cur_key_num != todo_chk_num )
			{
				todo_chk_num = cur_key_num;
				keypad_server_result__chk_set(todo_chk_num, buf);
			}
		}
#ifndef KEY_PAD_ECHO__CHK_SERVER_RESP
		bool_receive_data = 1;
		_ack_keypad(buf);
#endif
	}

	_deinit_keypad();
}

void exit_thread_keypad(void)
{
	flag_run_thread_keypad = 0;
}

static int _init_keypad(void)
{
	int fd = 0;
	int n_try = 3;

	while(n_try-- > 0)
	{
		fd = init_uart("/dev/ttyHSL1", 19200);
		if(fd >= 0)
		{
			break;
		}
		sleep(1);
	}
	if(n_try < 0)
	{
		printf("ERROR : Fail to open keypad.\n");
		return -1;
	}

	fd_keypad = fd;

	return 0;
}

static void _deinit_keypad(void)
{
	if(fd_keypad < 0)
	{
		return;
	}

	close(fd_keypad);
}

static int _process_keypad(int num)
{
	switch(num)
	{
		case 1:
			sender_add_data_to_buffer(eBUTTON_NUM0_EVT, NULL, ePIPE_1);
			break;
		case 2:
			sender_add_data_to_buffer(eBUTTON_NUM1_EVT, NULL, ePIPE_1);
			break;
		case 3:
			sender_add_data_to_buffer(eBUTTON_NUM2_EVT, NULL, ePIPE_1);
			break;
		case 4:
			sender_add_data_to_buffer(eBUTTON_NUM3_EVT, NULL, ePIPE_1);
			break;
		case 5:
			sender_add_data_to_buffer(eBUTTON_NUM4_EVT, NULL, ePIPE_1);
			break;
		case 6:
			sender_add_data_to_buffer(eBUTTON_NUM5_EVT, NULL, ePIPE_1);
			break;
		default :
			printf("ERROR : Inputted undefind key number. [value:%d]", num);
	}
	
	return 0;
}


static int _ack_keypad(char *buf)
{
	LOGI(LOG_TARGET, "[KEYPAD] KEY PAD SEND ACK : [%s]\r\n", buf);

	if(fd_keypad < 0)
	{
		return -1;
	}
	
	if(buf == NULL)
	{
		return -1;
	}
	
	usleep(ACK_DELAY_MS);
	write(fd_keypad, buf, strlen(buf));
	
	return 0;
}

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;
	int len = 0;
	int uart_len;
	int n_try = 1000;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	tout.tv_sec = ftime;
	tout.tv_usec = 0;
	result = select(fd + 1, &reads, 0, 0, &tout);
	if(result <= 0) //time out & select error
		return len;

	while(n_try-- > 0)
	{
		uart_len = read(fd, &(buf[len]), buf_len-len);
		if(uart_len <= 0)
			break;
		
		len += uart_len;
	}

	return len;
}

// ================================================================
#include <pthread.h>

static pthread_mutex_t keypad_result_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct key_result
{
	int chk_flag;
	int pkt_op;
	int key_num;
	char echo_str[32];
	int result;
}KEY_RESULT_T;

static KEY_RESULT_T _g_key_result;

static int __keypad_server_result__clr()
{
	memset(_g_key_result.echo_str, 0x00, sizeof(_g_key_result.echo_str));
	_g_key_result.pkt_op = -1;
	_g_key_result.key_num = -1;
	_g_key_result.result = KEY_RESULT_FALSE;

}

int keypad_server_result__chk_set(int num, char* echo_str)
{
	pthread_mutex_lock(&keypad_result_mutex);

	__keypad_server_result__clr();
	switch(num)
	{
		case 1:
			_g_key_result.pkt_op = eBUTTON_NUM0_EVT;
			break;
		case 2:
			_g_key_result.pkt_op = eBUTTON_NUM1_EVT;
			break;
		case 3:
			_g_key_result.pkt_op = eBUTTON_NUM2_EVT;
			break;
		case 4:
			_g_key_result.pkt_op = eBUTTON_NUM3_EVT;
			break;
		case 5:
			_g_key_result.pkt_op = eBUTTON_NUM4_EVT;
			break;
		case 6:
			_g_key_result.pkt_op = eBUTTON_NUM5_EVT;
			break;
		default :
			_g_key_result.pkt_op = -1;
			printf("ERROR : Inputted undefind key number. [value:%d]", num);
	}

	_g_key_result.key_num = num;
	_g_key_result.result = KEY_RESULT_FALSE;
	
	strcpy(_g_key_result.echo_str, echo_str);
	pthread_mutex_unlock(&keypad_result_mutex);
	return 0;
}

int keypad_server_result__set_result(int op, int result)
{
	pthread_mutex_lock(&keypad_result_mutex);
	if ( _g_key_result.pkt_op == op )
		_g_key_result.result = result;
	pthread_mutex_unlock(&keypad_result_mutex);
	return 0;
}

int keypad_server_result__get_result(int num, char* echo_str)
{
	int chk_op;
	int ret = 0;

	LOGD(LOG_TARGET, "[KEYPAD] KEY PAD CHK RESULT [%d] : START\r\n", num);

	pthread_mutex_lock(&keypad_result_mutex);
	switch(num)
	{
		case 1:
			chk_op = eBUTTON_NUM0_EVT;
			break;
		case 2:
			chk_op = eBUTTON_NUM1_EVT;
			break;
		case 3:
			chk_op = eBUTTON_NUM2_EVT;
			break;
		case 4:
			chk_op = eBUTTON_NUM3_EVT;
			break;
		case 5:
			chk_op = eBUTTON_NUM4_EVT;
			break;
		case 6:
			chk_op = eBUTTON_NUM5_EVT;
			break;
		default :
			chk_op = -1;
	}

	if ( ( _g_key_result.pkt_op == chk_op ) && ( _g_key_result.result == KEY_RESULT_TRUE ) )
	{
		strcpy(echo_str, _g_key_result.echo_str);
		__keypad_server_result__clr();
		LOGD(LOG_TARGET, "[KEYPAD] KEY PAD CHK RESULT [%d] : SUCCESS \r\n", num);
		ret =  KEY_RESULT_TRUE;
	}
	else
	{
		LOGE(LOG_TARGET, "[KEYPAD] KEY PAD CHK RESULT [%d] : FAIL \r\n", num);
		ret =  KEY_RESULT_FALSE;
	}

	pthread_mutex_unlock(&keypad_result_mutex);
	return ret;
}
