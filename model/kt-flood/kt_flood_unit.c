#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <board/uart.h>
#include <sys/ioctl.h>

#include "debug.h"
#include "logd/logd_rpc.h"

#include <base/sender.h>
#include "kt_flood_mdt800/packet.h"
#include "kt_flood_unit.h"
#include "leakdata-list.h"

#include "color_printf.h"

int g_rfid_fd = -1;
char g_sensor_state[MAX_SENSOR_CNT];
extern is_list_init;

int kt_flood_unit_init()
{
	g_rfid_fd = init_uart(UART_DEV_DEFAULT_PATH, UART_DEV_DEFAULT_BAUDRATE);

}
int kt_flood_unit_close()
{
	uart_close(g_rfid_fd);
}

void kt_flood_unit_get_read_buf(unsigned char * resp_buff, int buff_len)
{
	// int buff_len = strlen(resp_buff);
	int i = 0;
	int packet_idx = 0;
	int bSensor_diff = 0;	

	if ((resp_buff[packet_idx] != FLOOD_UNIT_START_FLAG) || ( resp_buff[buff_len-1] != FLOOD_UNIT_END_FLAG) )
    {
		devel_webdm_send_log("resp_buff error  0x%02x  0x%02x \n", resp_buff[packet_idx], resp_buff[buff_len-1]);
		print_yellow("resp_buff error  \n");

		for(i = packet_idx; i < buff_len; i++)
		{
			printf("[%d] 0x%02x  ", i,  resp_buff[i]);
		}
		print_yellow("resp_buff error  \n");       
    	return;
    }
	
	memset(&floodunit_sensor, 0x0, sizeof(floodunit_sensor));
	memcpy(floodunit_sensor, &resp_buff[packet_idx+1], sizeof(floodunit_sensor));

	// for(i = 0; i < sizeof(floodunit_sensor); i++)
	// {
	// 	printf("[%d] 0x%02x  ", i,  floodunit_sensor[i]);
	// }


	printf("\n");

	unsigned char XOR;

	XOR = mds_api_checksum_xor(&floodunit_sensor,sizeof(floodunit_sensor));
	if(XOR != resp_buff[buff_len-2])
	{
		devel_webdm_send_log("resp_buff error  xor 0x%02x  / 0x%02x \n", resp_buff[buff_len-2], XOR);        
    	return;
    }

	//print_yellow("Xor 0x%02x /  0x%02x ", resp_buff[buff_len-2], XOR);

	for(i = 0; i < sizeof(g_sensor_state); i++)
	{
		if(g_sensor_state[i] != floodunit_sensor[i])
		{
			memcpy(&g_sensor_state, &floodunit_sensor, sizeof(floodunit_sensor));
            devel_webdm_send_log("g_sensor_state modify [%d] 0x%02x\n", i, floodunit_sensor[i]);
			LOGI(LOG_TARGET, "g_sensor_state modify [%d] 0x%02x\n", i, floodunit_sensor[i]);
			bSensor_diff = 1;

			print_yellow("g_sensor_state change !! \n");
            sender_add_data_to_buffer(eUART_CHANGE_EVT, NULL, ePIPE_1);
			return;
		}

	}

	// printf("bSensor_diff : %d \n", bSensor_diff);
}

