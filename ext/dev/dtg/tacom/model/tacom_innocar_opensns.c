/**
* @file tacom_innocar.c
* @brief 
* @author Jinwook Hong (jinwook@mdstec.com)
* @version 
* @date 2013-06-10
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <wrapper/dtg_log.h>
#include <tacom_internal.h>
#include <tacom_protocol.h>

#include <tacom_innocar_protocol.h>
#include <common/crc16.h>
#include <common/power.h>
#include <common/w200_led.h>
#include <termios.h>
#include "uart.h"
#include "convtools.h"
#include "taco_store.h"

//#define DTG_TTY_DEV_NAME	"/dev/ttyMSM"
//#define DTG_TTY_DEV_NAME	"/dev/ttyHSL2"


#if defined(SERVER_MODEL_OPENSNS)
	#include "iniutill.h"
	#define CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/opensns.ini"
	#define CONFIG_FILE_PATH		"/data/opensns.ini"
	#define CREATE_MDT_PERIOD		"mdt_config:create_period"
	int g_mdt_collection_period		= 3; //default value
	int g_mdt_count = 0;
#elif defined(SERVER_MODEL_OPENSNS_TB)
	#include "iniutill.h"
	#define CONFIG_FILE_PATH_ORG	"/system/mds/system/bin/opensns_tb.ini"
	#define CONFIG_FILE_PATH		"/data/opensns_tb.ini"
	#define CREATE_MDT_PERIOD		"mdt_config:create_period"
	int g_mdt_collection_period		= 3; //default value
	int g_mdt_count = 0;
#endif

//#define GPS_FIXED_IN_SPEED_ZERO

extern int dtg_uart_fd;

int inno_ack_records(int readed_bytes);
static int valid_check(unsigned char *buf, int size);
struct tacom_setup inno_setup  = {
	.tacom_dev				= "TACOM INNOCAR : ",
	.cmd_rl					= NULL,
	.cmd_rs					= NULL,
	.cmd_rq					= NULL,
	.cmd_rr					= NULL,
	.cmd_rc					= NULL,	
	.cmd_ack				= NULL,
	.data_rl				= 0,
	.data_rs				= 0,
	.data_rq				= 0,
	.start_mark				= 0x02,
	.end_mark				= 0x03,
	.head_length			= 79,
	.head_delim				= ',',
	.head_delim_index		= 80,
	.head_delim_length		= 1,
	.record_length			= 53,
	.max_records_size		= (MAX_ONE_DATA_COUNT * MAX_INNO_DATA_PACK), // this constant value have to set over max_records_per_once's value.
	.max_records_per_once	= 470,
	.conf_flag				= 0x6,
};

static tacom_inno_hdr_t inno_header = {0, };
//static tacom_inno_hdr_t inno_header_tmp = {0, };
static int _header_tmp = 0;

#define INNO_HEADER_EMPTY 0
#define INNO_HEADER_FULL 1
static int inno_header_status = INNO_HEADER_EMPTY;

static tacom_inno_data_t read_curr_buf;

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
void wait_taco_unill_power_on()
{
	int ign_status;
	unsigned char buf[128];
	int i;
	if(power_get_ignition_status() == POWER_IGNITION_OFF)
	{
		while(1)
		{
			ign_status = power_get_ignition_status();
			if(ign_status == POWER_IGNITION_ON) {
				inno_header_status = INNO_HEADER_EMPTY;
				memset(&read_curr_buf, 0x00, sizeof(tacom_inno_data_t));
				return;
			}

			DTG_LOGE("taco thread wait for ign off\n");
			for(i = 0; i < 10; i++)
			{
				if(dtg_uart_fd > 0)
					read(dtg_uart_fd, buf, 128); //uart flush
				sleep(1);
			}
		}
	}
}
#endif	

static int valid_check(unsigned char *buf, int size){
	unsigned char checksum = 0;
	int i;

#ifdef GPS_FIXED_IN_SPEED_ZERO
	if(buf[size-1] == 0xff)
		return 0;
#endif

	for(i = 0; i < size-1; i++){
		checksum += buf[i];
	}

	if (checksum == buf[size-1])
		return 0;
	else {
		DTG_LOGE("valid_check error = 0x%x, 0x%x", checksum, buf[size-1]);
		return -1;
	}
}

struct l_table {
	char head;
	unsigned char body[4];
};

struct p_table {
	char head[5];
	unsigned char body[4];
	int size;
};

struct l_table locale_tbl[17] = {
	{'A', "����"}, {'B', "����"}, {'C', "����"}, {'D', "�泲"}, 
	{'E', "����"}, {'F', "����"}, {'G', "����"}, {'H', "�泲"}, 
	{'I', "����"}, {'J', "����"}, {'K', "��õ"}, {'L', "����"},
	{'M', "�λ�"}, {'N', "�뱸"}, {'O', "����"}, {'P', "����"}, 
	{'Z', "����"}
};

struct p_table part_tbl[44] = {
	{"BA   ", "��", 2}, {"SA   ", "��", 2}, 
	{"AH   ", "��", 2}, {"JA   ", "��", 2}, 
	{"HEO  ", "��", 2}, {"GA   ", "��", 2}, 
	{"NA   ", "��", 2}, {"DA   ", "��", 2}, 
	{"RA   ", "��", 2}, {"MA   ", "��", 2}, 
	{"GEO  ", "��", 2}, {"NEO  ", "��", 2}, 
	{"DEO  ", "��", 2}, {"REO  ", "��", 2}, 
	{"MEO  ", "��", 2}, {"BEO  ", "��", 2}, 
	{"SEO  ", "��", 2}, {"EO   ", "��", 2}, 
	{"JEO  ", "��", 2}, {"GO   ", "��", 2}, 
	{"NO   ", "��", 2}, {"DO   ", "��", 2}, 
	{"RO   ", "��", 2}, {"MO   ", "��", 2}, 
	{"BO   ", "��", 2}, {"SO   ", "��", 2}, 
	{"OH   ", "��", 2}, {"JO   ", "��", 2}, 
	{"GU   ", "��", 2}, {"NU   ", "��", 2}, 
	{"DU   ", "��", 2}, {"RU   ", "��", 2}, 
	{"MU   ", "��", 2}, {"BU   ", "��", 2}, 
	{"SU   ", "��", 2}, {"WO   ", "��", 2}, 
	{"JU   ", "��", 2}, {"DIPL ", "��", 2}, 
	{"CNSL ", "��", 2}, {"S-DIP", "�ؿ�", 4}, 
	{"SCNSL", "�ؿ�", 4}, {"INTL ", "����", 4}, 
	{"AGREE", "����", 4}, {"ETC  ", "��Ÿ", 4}, 
};

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;
	int len = 0;
	int uart_len;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return len;

		uart_len = read(fd, &buf[len], buf_len-len);
		if(uart_len <= 0)
			return len;

		len += uart_len;
		break; //success
	}

	return len;
}


static void ri_to_uni(char *buf, char*destbuf)
{
	int b_idx = 0;
	int t_idx = 0;
	int i;
	char tmp_buf[12];

/*	
printf("source ri_to_uni==========>\n");
	for(i = 0; i < 12; i++)
		printf("%c ", buf[i]);
	printf("\n");
*/

	memset(tmp_buf, 0, 12);
	if ((buf[b_idx] >= 'A' && buf[b_idx] <= 'P') || buf[b_idx] == 'Z') {
		for (i = 0; i < 17; i++) 
		{
			if (buf[b_idx] == locale_tbl[i].head) {
				memcpy(tmp_buf, locale_tbl[i].body, 4);
				t_idx +=4;
				break;
			}
		}
		b_idx++;
	}
	memcpy(&tmp_buf[t_idx], &buf[b_idx], 2);
	t_idx += 2;
	b_idx += 2;
	for(i = 0; i < 44; i++) 
	{
		if(!strncmp(&buf[b_idx], part_tbl[i].head, 5)) 
		{
			memcpy(&tmp_buf[t_idx], part_tbl[i].body, part_tbl[i].size);
			t_idx += part_tbl[i].size;
			break;
		}
	}
	memcpy(destbuf, tmp_buf, t_idx);
	memcpy(&destbuf[8], &buf[8], 4);
