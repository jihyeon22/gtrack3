//For DRAGON Duali RFID reader.

#include <stdio.h>         // printf()
#include <string.h>        // strlen()
#include <time.h>
#include <pthread.h>
#include <fcntl.h>         // O_WRONLY
#include <unistd.h>        // write(), close()
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include <base/devel.h>
#include <board/uart.h>
#include <util/debug.h>
#include <util/tools.h>
#include "rfidtool.h"
#include "logd/logd_rpc.h"

const unsigned char cmd_find_card[] = {0x02,0x00,0x01,0x4c,0x4d};
const unsigned char cmd_beep_off[] = {0x02,0x00,0x02,0x13,0x01,0x10};

static int rfid_port = -1;

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;
	int len = 0;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = 0;
		tout.tv_usec = ftime;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return -1;
		
		if ( (len = read(fd, buf, buf_len)) <= 0)
			return -1;

		break; //success
	}

	return len;
}

static int rfid_check_packet(unsigned char *packet, int packet_len)
{
	int len = 0;
	unsigned char csum = 0;
	
	if(packet[0] != 0x02)
	{
		return -1;
	}

	len = packet[1]*256 + packet[2];
	
	//printf("packet len %d , len %d\n", packet_len, len);
	
	if(len + 4 != packet_len)
	{
		return -1;
	}

	csum = tools_checksum_xor(packet + 1, packet_len -2);
	if(csum != packet[packet_len - 1])
	{
		return -1;
	}

	return 0;	
}

int rfid_init(void)
{
	int ret;

	if ( (ret = init_uart(DEV_RFID, 115200)) < 0) {
		LOGE(eSVC_BASE, "uart initialize fail [%d]\n", ret);
		return -1;
	}
	rfid_port = ret;

	return 0;
}

int rfid_deinit(void)
{
	if(rfid_port < 0)
	{
		return -1;
	}

	close(rfid_port);
	rfid_port = -1;

	return 0;
}

int rfid_find_card(unsigned char *uid)
{
	unsigned char buff[40] = {0};
	int res = 0;
	
	write(rfid_port, cmd_find_card, sizeof(cmd_find_card));
	
	res = _wait_read(rfid_port, buff, sizeof(buff), 500000);
	if(res <= 0)
	{
		return -1;
	}

	if(rfid_check_packet(buff,res) < 0)
	{
		LOGE(eSVC_BASE, "rfid check packet error\n");
		return -1;
	}

	if(res == 5)
	{
		return 0;
	}

	if(res != 22 && res != 11 && res !=27)
	{
		return -1;
	}

	printf("find card\n");
	debug_hexdump_buff(buff, res);

	memcpy(uid, &(buff[res - 5]), 4);
	
	return 4;
}

int rfid_beep(int octave, int frequency, int off_usecs)
{
	unsigned char beep_on_buff[10] = {0x02, 0x00, 0x04, 0x13, 0x02};
	unsigned char res_buff[255] = {0};
	int res = 0;
	
	beep_on_buff[5] = octave;
	beep_on_buff[6] = frequency;
	beep_on_buff[7] = tools_checksum_xor(beep_on_buff+1, 6);

	write(rfid_port, beep_on_buff, 8);
	
	res = _wait_read(rfid_port, res_buff, sizeof(res_buff), 500000);

	printf("beep\n");
	debug_hexdump_buff(res_buff, res);

	if(off_usecs != 0)
	{
		usleep(off_usecs);
		
		write(rfid_port, cmd_beep_off, sizeof(cmd_beep_off));
		
		_wait_read(rfid_port, res_buff, sizeof(res_buff), 500000);
	}
	
	return 0;
}

int rfid_clear_uart()
{
	char dummy_buff[256];

	if(rfid_port < 0)
	{
		return -1;
	}
	
	while( read(rfid_port, dummy_buff, sizeof(dummy_buff)) > 0 );

	return 0;
}

static rfidData_t arr_rfid[MAX_RFID_ARRAY];
static int max_idx_rfid = -1;

