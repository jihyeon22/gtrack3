#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <base/sender.h>
#include <base/watchdog.h>
#include <board/uart.h>

#include "lotte_packet.h"
#include "thread-model.h"
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

void thread_keypad(void)
{
	unsigned char buf[LEN_READ_BUFF] = {0};
	unsigned char resp_buf[LEN_READ_BUFF] = {0};
	unsigned char *pStart = NULL, *pEnd = NULL;
	int bool_receive_data = 0;

	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	_init_keypad();

	while(flag_run_thread_keypad)
	{
		int len = 0;

		watchdog_set_cur_ktime(eWdNet2);
		
		memset(buf, 0, sizeof(buf));
		len = _wait_read(fd_keypad, buf, sizeof(buf), 2);

		if(len <= 0)
		{
			if(bool_receive_data == 1)
			{
				_ack_keypad(resp_buf);
				bool_receive_data = 0;
			}
			continue;
		}

		bool_receive_data = 1;
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
		}
		
		_ack_keypad(buf);
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
#ifdef USE_DTG_MODEL
		fd = init_uart("/dev/ttyHSL2", 19200);
#else
		fd = init_uart("/dev/ttyHSL1", 19200);
#endif
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