/*
printf("dest ri_to_uni==========>\n");
	for(i = 0; i < 12; i++)
		printf("%c", destbuf[i]);
	printf("\n");

	for(i = 0; i < 12; i++)
		printf("%02x ", destbuf[i]);
	printf("\n");
*/
}


static unsigned short g_K_Factor = 0;
static char g_RPM_Factor = 0;

void request_setting_info()
{
	int total_error_cnt = 0;
	int i;
	int ret = 0;
	int read_cnt = 0;
	int read_bytes = 0;
	int total_read_bytes = 0;
	unsigned char buf[512];
	unsigned short data_len = 0x0005;
	unsigned char rv_cmdstr[9];
	tacom_inno_sv_t *p_set_value;
	tacom_inno_old_sv_t  *p_old_set_value;
	memset(rv_cmdstr, 0, sizeof(rv_cmdstr));
	rv_cmdstr[0] = 0x02;
	rv_cmdstr[1] = 0x50;
	memcpy(&rv_cmdstr[2], &data_len, 2);
	strncpy(&rv_cmdstr[4], "RV", 2);
	rv_cmdstr[6] = 0x0;
	for(i = 0; i < 7; i++){
		rv_cmdstr[7] += rv_cmdstr[i];
	}
	rv_cmdstr[8] = 0x03;


#if (0) //Test
	rv_cmdstr[0] = 0x02;
	rv_cmdstr[1] = 0x50;
	rv_cmdstr[2] = 0x05;
	rv_cmdstr[3] = 0x00;
	rv_cmdstr[4] = 0x52;
	rv_cmdstr[5] = 0x56;
	rv_cmdstr[6] = 0x09;
	rv_cmdstr[7] = 0x08;
	rv_cmdstr[8] = 0x03;
#endif

	total_error_cnt = 0;
	do {
		read_cnt = 0;
//read_bytes = sizeof(tacom_inno_sv_t);
read_bytes = 512;
		total_read_bytes = 0;
		if(power_get_ignition_status() == POWER_IGNITION_OFF)
			break;

		if(total_error_cnt++ > 10) {
			close(dtg_uart_fd);
			dtg_uart_fd = -1;
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, B115200);
			total_error_cnt = 0;
		}

//printf("RV Command Set\n");
//for(i = 0; i < 9; i++)
//	printf("%02x", rv_cmdstr[i]);
//printf("\n\n");
		if(dtg_uart_fd > 0)
			uart_write(dtg_uart_fd, rv_cmdstr, 9); //hdr_cmdstr : RV

		sleep(1);
		do{
			read_cnt += 1;
			if(read_cnt > 5)
				break;
			ret = 0;
			if(dtg_uart_fd > 0)
				ret = _wait_read(dtg_uart_fd, (unsigned char *)&buf[total_read_bytes], read_bytes, 5);

			if(ret > 0) {
				total_read_bytes += ret;
				if(total_read_bytes >= sizeof(tacom_inno_sv_t))
					break;

				if(total_read_bytes == sizeof(tacom_inno_old_sv_t)) //old version
					break;

				read_bytes = sizeof(tacom_inno_sv_t) - total_read_bytes;
			}
			else
			{
				DTG_LOGE("Request Setting Value Can't Ack from DTG #1 ret[%d/%d], retry[%d]", total_read_bytes, sizeof(tacom_inno_sv_t), read_cnt);
				//sleep(3);
				//continue;
				break;
				
			}

		} while(1);

//printf("sizeof(tacom_inno_sv_t) = [%d]\n", sizeof(tacom_inno_sv_t));
//printf("sizeof(tacom_inno_old_sv_t) = [%d]\n", sizeof(tacom_inno_old_sv_t));
//printf("total_read_bytes = [%d]\n", total_read_bytes);

		if(total_read_bytes == sizeof(tacom_inno_sv_t)) 
		{
			ret = valid_check((unsigned char *)buf, sizeof(tacom_inno_sv_t) - 1);
			if (ret == 0){
				p_set_value = (tacom_inno_sv_t *)buf;
				g_K_Factor = p_set_value->k_factor;
				g_RPM_Factor = p_set_value->rpm_factor;
				printf("================================>\n");
				printf("g_K_Factor = [%d]\n", g_K_Factor);
				printf("g_RPM_Factor = [%d]\n", g_RPM_Factor);
				break;
			}
			else
			{
				DTG_LOGE("Request Setting Value Invalid Data Received : [%d]", ret);
				sleep(3);
			}
		} 
		else if(total_read_bytes == sizeof(tacom_inno_old_sv_t)) 
		{
			ret = valid_check((unsigned char *)buf, sizeof(tacom_inno_old_sv_t) - 1);
			if (ret == 0){
				p_old_set_value = (tacom_inno_old_sv_t *)buf;
				g_K_Factor = p_old_set_value->k_factor;
				g_RPM_Factor = p_old_set_value->rpm_factor;
				printf("================================>\n");
				printf("Old Version g_K_Factor = [%d]\n", g_K_Factor);
				printf("Old Version g_RPM_Factor = [%d]\n", g_RPM_Factor);
				break;
			}
			else
			{
				DTG_LOGE("OLD Request Setting Value Invalid Data Received : [%d]", ret);
				sleep(3);
			}
		}
		else 
		{
			DTG_LOGE("Request Setting Value Received Length Error [%d/%d]", total_read_bytes, sizeof(tacom_inno_sv_t));
			sleep(3);
		}
	} while(ret <= 0);
}

