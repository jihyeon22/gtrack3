/**
* @file tacom_daesin.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-22
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h> //htons

#include <wrapper/dtg_log.h>
#include <tacom_internal.h>
#include <tacom_protocol.h>

#include <tacom_daesin_protocol.h>
#include <common/crc16.h>
#include <common/power.h>
#include <common/w200_led.h>
#include "uart.h"
#include "convtools.h"
#include "utill.h"

#include <time.h>
#include <common/modem-time.h>

#define UART_REOPEN_ENABLE_FLAG

struct tacom_setup daesin_setup  = {
	.tacom_dev				= "TACOM DAESIN : ",
	.cmd_rl					= "800201",
	.cmd_rs					= NULL,
	.cmd_rq					= NULL,
	.cmd_rr					= NULL,
	.cmd_rc					= NULL,	
	.cmd_ack				= NULL,
	.data_rl				= 0,
	.data_rs				= 0,
	.data_rq				= 0,
	.start_mark				= '>',
	.end_mark				= '<',
	.head_length			= 79,
	.head_delim				= ',',
	.head_delim_index		= 80,
	.head_delim_length		= 1,
	.record_length			= 53,
	.max_records_size		= (MAX_DAESIN_DATA * MAX_DAESIN_DATA_PACK), // this constant value have to set over max_records_per_once's value.
#if defined(SERVER_MODEL_ETRS) || defined(SERVER_MODEL_ETRS_TB)
	.max_records_per_once	= 200,
#else
	.max_records_per_once	= 470,
#endif
	.conf_flag				= 0x5,
};

extern int dtg_uart_fd;
static tacom_daesin_hdr_t daesin_header = {0, };

#define DAESIN_HEADER_EMPTY 0
#define DAESIN_HEADER_FULL 1
static int daesin_header_status = DAESIN_HEADER_EMPTY;

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_DAESIN_DATA_PACK;
static daesin_data_pack_t recv_bank[MAX_DAESIN_DATA_PACK];

static tacom_daesin_data_t read_curr_buf;
int daesin_ack_records(int readed_bytes);

static void store_recv_bank(char *buf, int size)
{
	tacom_daesin_data_t *daesin_data_recv_buf;

	if(recv_bank[end].count >= MAX_DAESIN_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(daesin_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(daesin_data_pack_t));
	}

	if ((size == sizeof(tacom_daesin_data_t)) && (recv_avail_cnt > 0)) {
		daesin_data_recv_buf = (tacom_daesin_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)daesin_data_recv_buf, buf, sizeof(tacom_daesin_data_t));
		memcpy((char *)&read_curr_buf, buf, sizeof(tacom_daesin_data_t));
		recv_bank[end].count++;
		if (recv_bank[end].count >= MAX_DAESIN_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_DAESIN_DATA_PACK) {
					end = 0;
				}
				memset(&recv_bank[end], 0x00, sizeof(daesin_data_pack_t));
			}
			else
			{
				//jwrho 2015.05.13 ++
				//when buffer is full, most old data remove first.
				//cy_data_recv_buf = (tacom_cy_data_t *)&recv_bank[curr].buf[0];
				//printf("Remove Data : ");
				//for(i = 0; i < 14; i++)
				//	printf("%c", cy_data_recv_buf->date_time[i]);
				//printf("\n");

				end = curr;
				memset(&recv_bank[curr], 0x00, sizeof(cy_data_pack_t));
				curr += 1;
				recv_avail_cnt += 1;
				if  (curr == MAX_CY_DATA_PACK)
					curr = 0;
			}
		} else {
			//recv_bank[end].status = DATA_PACK_AVAILABLE;
			recv_bank[end].status = DATA_PACK_EMPTY;
		}
	}
}

void saved_data_recovery()
{
	int fd;
	int ret;
	char buf[128] = {0};

	if(check_file_exist("/var/da_stored_records") == 0) {
		fd = open("/var/da_stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, buf, sizeof(tacom_daesin_data_t));
				if(ret == sizeof(tacom_daesin_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(buf, sizeof(tacom_daesin_data_t));
					}
				} else {
					break;
				}
			}
			close(fd);
		}
		unlink("/var/da_stored_records");
	}
}

int buffer_rollback(unsigned char *dest, unsigned char *tmp, int size)
{
	int i;
	int ret = 0;
	for(i = 1; i < size; i++) {
		if(dest[i] == '>') {
			memcpy(tmp, &dest[i], size - i);
			memset(dest, 0x00, size);
			memcpy(dest, tmp, size - i);
			ret = size - i;
			break;
		}
	}

	if(i == size) {
		ret = 0;
		fprintf(stderr, "Next '>' Can't find\n");
	}

	return ret;
}
extern dtg_status_t dtg_status;

int dtg_info_file_save(char *file_path, tacom_daesin_hdr_t new_header)
{
	int fd;
	tacom_daesin_hdr_t daesin_header_tmp;

	if(check_file_exist("/var/daesin_dtg_hd") >= 0) {
		fd = open("/var/daesin_dtg_hd", O_RDONLY, 0644);
		if(fd > 0) {
			if(read(fd, &daesin_header_tmp, sizeof(tacom_daesin_hdr_t)) >= sizeof(tacom_daesin_hdr_t)) {
				if(!memcmp(&daesin_header_tmp, &new_header, sizeof(tacom_daesin_hdr_t))) {
					fprintf(stderr, "============================================\n");
					fprintf(stderr, "DTG INFO NO NEED UPDATE\n");
					fprintf(stderr, "============================================\n");
					return 0; //no need new file save.
				}
			}
			close(fd);
		}
	}

	//new dtg information update
	fd = open("/var/daesin_dtg_hd", O_WRONLY | O_CREAT, 0644);
	if(fd > 0) {
		write(fd, &new_header, sizeof(tacom_daesin_hdr_t));
		close(fd);
	} else {
		DTG_LOGE("/var/daesin_dtg_hd file save error");
	}
	return 0;
}

void request_dtg_header_info()
{
	int fuart_check = 0;
	int ret;
	int ri_fd = 0;
	int uart_err_cnt = 5;

	memset(&daesin_header, 0, sizeof(tacom_daesin_hdr_t));
	load_dtg_status();

	//daesin_header default value ++
	daesin_header.start_m = '>';
	memset(daesin_header.dtg_model, '0', 20);;
	memset(daesin_header.vehicle_id_num, '0', 17);
	memset(daesin_header.vehicle_type, '0', 2);
	memset(daesin_header.regist_num, '0', 12);
	memset(daesin_header.business_num, '0', 10);
	memset(daesin_header.driver_code, '0', 18);
	memset(daesin_header.crc16_d, '0', 2);
	daesin_header.end_m = '<';
	//daesin_header default value --


#ifdef UART_REOPEN_ENABLE_FLAG
	if(dtg_uart_fd > 0) {
		close(dtg_uart_fd);
		dtg_uart_fd = -1;

	}
	while(1)
	{
		if(dtg_uart_fd < 0) {
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, 115200);
			if(dtg_uart_fd < 0 ) {
				DTG_LOGE("UART OPEN ERROR DAESIN");
				sleep(1);
				continue;
			}

			break; //uart open success
		}
	}
#endif
	//fuart_check : 
	//            0   : time out
	//            > 0 : data received
	//            < 0 : uard error
	while((fuart_check = uart_check(dtg_uart_fd, 2, 5)) < 0) {
		DTG_LOGE("uart_check error no.1");
		sleep(1);
	}

	DTG_LOGD("================================================");
	DTG_LOGD("fuart_check = [%d]", fuart_check);
	DTG_LOGD("================================================");
	
	if(fuart_check == 0) { //no data
		while(1) {
			sleep(3);
			//[3e][52][49][30][b1][3c] : RI
			if(send_cmd("RI", &daesin_setup, NULL) < 0) {
				DTG_LOGE("%s:%d> RI Send CMD Fail\n", __func__, __LINE__);
				continue;
			} else { 
				ret = uart_read(dtg_uart_fd, (unsigned char *)&daesin_header, sizeof(tacom_daesin_hdr_t), 5);
				if(ret == sizeof(tacom_daesin_hdr_t)) {
					ret = crc_check((unsigned char *)&daesin_header, sizeof(tacom_daesin_hdr_t), &daesin_setup);
					if (ret == 0){
						dtg_info_file_save("/var/daesin_dtg_hd", daesin_header);
						daesin_header_status = DAESIN_HEADER_FULL;
						break; //success.
					}
				}
				else
				{
					DTG_LOGE("%s:%d> RI Received Data Fail\n", __func__, __LINE__);
				}
			} 
		}
	} else {
		ri_fd = 0;
		ret = 0;
		if(check_file_exist("/var/daesin_dtg_hd") >= 0) {
			ri_fd = open("/var/daesin_dtg_hd", O_RDONLY, 0644);
		}
		else {
			DTG_LOGE("/var/daesin_dtg_hd file don't exist");
		}
		if(ri_fd > 0) {
			ret = read(ri_fd, &daesin_header, sizeof(tacom_daesin_hdr_t));
			close(ri_fd);
			ri_fd = 0;
		}

		if(ret > 0) {
			DTG_LOGD("%s:%d> Get RI Info From File\n", __func__, __LINE__);
		}
		else {
			DTG_LOGD("%s:%d> Get RI Info From Default Value\n", __func__, __LINE__);
		}
		daesin_header_status = DAESIN_HEADER_FULL;
	}

	memcpy(dtg_status.vrn, daesin_header.regist_num, 12);
	store_dtg_status();
}

void request_broadcast_start()
{
	int ret;
	int retry_cnt;
	unsigned char outbuf[128];
	retry_cnt = 0;
	while(1) 
	{
		//[3e][52][53][4b][02][3c] : RS
		ret = send_cmd("RS", &daesin_setup, NULL);
		if(ret < 0) {
			DTG_LOGD("%s:%d> RS Send CMD Fail\n", __func__, __LINE__);
			continue;
		}		

		if( uart_read(dtg_uart_fd, (unsigned char *)outbuf, 2, 2) > 0) {
			DTG_LOGD("%s:%d> RS Send CMD Complete\n", __func__, __LINE__);
			break; //send command SUCCESS
		} else {
			DTG_LOGD("%s:%d> No Data, Retry RS CMD\n", __func__, __LINE__);
		}
		sleep(2);
		retry_cnt += 1;
		if(retry_cnt > 10)
			break;
	}
}


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
			if(ign_status == POWER_IGNITION_ON)
			{
				request_dtg_header_info();
				request_broadcast_start();
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



int data_extract(unsigned char *dest, int dest_len, unsigned char *un_do_buf, int un_do_buf_size, int current_undo_bufer_length)
{
	static int invalid_data_cnt = 0;
	static int led_init = 0;
	static int led_flag = 0;
	unsigned char op_data_buf[1024];
	int i;
	int ret;

	struct tm cur_time;
	time_t system_time;
	struct tm *timeinfo;
	tacom_daesin_data_t *p_daesin_data;


#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	if(led_init == 0)
	{
		// w200_led_init(W200_LED_GPS);
		// w200_led_set_color(W200_LED_GPS, W200_LED_G);
		led_init = 1;
	}
#endif

	memset(op_data_buf, 0x00, sizeof(op_data_buf));
	if(current_undo_bufer_length > 0)
	{
		memcpy(op_data_buf, un_do_buf, current_undo_bufer_length);
		memcpy(&op_data_buf[current_undo_bufer_length], dest, dest_len);
		dest_len += current_undo_bufer_length;
	}
	else
	{
		memcpy(op_data_buf, dest, dest_len);
	}

	for(i = 0; i < dest_len; i++) 
	{
		if( (i + sizeof(tacom_daesin_data_t) ) > dest_len)
		{
			memset(un_do_buf, 0x00, un_do_buf_size);
			memcpy(un_do_buf, &op_data_buf[i], dest_len-i);
			return (dest_len-i);
		}

		if(op_data_buf[i] == '>')
		{
			if( (i + sizeof(tacom_daesin_data_t)) <= dest_len)
			{
				ret = crc_check((unsigned char *)&op_data_buf[i], sizeof(tacom_daesin_data_t), &daesin_setup);
				if(ret < 0) {
					DTG_LOGE("CRC ERROR");
					if(invalid_data_cnt > 30)
					{
						// w200_led_set_color(W200_LED_GPS, W200_LED_R);
						// w200_led_on(W200_LED_GPS);
					}
					else
					{
						invalid_data_cnt += 1;
					}
				}
				else
				{
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
					invalid_data_cnt = 0;
					if(daesin_header_status == DAESIN_HEADER_FULL)
					{
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
					}
					p_daesin_data = (tacom_daesin_data_t *)&op_data_buf[i];
					
					/*
					printf("p_daesin_data gps_date : %02d%02d%02d%02d%02d%02d%02d\n",	p_daesin_data->year, 
																						p_daesin_data->mon, 
																						p_daesin_data->day,
																						p_daesin_data->hour, 
																						p_daesin_data->min, 
																						p_daesin_data->sec,
																						p_daesin_data->m_sec);
					*/
					if(p_daesin_data->year <= 0 || p_daesin_data->year > 60) {
						if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
							time(&system_time);
							timeinfo = localtime ( &system_time );
						}
						else {
							timeinfo = (struct tm *)&cur_time;
						}
						p_daesin_data->year = (timeinfo->tm_year+1900)%100;
						p_daesin_data->mon = timeinfo->tm_mon+1;
						p_daesin_data->day = timeinfo->tm_mday;
						p_daesin_data->hour = timeinfo->tm_hour;
						p_daesin_data->min = timeinfo->tm_min;
						p_daesin_data->sec = timeinfo->tm_sec;
						p_daesin_data->m_sec = 0x00;
						/*
						printf("chnage gps_date : %02d%02d%02d%02d%02d%02d%02d\n",	p_daesin_data->year, 
																					p_daesin_data->mon, 
																					p_daesin_data->day,
																					p_daesin_data->hour, 
																					p_daesin_data->min, 
																					p_daesin_data->sec,
																					p_daesin_data->m_sec);
						*/
						p_daesin_data->crc16_d[0] = 0x00;
						p_daesin_data->crc16_d[1] = 0x00;
					}

					store_recv_bank(&op_data_buf[i], sizeof(tacom_daesin_data_t));
					i += sizeof(tacom_daesin_data_t)-1;