int rfid_check_passenser(unsigned char *uid, int len_uid)
{
	int i;
	int board = RFID_NOT_USED;
	int idx_first_empty = -1;
	int limit_idx = 0;

//1.Search uid.
	if(max_idx_rfid + 2 < MAX_RFID_ARRAY)
	{
		limit_idx = max_idx_rfid + 2;
	}
	else
	{
		limit_idx = MAX_RFID_ARRAY;
	}

	for(i = 0;i < limit_idx; i++)
	{
		if(arr_rfid[i].boarding == RFID_NOT_USED)
		{
			if(idx_first_empty == -1)
				idx_first_empty = i;
			
			continue;
		}

		if(memcmp(arr_rfid[i].uid, uid, 4) == 0)
		{
			break;
		}
	}

//2.Process noexist/exist uid.
	time_t now = tools_get_kerneltime();

	if(i >= limit_idx)
	{
		LOGT(eSVC_BASE, "RFID : New rfid. %x%x%x%x\n", uid[0], uid[1], uid[2], uid[3]);
#if 1
		if(idx_first_empty != -1)
		{
			memcpy(arr_rfid[idx_first_empty].uid, uid, len_uid);
			arr_rfid[idx_first_empty].len_uid = len_uid;
			arr_rfid[idx_first_empty].ktime = now;
			i = idx_first_empty;

			if(idx_first_empty > max_idx_rfid)
			{
				max_idx_rfid = idx_first_empty;
			}
		}
		else
		{
			LOGE(eSVC_BASE, "RFID : Overflow array. (MAX %d, cur max idx %d)", MAX_RFID_ARRAY, max_idx_rfid);
			devel_webdm_send_log("RFID : Overflow array. (MAX %d, cur max idx %d)", MAX_RFID_ARRAY, max_idx_rfid);

			return RFID_GET_ON;
		}
#else
		return -1;
#endif
	}
	else if(now - arr_rfid[i].ktime < RFID_SKIP_SECS)
	{
		LOGT(eSVC_BASE, "RFID : %x%x%x%x Skip(check time < 30, %ds)\n",
			arr_rfid[i].uid[0],arr_rfid[i].uid[1],arr_rfid[i].uid[2],arr_rfid[i].uid[3], now - arr_rfid[i].ktime);
		return 0;
	}

//	LOGT(eSVC_BASE, "rfid history[%d] %x%x%x%x,%d,%d,%u\n", i, arr_rfid[i].uid[0],arr_rfid[i].uid[1],arr_rfid[i].uid[2],arr_rfid[i].uid[3],
//		arr_rfid[i].len_uid, arr_rfid[i].boarding, arr_rfid[i].ktime);

	arr_rfid[i].ktime = now;
	if(arr_rfid[i].boarding == RFID_GET_ON)
	{
		arr_rfid[i].boarding = RFID_NOT_USED;
		
		board = RFID_GET_OFF;
		LOGT(eSVC_BASE, "RFID : %x%x%x%x get off\n",  arr_rfid[i].uid[0],arr_rfid[i].uid[1],arr_rfid[i].uid[2],arr_rfid[i].uid[3]);
	}
	else
	{
		arr_rfid[i].boarding = RFID_GET_ON;
		
		board = RFID_GET_ON;
		LOGT(eSVC_BASE, "RFID : %x%x%x%x get on\n",  arr_rfid[i].uid[0],arr_rfid[i].uid[1],arr_rfid[i].uid[2],arr_rfid[i].uid[3]);
	}

	//rfid_dump_var_log();
	return board;
}

void rfid_dump_var_log(void)
{
	FILE *fp_log = NULL;
	char str_log[128] = {0};
	int i;
	int limit_idx = 0;

	if(max_idx_rfid + 2 < MAX_RFID_ARRAY)
	{
		limit_idx = max_idx_rfid + 2;
	}
	else
	{
		limit_idx = MAX_RFID_ARRAY;
	}

	fp_log = fopen(RFID_LOG_PATH, "w");

	for(i=0;i<limit_idx;i++)
	{
		snprintf(str_log, sizeof(str_log), "%x%x%x%x,%d,%d,%u\n",
			arr_rfid[i].uid[0],arr_rfid[i].uid[1],arr_rfid[i].uid[2],arr_rfid[i].uid[3],
			arr_rfid[i].len_uid, arr_rfid[i].boarding, arr_rfid[i].ktime);
		fwrite(str_log, 1, strlen(str_log), fp_log);
	}

	fclose(fp_log);
}