void request_header_info()
{
	int total_error_cnt = 0;
	int i;
	int ret = 0;
	int read_cnt = 0;
	int read_bytes = 0;
	int total_read_bytes = 0;
	unsigned char buf[512];
	unsigned short data_len = 0x0005;
	unsigned char hdr_cmdstr[9];
	memset(hdr_cmdstr, 0, sizeof(hdr_cmdstr));
	hdr_cmdstr[0] = 0x02;
	hdr_cmdstr[1] = 0x50;
	memcpy(&hdr_cmdstr[2], &data_len, 2);
	strncpy(&hdr_cmdstr[4], "RI", 2);
	hdr_cmdstr[6] = 0x0;
	for(i = 0; i < 7; i++){
		hdr_cmdstr[7] += hdr_cmdstr[i];
	}
	hdr_cmdstr[8] = 0x03;

	total_error_cnt = 0;
	do {
		read_cnt = 0;
		read_bytes = sizeof(tacom_inno_hdr_t);
		total_read_bytes = 0;

		if(power_get_ignition_status() == POWER_IGNITION_OFF)
			break;

		if(dtg_uart_fd < 0) {
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, B115200);
			if(dtg_uart_fd < 0) {
				DTG_LOGE("UART OPEN ERROR INNOCAR");
				sleep(1);
				continue;
			}
		}

		if(total_error_cnt++ > 10) {
			close(dtg_uart_fd);
			dtg_uart_fd = -1;
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, B115200);
			total_error_cnt = 0;
		}

