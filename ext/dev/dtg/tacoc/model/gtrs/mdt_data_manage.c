#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_atcmd.h>
#include <board/board_system.h>
#include <wrapper/dtg_convtools.h>
#include <standard_protocol.h>

#include "dtg_data_manage.h"
#include "mdt_data_manage.h"

#define MAX_MDT_PACKET	360
static mdt800_packet_t g_mdt_pck[MAX_MDT_PACKET];
static int g_current_pck_insert_idx = 0;
static int g_current_pck_current_idx = 0;
static int g_current_save_count = 0;

unsigned char crc8(unsigned char crc, const unsigned char *buf, int len)
{
	while(len--) {
		crc ^= *buf++;
	}

	return crc & 0xff;
}


/**************************************************************
 * Async HDLC encoding & decoding
 **************************************************************/
int hdlc_async_encode(unsigned char *dst, unsigned char *src, int len)
{
	int dstlen = 0;

	while(len--) {
		if(*src == 0x7e || *src == 0x7d) {
			*dst = 0x7d;
			dst++;
			dstlen++;
			*dst = *src ^ 0x20;
		} else {
			*dst = *src;
		}
		dst++;
		dstlen++;
		src++;
	}
	return dstlen;
}


int get_mdt_count()
{
	return g_current_save_count;
}

void save_mdt_data(mdt800_packet_t *data)
{
	memcpy(&g_mdt_pck[g_current_pck_insert_idx++], data, sizeof(mdt800_packet_t));
	if(g_current_pck_insert_idx >= MAX_MDT_PACKET)
		g_current_pck_insert_idx = 0;

	g_current_save_count += 1;
	if(g_current_save_count > MAX_MDT_PACKET)
		g_current_save_count = MAX_MDT_PACKET;
}

void clear_mdt_data(int clear_count)
{
	int i;
	for(i = 0; i < clear_count; i++)
	{
		g_current_pck_current_idx += 1;
		if(g_current_pck_insert_idx >= MAX_MDT_PACKET)
			g_current_pck_insert_idx = 0;

		g_current_save_count -= 1;
		if(g_current_save_count < 0)
			g_current_save_count = 0;
	}
}

int get_mdt_data(unsigned char *buf, int request_count)
{
	int i;
	int start_idx;
	int idx;
	int real_cnt = 0;

	if(g_current_save_count >= request_count)
		real_cnt = request_count;
	else
		real_cnt = g_current_save_count;

	start_idx = g_current_pck_current_idx;
	idx = 0;
	for(i = 0; i < real_cnt; i++) {
		memcpy(&buf[idx], &g_mdt_pck[start_idx], sizeof(mdt800_packet_t));
		idx += sizeof(mdt800_packet_t);
		start_idx += 1;
		if(start_idx >= MAX_MDT_PACKET)
			start_idx = 0;
	}
	return real_cnt;
}