#endif
				}
			}
		}
	} //end for
							
	return 0;
}

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

static void *daesin_recv_data_thread (void *pparam)
{
	char uart_buf[1024] = {0};
	unsigned char rest_buf[516];
	int try_cnt = 0;
	int readcnt = 0;
	int rest_buf_len = 0;

	saved_data_recovery();

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	request_dtg_header_info();
	request_broadcast_start();
	wait_taco_unill_power_on();
#else
	request_dtg_header_info();
	request_broadcast_start();
#endif

	try_cnt = 0;
	rest_buf_len = 0;
	readcnt = 0;
	while(1){
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		wait_taco_unill_power_on();
#endif
		readcnt = _wait_read(dtg_uart_fd, uart_buf, sizeof(uart_buf), 5);
		if(readcnt <= 0) {
			try_cnt += 1;
			DTG_LOGE("broadcast data don't received!!!! : try_cnt[%d]", try_cnt);
			if(try_cnt > 3) {
				request_dtg_header_info();
				request_broadcast_start();
				readcnt = 0;
				try_cnt = 0;
			}
			continue;
		}
		else {
			try_cnt = 0;
			if( rest_buf_len > 0)
			{
				rest_buf_len = data_extract(uart_buf, readcnt, rest_buf, sizeof(rest_buf), rest_buf_len);
			}
			else
			{
				rest_buf_len = data_extract(uart_buf, readcnt, rest_buf, sizeof(rest_buf), 0);
			}
		}
	}
	free(recv_bank);
}