//printf("RI Command Set\n");
//for(i = 0; i < 9; i++)
//	printf("%02x", hdr_cmdstr[i]);
//printf("\n\n");
		if(dtg_uart_fd > 0)
			uart_write(dtg_uart_fd, hdr_cmdstr, 9); //hdr_cmdstr : RI

		sleep(1);
		do{
			read_cnt += 1;
			if(read_cnt > 5)
				break;

			ret = 0;
			if(dtg_uart_fd > 0)
				ret = _wait_read(dtg_uart_fd, (unsigned char *)&buf[total_read_bytes], read_bytes, 5);

			if(ret > 0) {
				total_read_bytes += ret;
				if(total_read_bytes >= sizeof(tacom_inno_hdr_t))
					break;

				read_bytes = sizeof(tacom_inno_hdr_t) - total_read_bytes;
			}
			else
			{
				DTG_LOGE("%s(fd:%d): Header Request Can't Ack from DTG #1 ret[%d/%d] retry[%d]", DTG_TTY_DEV_NAME, dtg_uart_fd, total_read_bytes, sizeof(tacom_inno_hdr_t), read_cnt);
				//sleep(3);
				//continue;
				break;
				
			}

		} while(1);

		if(total_read_bytes == sizeof(tacom_inno_hdr_t)) 
		{
			//unsigned char *tmp;
			//int i;
			//tmp = (unsigned char *)&inno_header;
			//for(i = 0; i < sizeof(tacom_inno_hdr_t); i++)
			//	printf("%02x ", tmp[i]);
			//printf("\n");
			
			ret = valid_check((unsigned char *)buf, sizeof(tacom_inno_hdr_t) - 1);
			if (ret == 0){
				memcpy(&inno_header, buf, sizeof(tacom_inno_hdr_t));
				inno_header_status = INNO_HEADER_FULL;
				_header_tmp = 1;
				break;
			}
			else
			{
				DTG_LOGE("Header Request Invalid Data Received : [%d]", ret);
				sleep(3);
			}
		} else 
		{
			memset(&inno_header, 0, sizeof(tacom_inno_hdr_t));
			DTG_LOGE("Header Request Received Length Error [%d/%d]", total_read_bytes, sizeof(tacom_inno_hdr_t));
			sleep(3);
		}
	} while(ret <= 0);

}

void request_data_info()
{
	int total_error_cnt = 0;
	int i;
	int ret = 0;
	int read_cnt = 0;
	int read_bytes = 0;
	int total_read_bytes = 0;
	unsigned char buf[512];
	unsigned short data_len = 0x0005;
	unsigned char dtg_cmdstr[9];

	//tacom_inno_data_t *p_inno_data;

	memset(dtg_cmdstr, 0, sizeof(dtg_cmdstr));
	dtg_cmdstr[0] = 0x02;
	dtg_cmdstr[1] = 0x50;
	memcpy(&dtg_cmdstr[2], &data_len, 2);
	strncpy(&dtg_cmdstr[4], "RU", 2);
	dtg_cmdstr[6] = 0x0;
	for(i = 0; i < 7; i++){
		dtg_cmdstr[7] += dtg_cmdstr[i];
	}
	dtg_cmdstr[8] = 0x03;


	total_error_cnt = 0;
	do {
		read_cnt = 0;
		read_bytes = sizeof(tacom_inno_data_t);
		total_read_bytes = 0;

		if(power_get_ignition_status() == POWER_IGNITION_OFF)
			break;

		if(dtg_uart_fd < 0) {
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, B115200);
			if(dtg_uart_fd < 0) {
				DTG_LOGE("UART OPEN ERROR INNOCAR");
				sleep(1);
				continue;
			}
		}


		if(total_error_cnt++ > 10) {
			close(dtg_uart_fd);
			dtg_uart_fd = -1;
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, B115200);
			total_error_cnt = 0;
		}