void print_report_data(mdt800_packet_t packet);
int parse_mdt_msg(unsigned char *std_buff, unsigned char *dest, int ev)
{
	
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;
	char *phonenum;
	int tmp_int = 0;
	mdt800_packet_t *p_mdt800_pack;
	p_mdt800_pack = (mdt800_packet_t *)dest;

	p_std_hdr = (tacom_std_hdr_t *)std_buff;
	p_std_data = (tacom_std_data_t *)&std_buff[sizeof(tacom_std_hdr_t)];

	memset(p_mdt800_pack, 0x00, sizeof(mdt800_packet_t));

	p_mdt800_pack->msg_id = MDT800_PROTOCOL_ID;
	p_mdt800_pack->msg_type = MDT800_MESSAGE_TYPE;

	memset(p_mdt800_pack->dev_id, 0x20, MAX_DEV_ID_LED);
	phonenum = atcmd_get_phonenum(); 
	if(phonenum != NULL)
		memcpy(p_mdt800_pack->dev_id, phonenum, strlen(phonenum));
	
	p_mdt800_pack->evcode = ev;
	
	p_mdt800_pack->date.year = 2000 + char_mbtol(p_std_data->date_time, 2);
	p_mdt800_pack->date.mon  = char_mbtol(p_std_data->date_time+2, 2);
	p_mdt800_pack->date.day  = char_mbtol(p_std_data->date_time+4, 2);
	p_mdt800_pack->date.hour = char_mbtol(p_std_data->date_time+6, 2);
	p_mdt800_pack->date.min  = char_mbtol(p_std_data->date_time+8, 2);
	p_mdt800_pack->date.sec  = char_mbtol(p_std_data->date_time+10, 2);

	p_mdt800_pack->gps_pos.latitude = char_mbtol(p_std_data->gps_x, 9)*10;
	p_mdt800_pack->gps_pos.longitude = char_mbtol(p_std_data->gps_y, 9)*10;

	if(p_mdt800_pack->gps_pos.latitude == 0 || p_mdt800_pack->gps_pos.longitude == 0)
		p_mdt800_pack->gps_status = eWCDMA_GSP;
	else
		p_mdt800_pack->gps_status = eSAT_GSP;
		

	tmp_int = char_mbtol(p_std_data->azimuth, 3);
	if (tmp_int >= 338 || tmp_int < 23)
		p_mdt800_pack->gps_dir = 0;
	else if (tmp_int >= 23 && tmp_int < 68)
		p_mdt800_pack->gps_dir = 1;
	else if (tmp_int >= 68 && tmp_int < 113)
		p_mdt800_pack->gps_dir = 2;
	else if (tmp_int >= 113 && tmp_int < 158)
		p_mdt800_pack->gps_dir = 3;
	else if (tmp_int >= 158 && tmp_int < 203)
		p_mdt800_pack->gps_dir = 4;
	else if (tmp_int >= 203 && tmp_int < 248)
		p_mdt800_pack->gps_dir = 5;
	else if (tmp_int >= 248 && tmp_int < 293)
		p_mdt800_pack->gps_dir = 6;
	else if (tmp_int >= 293 && tmp_int < 338)
		p_mdt800_pack->gps_dir = 7;


	p_mdt800_pack->speed = char_mbtol(p_std_data->speed, 3);
	p_mdt800_pack->vehicle_odo = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;

	p_mdt800_pack->temp1 = -5555;
	p_mdt800_pack->temp2 = -5555;
	p_mdt800_pack->temp3 = -5555;
/*
	if(therm_get_curr_data(&tmp_therm) == 0)
	{
		int i = 0;
		short temper = 0;

		for(i=0 ; i < tmp_therm.channel; i++)
		{
			switch(tmp_therm.temper[i].status)
			{
				case eOK:
					temper =  tmp_therm.temper[i].data;
					break;
					
				case eOPEN:
					temper = -3333;
					break;
					
				case eSHORT:
					temper = -4444;
					break;
					
				case eUNUSED:
				case eNOK:
				default:
					temper = -5555;
			}
			printf("CH-%d : %d C\n", i, temper);

			if(i  == 0)
				packet->temp1 = temper;
			else if(i == 1)
				packet->temp2 = temper;
			else if(i == 2)
				packet->temp3 = temper;
		}
	}
*/
	p_mdt800_pack->report_cycle_time = get_mdt_report_period(); // unit : sec
	p_mdt800_pack->gpio_status = 0;

	if(power_get_power_source() == POWER_SRC_DC)
		p_mdt800_pack->dev_power = 0;
	else
		p_mdt800_pack->dev_power = 1;

	p_mdt800_pack->create_cycle_time = get_mdt_create_period(); // unit : sec
	p_mdt800_pack->crc16 = crc8(0, (unsigned char *)p_mdt800_pack, sizeof(mdt800_packet_t)-2);

	print_report_data(*p_mdt800_pack);

#if defined(DEVICE_MODEL_INNOCAR)
	set_factor_value(p_std_data->k_factor, p_std_data->rpm_factor, p_std_data->weight1, p_std_data->weight2, p_std_hdr->dtg_fw_ver);
#endif
	
	return 0;
}


int mdt_enc_msg(unsigned char *src, unsigned char *dest, int len)
{
	int enc_len = 0;
	enc_len = hdlc_async_encode(dest, src, len);//sizeof(mdt800_packet_t));

	return enc_len;
}

void print_report_data(mdt800_packet_t packet)
{
	int i;
	printf("created report data ====>\n");
	printf("\tdata = %04d-%02d-%02d %02d:%02d:%02d\n", 
												packet.date.year,
												packet.date.mon,
												packet.date.day,
												packet.date.hour,
												packet.date.min,
												packet.date.sec
												);
	printf("\tmsg_id = [0x%02x]\n", packet.msg_id);
	printf("\tmsg_type = [0x%02x]\n", packet.msg_type);

	printf("\tdev_id : ");
	for(i = 0; i < MAX_DEV_ID_LED; i++) {
		printf("[%02x]", packet.dev_id[i]);
	}
	printf("\n");
	printf("\tevcode = [%d]\n", packet.evcode);
	printf("\tgps_status = [%d]\n", packet.gps_status);

	printf("\tlatitude = [%d]\n", packet.gps_pos.latitude);
	printf("\tlongitude = [%d]\n", packet.gps_pos.longitude);

	printf("\tgps_dir = [%d]\n", packet.gps_dir);
	printf("\tspeed = [%d]\n", packet.speed);

	printf("\tvehicle_odo = [%u]\n", packet.vehicle_odo);
	printf("\ttemp1 = [%d]\n", packet.temp1);
	printf("\ttemp2 = [%d]\n", packet.temp2);
	printf("\ttemp3 = [%d]\n", packet.temp3);
	printf("\treport_cycle_time = [%d]\n", packet.report_cycle_time);
	printf("\tgpio_status = [%d]\n", packet.gpio_status);
	printf("\tdev_power = [%d]\n", packet.dev_power);
	printf("\tcreate_cycle_time = [%d]\n", packet.create_cycle_time);
	printf("\crc = [%d]\n", packet.crc16);
}