pthread_t tid_recv_data;

int daesin_init_process()
{
	memset(recv_bank, 0, sizeof(daesin_data_pack_t) * MAX_DAESIN_DATA_PACK);
	if (pthread_create(&tid_recv_data, NULL, daesin_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create daesin_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int daesin_unreaded_records_num ()
{
	if(MAX_DAESIN_DATA_PACK <= recv_avail_cnt)
		return 0;

	if (recv_avail_cnt > 0)
		return (MAX_DAESIN_DATA_PACK - recv_avail_cnt) * MAX_DAESIN_DATA + recv_bank[end].count;
	else
		return (MAX_DAESIN_DATA_PACK - recv_avail_cnt) * MAX_DAESIN_DATA;
}
/*
void print_dbg_msg(char *title, char *msg, int len)
{
	int i;
	printf("%s> ", title);
	for(i = 0; i < len; i++)
		printf("%c", msg[i]);
	printf("\n");
}
*/
static int data_convert(tacom_std_data_t *std_data, tacom_daesin_data_t *daesin_data) {
	int ret = 0;

	if(daesin_data->crc16_d[0] != 0x00 && daesin_data->crc16_d[0] != 0x00)
	{
		ret = crc_check((unsigned char *)daesin_data, sizeof(tacom_daesin_data_t), &daesin_setup);
		if(ret < 0) {
			DTG_LOGE("wrong dtg data");
			return ret;
		}
	}
	sprintf(std_data->day_run_distance, "%04d", htons(daesin_data->day_dist));
//print_dbg_msg("day dist", std_data->day_run_distance, sizeof(std_data->day_run_distance));
	sprintf(std_data->cumulative_run_distance, "%07d", htonl(daesin_data->all_dist));
//print_dbg_msg("cumulative dist", std_data->cumulative_run_distance, sizeof(std_data->cumulative_run_distance));
	sprintf(std_data->date_time, "%02d%02d%02d%02d%02d%02d%02d"
		, daesin_data->year, daesin_data->mon, daesin_data->day
		, daesin_data->hour, daesin_data->min, daesin_data->sec
		, daesin_data->m_sec);
//print_dbg_msg("date", std_data->date_time, sizeof(std_data->date_time));
	sprintf(std_data->speed, "%03d",daesin_data->speed);
//print_dbg_msg("speed", std_data->speed, sizeof(std_data->speed));
	sprintf(std_data->rpm, "%04d", htons(daesin_data->rpm));
//print_dbg_msg("rpm", std_data->rpm, sizeof(std_data->rpm));

	std_data->bs = daesin_data->bs | 0x30;

	//x : longitude
	//y : latitude
	sprintf(std_data->gps_x, "%09d", htonl(daesin_data->longitude));
//print_dbg_msg("gps_x", std_data->gps_x, sizeof(std_data->gps_x));
	sprintf(std_data->gps_y, "%09d", htonl(daesin_data->latitude));
//print_dbg_msg("gps_y", std_data->gps_y, sizeof(std_data->gps_y));
	sprintf(std_data->azimuth, "%03d", htons(daesin_data->azimuth));
//print_dbg_msg("azimuth", std_data->azimuth, sizeof(std_data->azimuth));
	if(daesin_data->accelation_x >= 0){
		sprintf(std_data->accelation_x, "+%03d.%d", 
			htons(daesin_data->accelation_x)/10, htons(daesin_data->accelation_x)%10);
	} else {
		sprintf(std_data->accelation_x, "-%03d.%d", 
			htons(abs(daesin_data->accelation_x))/10, htons(abs(daesin_data->accelation_x))%10);
	}
//print_dbg_msg("accelation_x", std_data->accelation_x, sizeof(std_data->accelation_x));

	if(daesin_data->accelation_y >= 0){
		sprintf(std_data->accelation_y, "+%03d.%d", 
			htons(daesin_data->accelation_y)/10, htons(daesin_data->accelation_y)%10);
	} else {
		sprintf(std_data->accelation_y, "-%03d.%d", 
			htons(abs(daesin_data->accelation_y))/10, htons(abs(daesin_data->accelation_y))%10);
	}
//print_dbg_msg("accelation_y", std_data->accelation_y, sizeof(std_data->accelation_y));

	sprintf(std_data->status, "%02d",daesin_data->status);
//print_dbg_msg("status", std_data->status, sizeof(std_data->status));

	sprintf(std_data->day_oil_usage, "%09d", htonl(daesin_data->day_oil));
//print_dbg_msg("day_oil_usage", std_data->day_oil_usage, sizeof(std_data->day_oil_usage));
	sprintf(std_data->cumulative_oil_usage, "%09d", htonl(daesin_data->all_oil));
//print_dbg_msg("cumulative_oil_usage", std_data->cumulative_oil_usage, sizeof(std_data->cumulative_oil_usage));
	if(daesin_data->temp_a >= 0){
		sprintf(std_data->temperature_A, "+%02d.%d", 
			htons(daesin_data->temp_a)/10, htons(daesin_data->temp_a)%10);
	} else {
		sprintf(std_data->temperature_A, "-%02d.%d", 
			htons(abs(daesin_data->temp_a))/10, htons(abs(daesin_data->temp_a))%10);
	}
//print_dbg_msg("temperature_A", std_data->temperature_A, sizeof(std_data->temperature_A));
	if(daesin_data->temp_b >= 0){
		sprintf(std_data->temperature_B, "+%02d.%d", 
			htons(daesin_data->temp_b)/10, htons(daesin_data->temp_b)%10);
	} else {
		sprintf(std_data->temperature_B, "-%02d.%d", 
			htons(abs(daesin_data->temp_b))/10, htons(abs(daesin_data->temp_b))%10);
	}
//print_dbg_msg("temperature_B", std_data->temperature_B, sizeof(std_data->temperature_B));
	sprintf(std_data->residual_oil, "%07d", htonl(daesin_data->residual_oil));
//print_dbg_msg("residual_oil", std_data->residual_oil, sizeof(std_data->residual_oil));

	return ret;
}

static int last_read_num;	

void save_record_data()
{
	int i;
	tacom_cy_data_t *cy_data;
	int retry_cnt = 5;
	FILE *fptr = NULL;

	//jwrho file save patch ++
	while(retry_cnt-- > 0)
	{
		fptr = fopen("/var/da_stored_records", "w" );
		if(fptr != NULL)
			break;
		sleep(1);
	}

	if(fptr == NULL)
		return;

	//jwrho file save patch --
	curr_idx = curr;
	while ((MAX_DAESIN_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			cy_data = (tacom_cy_data_t *)&recv_bank[curr_idx].buf[i];
			fwrite(cy_data, 1, sizeof(tacom_cy_data_t), fptr);
		}
		recv_bank[curr_idx].status = DATA_PACK_EMPTY;

		curr_idx++;
		if  (curr_idx == MAX_DAESIN_DATA_PACK) {
			curr_idx = 0;
		}
	}

	if (fptr != NULL) {
		fflush(fptr);
		sync();
		fclose(fptr); fptr = NULL;
		sleep(10); //jwrho 2015.01.17
	}
}


static int std_parsing(TACOM *tm, int request_num, int file_save_flag)
{
	int dest_idx = 0;
	int r_num = 0;
	int ret, i;
	int unread_count = 0;
	
	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;

	tacom_daesin_data_t *daesin_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	//jwrho ++
	unread_count = daesin_unreaded_records_num();
	DTG_LOGD("std_parsing> daesin_unreaded_records_num = [%d]\n", unread_count);
	if(unread_count <= 0)
		return -1;

	while (daesin_header_status == DAESIN_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> daesin_header_status is DAESIN_HEADER_EMPTY\n");
		sleep(2);
	}
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	memcpy(std_hdr->vehicle_model, daesin_header.dtg_model, 20);
	memcpy(std_hdr->vehicle_id_num, daesin_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, daesin_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, daesin_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, daesin_header.business_num, 10);
	memcpy(std_hdr->driver_code, daesin_header.driver_code, 18);

printf("driver_code : \n");
for(i = 0; i < 18; i++)
	printf("[0x%02x] ", std_hdr->driver_code[i]);
printf("\n");

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
		daesin_data = (tacom_daesin_data_t *)&read_curr_buf;
		ret = data_convert(std_data, daesin_data);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;
/*
		while ((MAX_DAESIN_DATA_PACK > recv_avail_cnt) && 
			(recv_bank[curr_idx].status == DATA_PACK_FULL) &&
			((request_num + MAX_DAESIN_DATA) <= tm->tm_setup->max_records_per_once)) {
*/
		r_num = 0;
		while(1)
		{
			if(recv_bank[curr_idx].status != DATA_PACK_FULL)
			{
				DTG_LOGE("bank status is not full pack...r_num[%d]\n", r_num);
				if(r_num <= 0) {
					daesin_ack_records(0);
				}
				break;
			}
			if(r_num > tm->tm_setup->max_records_per_once)
			{
				DTG_LOGE("once max data count over[%d].\n", tm->tm_setup->max_records_per_once);
				break;
			}

			for (i = 0; i < recv_bank[curr_idx].count; i++) {
				std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
				daesin_data = (tacom_daesin_data_t *)&recv_bank[curr_idx].buf[i];
				ret = data_convert(std_data, daesin_data);
				if (ret < 0)
					continue;//return ret;
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_DAESIN_DATA_PACK) {
				curr_idx = 0;
			}
		}

		if(unread_count > 500 && r_num < 100) {
			DTG_LOGE("unread_count = [%d]", unread_count);
			DTG_LOGE("MAX_CY_DATA_PACK/recv_avail_cnt = [%d/%d]", MAX_DAESIN_DATA_PACK, recv_avail_cnt);
			DTG_LOGE("curr_idx = [%d]", curr_idx);
			DTG_LOGE("recv_bank[curr_idx].status = [%d]", recv_bank[curr_idx].status);
			DTG_LOGE("tm->tm_setup->max_records_per_once = [%d]\n", tm->tm_setup->max_records_per_once);
			DTG_LOGE("count = [%d]\n", r_num);
		}
	}
	last_read_num = r_num;

	DTG_LOGD("Stream Size HDR[%d] + DATA[%d] : [%d]", 
			sizeof(tacom_std_hdr_t), sizeof(tacom_std_data_t) * request_num, dest_idx);
	DTG_LOGD("Size : [%d]", dest_idx);

	return dest_idx;
}

int daesin_read_current() 
{
	TACOM *tm = tacom_get_cur_context();
	return std_parsing(tm, 1, 0);
}

int daesin_read_records (int r_num) {
	int ret;
	TACOM *tm = tacom_get_cur_context();

	DTG_LOGD("r_num---------------->[%d]:[%0x%x]\n", r_num, r_num);

	if (r_num == 0x10000000)
	{
		ret = std_parsing(tm, r_num, 1);
	}
	else
	{
		ret = std_parsing(tm, r_num, 0);
	}

	return ret;
}

int daesin_ack_records(int readed_bytes)
{
	int r_num = 0;
	int i;
	int unread_bank_cnt = 0;

	TACOM *tm = tacom_get_cur_context();
	
	r_num = last_read_num;
	DTG_LOGT("%s:%d> end[%d] curr_idx[%d] : curr[%d] : recv_avail_cnt[%d]\n", __func__, __LINE__, end, curr_idx, curr, recv_avail_cnt);

	if (curr_idx == curr) {
		DTG_LOGE("Bank full flush. end[%d], curr_idx[%d], curr[%d]", end, curr_idx, curr);
		curr = curr_idx = end;
	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_DAESIN_DATA_PACK - curr) * sizeof(daesin_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(daesin_data_pack_t));
		recv_avail_cnt += (r_num / MAX_DAESIN_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(daesin_data_pack_t));
		recv_avail_cnt += (r_num / MAX_DAESIN_DATA);
		curr = curr_idx;
	}

	//jwrho 2015.01.21++
	unread_bank_cnt = 0;
	for(i = 0; i < MAX_DAESIN_DATA_PACK; i++)
		if(recv_bank[i].status == DATA_PACK_FULL)
			unread_bank_cnt += 1;

	if( (MAX_DAESIN_DATA_PACK - unread_bank_cnt) != recv_avail_cnt)
	{
		DTG_LOGE("patch #3 recv_avail_cnt : [%d] -> [%d]", recv_avail_cnt, (MAX_DAESIN_DATA_PACK - unread_bank_cnt));
		recv_avail_cnt = (MAX_DAESIN_DATA_PACK - unread_bank_cnt);
	}
	//jwrho 2015.01.21--

	return 0;
}

const struct tm_ops daesin_ops = {
	daesin_init_process,
	NULL,
	NULL,
	NULL,
	NULL,
	daesin_read_current,
	NULL,
	daesin_unreaded_records_num,
	daesin_read_records,
	NULL,
	daesin_ack_records,
};