/*
printf("RU Command Set\n");
for(i = 0; i < 9; i++)
	printf("%02x", dtg_cmdstr[i]);
printf("\n\n");
*/
		if(dtg_uart_fd > 0)
			uart_write(dtg_uart_fd, dtg_cmdstr, 9); //data_cmdstr : RU

		do{
			read_cnt += 1;
			if(read_cnt > 5)
				break;

			ret = 0;
			if(dtg_uart_fd > 0)
				ret = _wait_read(dtg_uart_fd, (unsigned char *)&buf[total_read_bytes], read_bytes, 5);

//printf("data read len = [%d]\n", ret);
			if(ret > 0) {
				total_read_bytes += ret;
				//DTG_LOGE("Data Request Received Data[%d/%d]", total_read_bytes, sizeof(tacom_inno_data_t));
				if(total_read_bytes >= sizeof(tacom_inno_data_t))
					break;

				read_bytes = sizeof(tacom_inno_data_t) - total_read_bytes;
			}
			else
			{
				DTG_LOGE("Data Request Can't Ack from DTG #1 ret[%d/%d], retry[%d]", total_read_bytes, sizeof(tacom_inno_data_t), read_cnt);
				//sleep(3);
				//continue;
				break;
				
			}

		} while(1);

		if(total_read_bytes == sizeof(tacom_inno_data_t)) 
		{
			ret = valid_check((unsigned char *)&buf, sizeof(tacom_inno_data_t) - 1);
			if(ret != 0) {
				DTG_LOGE("Data Request Invalid Data Received : [%d]", ret);
				sleep(1);
				continue;
			}
			
			if(memcmp(&buf[6], &read_curr_buf.data_t_y, 6) != 0)
			{
				printf("[%04d/%02d/%02d %02d:%02d:%02d]\n", buf[6]+2000, buf[7], buf[8], buf[9], buf[10], buf[11]);
				//p_old = (tacom_inno_data_t *)read_curr_buf;
#ifdef GPS_FIXED_IN_SPEED_ZERO
				tacom_inno_data_t *p_inno_data = (tacom_inno_data_t *)buf;
				if(p_inno_data->speed == 0) {
					if(read_curr_buf.latitude != 0 && read_curr_buf.longitude != 0) {
						p_inno_data->latitude = read_curr_buf.latitude;
						p_inno_data->longitude = read_curr_buf.longitude;
						p_inno_data->checksum = 0xFF;
					}
				}
#endif

				store_recv_bank(buf, sizeof(tacom_inno_data_t), (unsigned char *)&read_curr_buf);
#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
				g_mdt_count += 1;
				if(g_mdt_count >= g_mdt_collection_period) {
					g_mdt_count = 0;
					mdt_store_recv_bank(buf, sizeof(tacom_inno_data_t));
				}
#endif
				break;
			}
			//else
			//{
			//	DTG_LOGE("Same Data Received [%04d/%02d/%02d %02d:%02d:%02d]", buf[6]+2000, buf[7], buf[8], buf[9], buf[10], buf[11]);
			//}
		} 
		else 
		{
			DTG_LOGE("Data Request Received Length Error [%d/%d]", total_read_bytes, sizeof(tacom_inno_data_t));
			memset(buf, 0, sizeof(tacom_inno_data_t));
			sleep(1);
		}
	} while(1);

}

static void *inno_recv_data_thread (void *pargs)
{
	int led_flag = 0;

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	// w200_led_init(W200_LED_GPS);
	// w200_led_set_color(W200_LED_GPS, W200_LED_G);
#endif

	memset(&inno_header, 0, sizeof(tacom_inno_hdr_t));

	saved_data_recovery("/var/ino_stored_records", (unsigned char *)&read_curr_buf);

	#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		wait_taco_unill_power_on();
	#endif

	request_header_info();
	request_setting_info();

	while(1){
		#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
			wait_taco_unill_power_on();
		#endif

		if(inno_header_status != INNO_HEADER_FULL)
		{
			request_header_info();
			request_setting_info();
		}

		request_data_info();
		if(led_flag == 0)
		{
			// w200_led_on(W200_LED_GPS);
			led_flag = 1;
		}
		else
		{
			// w200_led_off(W200_LED_GPS);
			led_flag = 0;
		}

		usleep(900*1000);
	}
	destory_dtg_store();
#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	destory_mdt_store();
#endif
}