int kt_flood_unit_get_send_buf(unsigned char * send_buf, int buf_len)
{
	flood_etx_t flood_etx;
	//packet_date_t date;
	flood_date_t date;
	int packet_idx = 0;
    int warring_cnt = 0;

	memset(send_buf, 0x00, buf_len);
	send_buf[packet_idx++] = 0x5b;	

    warring_cnt = get_leak_data_count();

	if (is_list_init > 0)
	{
		printf ("FLOODUNIT_CMD_EMPTY list !!!\n");
		send_buf[packet_idx++] = FLOODUNIT_CMD_EMPTY;
        sprintf(&send_buf[packet_idx], "%s", INIT_DEVICE_ID);
		packet_idx = strlen(send_buf);
		is_list_init = 0;

		if(warring_cnt > 0)
			list_del_all(&leak_data_list);
		
	}
	else if(warring_cnt > 0)
    {
		unsigned char * leak_device; 
		list_pop(&leak_data_list, &leak_device);

		send_buf[packet_idx++] = FLOODUNIT_CMD_LEAK;
		strncpy(&send_buf[packet_idx], leak_device, MAX_DEVICE_ID);

		print_yellow("warning deviceid : %s\n", leak_device);
		LOGI(LOG_TARGET, "leak_data deviceid : %s\n", leak_device);

		if(leak_device != NULL)
		{
			printf("free leak data\n");
			free(leak_device);
		}
        packet_idx += MAX_DEVICE_ID;
    }	
    else
    {
        send_buf[packet_idx++] = FLOODUNIT_CMD_REQUEST;
        sprintf(&send_buf[packet_idx], "%s", INIT_DEVICE_ID);
	    packet_idx = strlen(send_buf);
    }

	// jhcho afterwards
    struct tm time = {0,};
	get_modem_time_tm(&time);

	//date.year = time.tm_year + 1900;
	date.year[0] = (time.tm_year + 1900) / 100;
	date.year[1] = (time.tm_year + 1900) % 100;
	date.mon = time.tm_mon + 1;
	date.day = time.tm_mday;
	date.hour = time.tm_hour;
	date.min = time.tm_min;
	date.sec = time.tm_sec;

	memcpy(&send_buf[packet_idx], &date, sizeof(packet_date_t));
	packet_idx = packet_idx + sizeof(packet_date_t);

	flood_etx.XOR = mds_api_checksum_xor(&send_buf[1],packet_idx-1);
	flood_etx.ETX = 0x5d;

	memcpy(&send_buf[packet_idx], &flood_etx, sizeof(flood_etx_t));
	packet_idx += sizeof(flood_etx_t);

	return packet_idx;
}
int kt_flood_unit_state_write()
{
	unsigned char receive_buf[128];
	int uresult;
	int packet_idx = 0;
	flood_etx_t flood_etx;

	int rty_cnt;

	unsigned char send_buf[29];
	int send_buf_len;
	
	send_buf_len = strlen(send_buf);

	if(g_rfid_fd == -1)
	{
		LOGI(LOG_TARGET, "g_rfid_fd -1 error\n");
		return -1;
	}

	packet_idx = 0;
	rty_cnt = 10;

	while(1)
	{
		memset(&send_buf, 0x0, send_buf_len);
		packet_idx = kt_flood_unit_get_send_buf(send_buf, send_buf_len);
		uresult = uart_write(g_rfid_fd, (unsigned char *)send_buf, packet_idx);
		//printf("uart write = %d\n", uresult);

		if(uresult < 0)
		{
			printf("uart write error uresult :  %d\n", uresult);
			LOGI(LOG_TARGET, "uart write error!!! uresult : %d \n", uresult);
			return 0;
		}

		memset(&receive_buf, 0x00, sizeof(receive_buf));
		uresult = uart_read(g_rfid_fd, (unsigned char *)&receive_buf, 128, 5);		

		//printf("uart read = %d\n", uresult);
		if(uresult > 0)
		{
			kt_flood_unit_get_read_buf(receive_buf, uresult);
			break;
		}
		else 
			printf("uart read error = %d\n", uresult);

		if(rty_cnt-- <= 0)
		{
			print_red("rty_cnt fail : retry count over\n");
			LOGI(LOG_TARGET, "rty_cnt fail : retry count over \n");
			memset(g_sensor_state, 0x70, sizeof(g_sensor_state));
			return 0;
		}
		

	}
	
	// dump_packet(&receive_buf, strlen(receive_buf), "uart_read");
	// dump_data("uart_read", (unsigned char *)&receive_buf, strlen(receive_buf));		
}