pthread_t tid_recv_data;

#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
int setup_server_config_data(char *file_name, char *item)
{
	int ret = 0;
	if(open_ini_file(file_name) < 0) {
		printf("setup_server_config_data======> [%s], error #1\n", file_name);
		return -1;
	}

	ret = get_ini_int(item, &g_mdt_collection_period);
	if(ret < 0)
	{
		close_ini_file();
		printf("setup_server_config_data======> [%s], error #2\n", file_name);
		return -2;
	}

	printf("setup_server_config_data======>[%s]success\n", file_name);
	return ret;
}
#endif

int inno_init_process()
{
#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	if(setup_server_config_data(CONFIG_FILE_PATH, CREATE_MDT_PERIOD) < 0) {
		setup_server_config_data(CONFIG_FILE_PATH_ORG, CREATE_MDT_PERIOD);
	}

	printf("=======================================>\n");
	printf("g_mdt_collection_period = [%d]\n", g_mdt_collection_period);
	printf("=======================================>\n");
#endif

	create_dtg_data_store(MAX_INNO_DATA_PACK);

#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	create_mdt_data_store(MAX_INNO_MDT_DATA_PACK);
#endif

	if (pthread_create(&tid_recv_data, NULL, inno_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create inno_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int inno_unreaded_records_num ()
{

#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	return ((get_mdt_current_count() << 16) & 0xFFFF0000) | (get_dtg_current_count() & 0x0000FFFF);
#else
	return get_dtg_current_count();
#endif

}

#define GPS_DEGREE		(1000000)
#define GPS_FIX_LENGTH	(6)
unsigned long gps_chnage(unsigned long gps_pos)
{
	char tmp_gps[64];
	unsigned long result = 0;
	int front_gps;
	int rear_gps;
	int len;
	int i;
	int gps_cvt;

	if(gps_pos == 0)
		return 0;

	front_gps = gps_pos / GPS_DEGREE;
	rear_gps = gps_pos - (front_gps*GPS_DEGREE);

	gps_cvt = rear_gps / 0.6;

	sprintf(tmp_gps, "%06d", gps_cvt);
	len = strlen(tmp_gps);
	if(len > GPS_FIX_LENGTH)
	{
		tmp_gps[GPS_FIX_LENGTH] = 0x00;
		gps_cvt = atoi(tmp_gps);
	}

	sprintf(tmp_gps, "%03d%06d",front_gps, gps_cvt);
	result = atoi(tmp_gps);
	if(result < 0) result = 0;
	
	return result;
}

int data_convert(tacom_std_data_t *std_data, unsigned char *dtg_data, int debug_flag) {
	int ret = 0;
	unsigned long latitude;
	unsigned long longitude;
	tacom_inno_data_t *inno_data;

	inno_data = (tacom_inno_data_t *)dtg_data;

	ret = valid_check((unsigned char *)inno_data, sizeof(tacom_inno_data_t) - 1);
	if(ret < 0) {
		DTG_LOGE("wrong dtg data");
		return ret;
	}
	
	sprintf(std_data->day_run_distance, "%04ld",inno_data->day_dist/10);
	sprintf(std_data->cumulative_run_distance, "%07ld",inno_data->cumul_dist/10);

	sprintf(std_data->date_time, "%02d%02d%02d%02d%02d%02d%02d"
		, inno_data->data_t_y, inno_data->data_t_mon
		, inno_data->data_t_d, inno_data->data_t_h
		, inno_data->data_t_min, inno_data->data_t_s
		, inno_data->data_t_cs);
	sprintf(std_data->speed, "%03d",inno_data->speed);
	sprintf(std_data->rpm, "%04d",inno_data->rpm);

	std_data->bs = (inno_data->ex_input & 0x1) | 0x30; //0x30 is to make ascii (0x30('0'), 0x31('1'))

	//x : latitude(����)
	//y : longitude(�浵)
	if(debug_flag == 0)
		DTG_LOGT("DTG ORIGINAL GPS x, y = %ld, %ld", inno_data->latitude, inno_data->longitude);

	latitude = gps_chnage(inno_data->latitude);
	sprintf(std_data->gps_x, "%09ld", latitude);
	longitude = gps_chnage(inno_data->longitude);
	sprintf(std_data->gps_y, "%09ld",longitude);

	if(debug_flag == 0)
		DTG_LOGT("DTG CONVERT GPS x, y = %ld, %ld", latitude, longitude);
	
	sprintf(std_data->azimuth, "%03d",inno_data->azimuth);
	if(inno_data->accelation_x >= 0){
		sprintf(std_data->accelation_x, "+%03d.%d", 
			inno_data->accelation_x/10, inno_data->accelation_x%10);
	} else {
		sprintf(std_data->accelation_x, "%04d.%d", 
			inno_data->accelation_x/10, abs(inno_data->accelation_x)%10);
	}
	if(inno_data->accelation_y >= 0){
		sprintf(std_data->accelation_y, "+%03d.%d", 
			inno_data->accelation_y/10, inno_data->accelation_y%10);
	} else {
		sprintf(std_data->accelation_y, "%04d.%d", 
			inno_data->accelation_y/10, abs(inno_data->accelation_y)%10);
	}

	sprintf(std_data->status, "%02d",inno_data->status);

	/* innocar's dtg does not have external data. */
	/* so external data is '0'. */

	//memset(std_data->day_oil_usage, '0', 9);
	sprintf(std_data->day_oil_usage, "%09ld",inno_data->dalily_oil_usage);
	//memset(std_data->cumulative_oil_usage, '0', 9);
	sprintf(std_data->cumulative_oil_usage, "%09ld",inno_data->cumul_oil_usage);
	memcpy(std_data->temperature_A, "+00.0", 5);
	memcpy(std_data->temperature_B, "+00.0", 5);
	memset(std_data->residual_oil, '0', 7);

	std_data->weight1 = inno_data->weight1;
	std_data->weight2 = inno_data->weight2;

	std_data->k_factor = g_K_Factor;
	std_data->rpm_factor = g_RPM_Factor;

	//printf("%d, %d, %d, %d\n", std_data->weight1, std_data->weight2, std_data->k_factor, std_data->rpm_factor);

	return ret;
}

void save_record_data()
{
	save_record_data_taco("/var/ino_stored_records");
}



static int std_parsing(TACOM *tm, int request_num, int file_save_flag)
{
	int dest_idx = 0;
	int r_num = 0;
	int ret, i;
	int unread_count;
	
	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;
	tacom_inno_data_t *inno_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	unread_count = get_dtg_current_count();
	DTG_LOGD("std_parsing> inno_unreaded_records_num = [%d]\n", unread_count);
	if(unread_count <= 0)
		return -1;


	while (inno_header_status == INNO_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> inno_header_status is INNO_HEADER_EMPTY\n");
		sleep(2);
	}
	do {
		ret = valid_check((unsigned char *)&inno_header, sizeof(tacom_inno_hdr_t) - 1);
		if (ret < 0) {
			DTG_LOGE("dtg header is empty");
			inno_header_status = INNO_HEADER_EMPTY;
			sleep(3);
		}
	} while (ret < 0);

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];


	//ECO DTG-1000H151019F
	if(!strncmp(inno_header.dtg_model, "00000000ECO DTG-1000", 20)) {
		//old version
		memcpy(std_hdr->vehicle_model, inno_header.dtg_model, 20);
		strcpy(std_hdr->dtg_fw_ver, "OLD_VER");
	}
	else {
		//model : [0][0][0][0][0][0][0][E][C][O][ ][D][T][G][-][1][0][0][0][H]
		//dtg_fw_ver : [1][5][1][0][1][9][F]

		//new version
		memset(std_hdr->vehicle_model, '0', 20);
		memcpy(&std_hdr->vehicle_model[7], inno_header.dtg_model, 13);
		//char dtg_fw_ver[8];
		memset(std_hdr->dtg_fw_ver, 0x00, sizeof(std_hdr->dtg_fw_ver));
		memcpy(std_hdr->dtg_fw_ver, &inno_header.dtg_model[13], 7);
		std_hdr->dtg_fw_ver[7] = 0x00;
	}

	memcpy(std_hdr->vehicle_id_num, inno_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, inno_header.vehicle_type, 2);
	ri_to_uni(inno_header.regist_num, std_hdr->registration_num);
	memcpy(std_hdr->business_license_num, inno_header.business_lsn, 10);
	memcpy(std_hdr->driver_code, inno_header.driver_code, 18);

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
		inno_data = &read_curr_buf;
		ret = data_convert(std_data, (unsigned char *)inno_data, 0);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
			dest_idx = get_dtg_data(tm, dest_idx);
	}

	DTG_LOGD("Stream Size HDR[%d] + DATA[%d] : [%d]", 
			sizeof(tacom_std_hdr_t), sizeof(tacom_std_data_t) * request_num, dest_idx);
	DTG_LOGD("Size : [%d]", dest_idx);

	return dest_idx;
}


#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
static int std_mdt_parsing(TACOM *tm, int request_num)
{
	int dest_idx = 0;
	int ret;
	int unread_count;
	
	tacom_std_hdr_t *std_hdr;
	//tacom_std_data_t *std_data;
	//tacom_inno_data_t *inno_data;

	unread_count = get_mdt_current_count();
	DTG_LOGD("std_mdt_parsing> inno_unreaded_records_num = [%d]\n", unread_count);
	if(unread_count <= 0)
		return -1;


	while (inno_header_status == INNO_HEADER_EMPTY) {
		DTG_LOGD("std_mdt_parsing> inno_header_status is INNO_HEADER_EMPTY\n");
		sleep(2);
	}
	do {
		ret = valid_check((unsigned char *)&inno_header, sizeof(tacom_inno_hdr_t) - 1);
		if (ret < 0) {
			DTG_LOGE("dtg header is empty");
			inno_header_status = INNO_HEADER_EMPTY;
			sleep(3);
		}
	} while (ret < 0);

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];


	//ECO DTG-1000H151019F
	if(!strncmp(inno_header.dtg_model, "00000000ECO DTG-1000", 20)) {
		//old version
		memcpy(std_hdr->vehicle_model, inno_header.dtg_model, 20);
		strcpy(std_hdr->dtg_fw_ver, "OLD_VER");
	}
	else {
		//model : [0][0][0][0][0][0][0][E][C][O][ ][D][T][G][-][1][0][0][0][H]
		//dtg_fw_ver : [1][5][1][0][1][9][F]

		//new version
		memset(std_hdr->vehicle_model, '0', 20);
		memcpy(&std_hdr->vehicle_model[7], inno_header.dtg_model, 13);
		//char dtg_fw_ver[8];
		memset(std_hdr->dtg_fw_ver, 0x00, sizeof(std_hdr->dtg_fw_ver));
		memcpy(std_hdr->dtg_fw_ver, &inno_header.dtg_model[13], 7);
		std_hdr->dtg_fw_ver[7] = 0x00;
	}

	memcpy(std_hdr->vehicle_id_num, inno_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, inno_header.vehicle_type, 2);
	ri_to_uni(inno_header.regist_num, std_hdr->registration_num);
	memcpy(std_hdr->business_license_num, inno_header.business_lsn, 10);
	memcpy(std_hdr->driver_code, inno_header.driver_code, 18);

	dest_idx += sizeof(tacom_std_hdr_t);
	dest_idx = mdt_dtg_data(tm, dest_idx, g_mdt_collection_period);

	DTG_LOGD("std_mdt_parsing> Stream Size HDR[%d] + DATA[%d] : [%d]", 
			sizeof(tacom_std_hdr_t), sizeof(tacom_std_data_t) * request_num, dest_idx);
	DTG_LOGD("Size : [%d]", dest_idx);

	return dest_idx;
}
#endif

int inno_read_current(){
	int ret = 0;
	TACOM *tm = tacom_get_cur_context();

	ret = valid_check((unsigned char *)&read_curr_buf, sizeof(tacom_inno_data_t) - 1);
	if(ret < 0) {
		DTG_LOGE("wrong dtg data %s : %d : %d",__func__, __LINE__, sizeof(tacom_inno_data_t));
		return -1;
	}
	return std_parsing(tm, 1, 0);
}

int inno_read_records (int r_num)
{
	int ret;
	int tmp_value;
	printf("requst_num ================> 0x%08x\n", r_num);
	if (r_num == 0x10000000)
	{
		ret = std_parsing(tm, 0, 1); //only data file save
	}
	else if (r_num & 0x20000000) 
	{
		tmp_value = (r_num & 0x0000FFFF);
		if(tmp_value == 1) ////refresh header & setting value
			inno_header_status = INNO_HEADER_EMPTY;
#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
		else if(tmp_value == 2) //mdt period time change
			setup_server_config_data(CONFIG_FILE_PATH, CREATE_MDT_PERIOD);
#endif
		
		ret = 0;
	}
	else if (r_num & 0x40000000) //read mdt data.
	{
#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
		tmp_value = (r_num & 0x0000FFFF);
		ret = std_mdt_parsing(tm, tmp_value);
#endif
	}
	else
	{
		
		ret = std_parsing(tm, r_num, 0);
	}
	return ret;
}

int inno_ack_records(int readed_bytes)
{
#if defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	if(readed_bytes == 2)
		mdt_data_clear();
	else
		dtg_data_clear();
#else
	dtg_data_clear();

#endif

	
	return 0;
}

const struct tm_ops inno_ops = {
	inno_init_process,
	NULL,
	NULL,
	NULL,
	NULL,
	inno_read_current,
	NULL,
	inno_unreaded_records_num,
	inno_read_records,
	NULL,
	inno_ack_records,
};

