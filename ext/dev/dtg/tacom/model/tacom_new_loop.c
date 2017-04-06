/**
* @file tacom_new_loop.c
* @brief 
* @author jwrho
* @version 
* @date 2015-04-06
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <wrapper/dtg_log.h>
#include <tacom_internal.h>
#include <tacom_protocol.h>

#include <tacom_new_loop.h>
#include <common/crc16.h>
#include <common/power.h>
#include <common/w200_led.h>

#include "uart.h"
#include "convtools.h"
#include "utill.h"

#define UART_REOPEN_ENABLE_FLAG
extern int dtg_uart_fd;

struct tacom_setup loop2_setup  = {
	.tacom_dev				= "TACOM NEW_LOOP : ",
	.cmd_rl					= "800201",
	.cmd_rs					= NULL,
	.cmd_rq					= NULL,
	.cmd_rr					= NULL,
	.cmd_rc					= NULL,	
	.cmd_ack				= NULL,
	.data_rl				= 1,
	.data_rs				= 2,
	.data_rq				= 3,
	.start_mark				= '$',
	.end_mark				= 0x0A,
	.head_length			= 79,
	.head_delim				= ',',
	.head_delim_index		= 80,
	.head_delim_length		= 1,
	.record_length			= 53,
	.max_records_size		= (MAX_LOOP2_DATA * MAX_LOOP2_DATA_PACK), // this constant value have to set over max_records_per_once's value.
	.max_records_per_once	= 470,
	.conf_flag				= 0x5,
};

static tacom_loop2_hdr_t loop_header = {0, };
static tacom_loop2_hdr_t loop_header_tmp = {0, };

#define LOOP_HEADER_EMPTY 0
#define LOOP_HEADER_FULL 1
static int loop_header_status = LOOP_HEADER_EMPTY;
static int _header_tmp = 0;
void saved_data_recovery();

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
				loop_header_status = LOOP_HEADER_EMPTY;
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


static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_LOOP2_DATA_PACK;
static loop_data2_pack_t *recv_bank;

static int g_read_curr_buf_enable = 0;
static tacom_loop2_data_t read_curr_buf;

static void store_recv_bank(char *buf, int size)
{
	tacom_loop2_data_t *loop_data_recv_buf;

	if(recv_bank[end].count >= MAX_LOOP2_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(loop_data2_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(loop_data2_pack_t));
	}

	if ((size == sizeof(tacom_loop2_data_t)) && (recv_avail_cnt > 0)) {
		loop_data_recv_buf = (tacom_loop2_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)loop_data_recv_buf, buf, sizeof(tacom_loop2_data_t));
		g_read_curr_buf_enable = 1;
		memcpy(&read_curr_buf, buf, sizeof(tacom_loop2_data_t));
		recv_bank[end].count++;
		if (recv_bank[end].count >= MAX_LOOP2_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_LOOP2_DATA_PACK)
					end = 0;
				
				memset(&recv_bank[end], 0x00, sizeof(loop_data2_pack_t));
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
	int len;
	int ret;
	char buf[128] = {0};

	if(check_file_exist("/var/nlp_stored_records") == 0) {
		fd = open("/var/nlp_stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, buf, sizeof(tacom_loop2_data_t));
				if(ret == sizeof(tacom_loop2_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(buf, sizeof(tacom_loop2_data_t));
					}
				} else {
					break;
				}
			}
			close(fd);
		}
		unlink("/var/nlp_stored_records");
	}
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


#define DATA_START_CHAR	(0x3e)
#define DATA_END_CHAR	(0x3c)
#define HEADER_LENGTH	(81)
#define DATA_LENGTH		(46)

int data_extract(unsigned char *dest, int dest_len, unsigned char *un_do_buf, int un_do_buf_size, int current_undo_bufer_length)
{
	static int led_init = 0;
	static int led_flag = 0;
	char op_data_buf[1024];
	int i, j;
	int ret;
	tacom_loop2_data_t *loop_data;


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

//printf("======> dest_len [%d]\n", dest_len);
//printf("hdr [%d], data[%d]\n", sizeof(tacom_loop2_hdr_t), sizeof(tacom_loop2_data_t));
	for(i = 0; i < dest_len; i++) 
	{
		if( (i + sizeof(tacom_loop2_hdr_t)+sizeof(tacom_loop2_data_t) ) > dest_len)
		{
			memset(un_do_buf, 0x00, un_do_buf_size);
			memcpy(un_do_buf, &op_data_buf[i], dest_len-i);
			//printf("rest data [%s]\n", un_do_buf);
			return (dest_len-i);
		}

		// '#' is 0x23
		if( !(op_data_buf[i] == DATA_START_CHAR && op_data_buf[i+1] == '#' && op_data_buf[i+2] == '#') ) //header detect
		{
			if(op_data_buf[i] == DATA_START_CHAR)
				printf("##1 [idx:%d] : 0x%02x 0x%02x 0x%02x\n", i, op_data_buf[i], op_data_buf[i+1], op_data_buf[i+2]);

			continue;
		}
		else
		{
			if(op_data_buf[i + (HEADER_LENGTH-1)] != DATA_END_CHAR) {
				printf("##2 : 0x%02x\n", op_data_buf[i + (HEADER_LENGTH-1)]);
				continue;
			}

			if(op_data_buf[i + (HEADER_LENGTH)] != DATA_START_CHAR) {
				printf("##3 : 0x%02x\n", op_data_buf[i + (HEADER_LENGTH)]);
				continue;
			}

			if(op_data_buf[i + (HEADER_LENGTH+DATA_LENGTH)-1] != DATA_END_CHAR) {
				printf("##4 : 0x%02x\n", op_data_buf[i + (HEADER_LENGTH+DATA_LENGTH)]);
				continue;
			}

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

			if(loop_header_status == LOOP_HEADER_EMPTY)
			{
				memcpy(&loop_header, &op_data_buf[i], sizeof(tacom_loop2_hdr_t));
				loop_header_status = LOOP_HEADER_FULL;
			}
			else
			{
				memcpy(&loop_header_tmp, &op_data_buf[i], sizeof(tacom_loop2_hdr_t));
				_header_tmp = 1;
			}

			store_recv_bank(&op_data_buf[i+sizeof(tacom_loop2_hdr_t)], sizeof(tacom_loop2_data_t));
//loop_data = (tacom_loop2_data_t *)&op_data_buf[i+sizeof(tacom_loop2_hdr_t)];
//printf("date = [%02d-%02d-%02d %02d:%02d:%02d\n", (loop_data->year%100), loop_data->mon, loop_data->day,
//			loop_data->hour, loop_data->min, loop_data->sec);

			i += (sizeof(tacom_loop2_hdr_t) + sizeof(tacom_loop2_data_t))-1;

		}
	}
	return 0;
}

static void *loop_recv_data_thread (void)
{
	DTG_LOGT("%s : %s : Start loop receive data thread +++", __FILE__, __func__);
	char uart_buf[1024] = {0};
	int idx = 0;
	int readcnt = 0;
	int nuart_cnt = 0;
	unsigned char rest_buf[516];
	int rest_buf_len = 0;
	int read_err_cnt = 5;

	saved_data_recovery();

	while(1) {
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		wait_taco_unill_power_on();
#endif

#ifdef UART_REOPEN_ENABLE_FLAG
		if(dtg_uart_fd < 0) {
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, 115200);
			if(dtg_uart_fd < 0) {
				DTG_LOGE("UART OPEN ERROR LOOP");
				sleep(1);
				continue;
			}
		}

		readcnt = _wait_read(dtg_uart_fd, uart_buf, sizeof(uart_buf), 3);
		if(readcnt <= 0) //read error 
		{
			DTG_LOGE("DTG Data Read Fail : [%d]", read_err_cnt);
			if(read_err_cnt-- < 0) {
				read_err_cnt = 5;
				if(dtg_uart_fd > 0) {
					close(dtg_uart_fd);
					dtg_uart_fd = -1;
				}
				continue;
			}
			
		}
		else
		{
			read_err_cnt = 5;
		}
#else
		readcnt = _wait_read(dtg_uart_fd, uart_buf, sizeof(uart_buf), 5);
		if(readcnt <= 0) {
			continue;
		}
#endif

		if( rest_buf_len > 0)
		{
			rest_buf_len = data_extract(uart_buf, readcnt, rest_buf, sizeof(rest_buf), rest_buf_len);
		}
		else
		{
			rest_buf_len = data_extract(uart_buf, readcnt, rest_buf, sizeof(rest_buf), 0);
		}

	}
	free(recv_bank);
}

pthread_t tid_recv_data;

int loop2_init_process()
{
	printf("================================================================\n");
	printf("loop2_init_process\n");
	printf("================================================================\n");
	recv_bank = (loop_data2_pack_t *)malloc(sizeof(loop_data2_pack_t) * MAX_LOOP2_DATA_PACK);
	memset(recv_bank, 0, sizeof(loop_data2_pack_t) * MAX_LOOP2_DATA_PACK);

	if (pthread_create(&tid_recv_data, NULL, loop_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create loop_recv_data_thread thread\n");
		exit(1);
	}

	
	return 0;
}

int loop2_unreaded_records_num ()
{
	if(MAX_LOOP2_DATA_PACK <= recv_avail_cnt)
		return 0;

	if (recv_avail_cnt > 0)
		return (MAX_LOOP2_DATA_PACK - recv_avail_cnt) * MAX_LOOP2_DATA + recv_bank[end].count;
	else
		return (MAX_LOOP2_DATA_PACK - recv_avail_cnt) * MAX_LOOP2_DATA;
}

//static int test_data_seq = 0; //for test

//#define DEBUG_LOOP_DATA

//#ifdef DEBUG_LOOP_DATA
void print_data(char *msg, char *value, int len)
{
	char data[128];
	memset(data, 0x00, sizeof(data));
	memcpy(data, value, len);
	DTG_LOGD("[%s] = [%s]\n", msg, data);
}
//#endif

static int data_convert(tacom_std_data_t *std_data, tacom_loop2_data_t *loop_data) {
	int ret = 0;
	char tmp_buf[128]; //for test
	
#ifdef DEBUG_LOOP_DATA
	loop_data->day_run_dist = 1234;
	loop_data->acumul_run_dist = 1234567;
	loop_data->speed = 123;
	loop_data->rpm = 3600;
	loop_data->gps_x = 126753966;
	loop_data->gps_y = 37572826;
	loop_data->azimuth = 285;
	loop_data->accelation_x = 1234;
	loop_data->accelation_y = -1234;
	loop_data->status = 11;
	loop_data->day_oil_usage = 360;
	loop_data->acumul_oil_usage = 13127;
#endif
	snprintf(tmp_buf, 128, "%04d", loop_data->day_run_dist);
	memcpy(std_data->day_run_distance, tmp_buf, 4);

#ifdef DEBUG_LOOP_DATA
	print_data("day_run_distance", std_data->day_run_distance, 4);
#endif
	//
	snprintf(tmp_buf, 128, "%07d", loop_data->acumul_run_dist);
	memcpy(std_data->cumulative_run_distance, tmp_buf, 7);

#ifdef DEBUG_LOOP_DATA
	print_data("cumulative_run_distance", std_data->cumulative_run_distance, 7);
#endif

	snprintf(tmp_buf, 128, "%02d%02d%02d%02d%02d%02d%02d"
		, (loop_data->year%100), loop_data->mon, loop_data->day
		, loop_data->hour, loop_data->min, loop_data->sec
		, loop_data->msec);
	memcpy(std_data->date_time, tmp_buf, 14);

#ifdef DEBUG_LOOP_DATA
	print_data("date_time", std_data->date_time, 14);
#endif

	snprintf(tmp_buf, 128, "%03d",loop_data->speed);
	memcpy(std_data->speed, tmp_buf, 3);
#ifdef DEBUG_LOOP_DATA
	print_data("speed", std_data->speed, 3);
#endif

	snprintf(tmp_buf, 128, "%04d", loop_data->rpm);
	memcpy(std_data->rpm, tmp_buf, 4);
#ifdef DEBUG_LOOP_DATA
	print_data("rpm", std_data->rpm, 4);
#endif
	
	std_data->bs = (char )loop_data->bs | 0x30;;
#ifdef DEBUG_LOOP_DATA
	printf("bs = [%c]\n", std_data->bs);
#endif
	snprintf(tmp_buf, 128, "%09d", loop_data->gps_x);
	memcpy(std_data->gps_x, tmp_buf, 9);
#ifdef DEBUG_LOOP_DATA
	print_data("gps_x", std_data->gps_x, 9);
#endif
	snprintf(tmp_buf, 128, "%09d", loop_data->gps_y);
	memcpy(std_data->gps_y, tmp_buf, 9);
#ifdef DEBUG_LOOP_DATA
	print_data("gps_y", std_data->gps_y, 9);
#endif
	snprintf(tmp_buf, 128, "%03d", loop_data->azimuth);
	memcpy(std_data->azimuth, tmp_buf, 3);
#ifdef DEBUG_LOOP_DATA
	print_data("azimuth", std_data->azimuth, 3);
#endif

//printf("%d, %d, %d %d, %d\n", loop_data->gps_x, loop_data->gps_y, loop_data->azimuth, loop_data->accelation_x, loop_data->accelation_y);

	if(loop_data->accelation_x >= 0){
		snprintf(tmp_buf, 128, "+%03d.%d", 
			(loop_data->accelation_x)/10, (loop_data->accelation_x)%10);
	} else {
		snprintf(tmp_buf, 128, "-%03d.%d", 
			(abs(loop_data->accelation_x))/10, (abs(loop_data->accelation_x))%10);
	}
	memcpy(std_data->accelation_x, tmp_buf, 6);
#ifdef DEBUG_LOOP_DATA
	print_data("accelation_x", std_data->accelation_x, 6);
#endif
	if(loop_data->accelation_y >= 0){
		snprintf(tmp_buf, 128, "+%03d.%d", 
			(loop_data->accelation_y)/10, (loop_data->accelation_y)%10);
	} else {
		snprintf(tmp_buf, 128, "-%03d.%d", 
			(abs(loop_data->accelation_y))/10, (abs(loop_data->accelation_y))%10);
	}
	memcpy(std_data->accelation_y, tmp_buf, 6);
#ifdef DEBUG_LOOP_DATA
	print_data("accelation_y", std_data->accelation_y, 6);
#endif

	snprintf(tmp_buf, 128, "%02d",loop_data->status);
	memcpy(std_data->status, tmp_buf, 2);
#ifdef DEBUG_LOOP_DATA
	print_data("status", std_data->status, 2);
#endif
/*
printf("===============================================\n");
printf("test code...need code remove...\n");
	loop_data->day_oil_usage = 1000;
	loop_data->acumul_oil_usage = 1234;
printf("day_oil_usage[%d], acumul_oil_usage[%d]...\n", loop_data->day_oil_usage, loop_data->acumul_oil_usage);
printf("===============================================\n");
*/
	snprintf(tmp_buf, 128, "%09d", loop_data->day_oil_usage);
	memcpy(std_data->day_oil_usage, tmp_buf, 9);
#ifdef DEBUG_LOOP_DATA
	print_data("day_oil_usage", std_data->day_oil_usage, 9);
#endif
	snprintf(tmp_buf, 128, "%09d", loop_data->acumul_oil_usage);
	memcpy(std_data->cumulative_oil_usage, tmp_buf, 9);
#ifdef DEBUG_LOOP_DATA
	print_data("acumul_oil_usage", std_data->cumulative_oil_usage, 9);
#endif

	/* choyoung's dtg does not have external data. */
	/* so external data is '0'. */
	memcpy(std_data->temperature_A, "+00.0", 5);
	memcpy(std_data->temperature_B, "+00.0", 5);
	memset(std_data->residual_oil, '0', 7);
	return ret;
}

static int last_read_num = 0;	

void save_record_data()
{
	int i;
	tacom_loop2_data_t *loop_data;
	int retry_cnt = 5;
	FILE *fptr = NULL;

	//jwrho file save patch ++
	while(retry_cnt-- > 0)
	{
		fptr = fopen("/var/nlp_stored_records", "w" );
		if(fptr != NULL)
			break;
		sleep(1);
	}

	if(fptr == NULL)
		return;

	//jwrho file save patch --
	curr_idx = curr;
	while ((MAX_LOOP2_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			loop_data = &recv_bank[curr_idx].buf[i];
			fwrite(loop_data, 1, sizeof(tacom_loop2_data_t), fptr);
		}
		recv_bank[curr_idx].status = DATA_PACK_EMPTY;

		curr_idx++;
		if  (curr_idx == MAX_LOOP2_DATA_PACK) {
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
	tacom_loop2_data_t *loop_data;


	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	//jwrho ++
	unread_count = loop2_unreaded_records_num();
	DTG_LOGD("std_parsing> loop2_unreaded_records_num = [%d]\n", unread_count);
	if(unread_count <= 0)
		return -1;

	while (loop_header_status == LOOP_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> loop_header_status is LOOP_HEADER_EMPTY\n");
		sleep(2);
	}

	do {
		//already success
		if(_header_tmp == 1) 
		{
			if(memcmp(loop_header.regist_num, loop_header_tmp.regist_num, 12))
				memcpy(&loop_header, &loop_header_tmp, sizeof(tacom_loop2_hdr_t));
		}

		break;
	} while (ret < 0);
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	memcpy(std_hdr->vehicle_model, loop_header.dtg_model, 20);
	memcpy(std_hdr->vehicle_id_num, loop_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, loop_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, loop_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, loop_header.business_lsn, 10);
	memcpy(std_hdr->driver_code, loop_header.driver_code, 18);

	print_data("registration_num", loop_header.regist_num, 12);

	dest_idx += sizeof(tacom_std_hdr_t);
	if (request_num == 1) {
		std_data = &tm->tm_strm.stream[dest_idx];
		loop_data = &read_curr_buf;
		ret = data_convert(std_data, loop_data);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;


		r_num = 0;
		while(1)
		{
			if(recv_bank[curr_idx].status != DATA_PACK_FULL)
			{
				DTG_LOGE("bank status is not full pack...r_num[%d]\n", r_num);
				if(r_num <= 0) {
					loop2_ack_records(tm, 0);
				}
				break;
			}
			if(r_num > tm->tm_setup->max_records_per_once)
			{
				DTG_LOGE("once max data count over[%d].\n", tm->tm_setup->max_records_per_once);
				break;
			}

			for (i = 0; i < recv_bank[curr_idx].count; i++) {
				std_data = &tm->tm_strm.stream[dest_idx];
				loop_data = &recv_bank[curr_idx].buf[i];

				ret = data_convert(std_data, loop_data);
				if (ret < 0)
					continue;//return ret; //jwrho 2015-01-17
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_LOOP2_DATA_PACK) {
				curr_idx = 0;
			}
		}

		if(unread_count > 500 && r_num < 100) {
			DTG_LOGE("unread_count = [%d]", unread_count);
			DTG_LOGE("MAX_LOOP2_DATA_PACK/recv_avail_cnt = [%d/%d]", MAX_LOOP2_DATA_PACK, recv_avail_cnt);
			DTG_LOGE("curr_idx = [%d]", curr_idx);
			DTG_LOGE("recv_bank[curr_idx].status = [%d]", recv_bank[curr_idx].status);
			DTG_LOGE("tm->tm_setup->max_records_per_once = [%d]\n", tm->tm_setup->max_records_per_once);
			DTG_LOGE("count = [%d]\n", r_num);
		}
	}
	last_read_num = r_num;

	
	DTG_LOGD("Stream Size HDR[%d] + DATA[%d] : [%d], count[%d]", 
			sizeof(tacom_std_hdr_t), sizeof(tacom_std_data_t) * request_num, dest_idx, r_num);
	
	return dest_idx;
}

int loop2_read_current(){
	int ret = 0;

	if(g_read_curr_buf_enable == 0) {
		DTG_LOGE("%s: not yet read current data", __func__);
		return -1;
	}
	
	return std_parsing(tm, 1, 0);
}

int loop2_read_records (int r_num)
{
	
	int ret;
	TACOM *tm =  tacom_get_cur_context();

	DTG_LOGD("r_num---------------->[%d]:[%0x%x]\n", r_num, r_num);
/*	
	if (r_num == 0x20000000) //for abort test
	{
		char *test = NULL;
		memset(test, 0x00, 1024);
	}
*/
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

int loop2_ack_records(int readed_bytes)
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
		memset(&recv_bank[curr], 0, (MAX_LOOP2_DATA_PACK - curr) * sizeof(loop_data2_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(loop_data2_pack_t));
		recv_avail_cnt += (r_num / MAX_LOOP2_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(loop_data2_pack_t));
		recv_avail_cnt += (r_num / MAX_LOOP2_DATA);
		curr = curr_idx;
	}

	//jwrho 2015.01.21++
	unread_bank_cnt = 0;
	for(i = 0; i < MAX_LOOP2_DATA_PACK; i++)
		if(recv_bank[i].status == DATA_PACK_FULL)
			unread_bank_cnt += 1;

	if( (MAX_LOOP2_DATA_PACK - unread_bank_cnt) != recv_avail_cnt)
	{
		DTG_LOGE("patch #3 recv_avail_cnt : [%d] -> [%d]", recv_avail_cnt, (MAX_LOOP2_DATA_PACK - unread_bank_cnt));
		recv_avail_cnt = (MAX_LOOP2_DATA_PACK - unread_bank_cnt);
	}
	//jwrho 2015.01.21--

	return 0;
}



const struct tm_ops loop2_ops = {
	loop2_init_process,
	NULL,
	NULL,
	NULL,
	NULL,
	loop2_read_current,
	NULL,
	loop2_unreaded_records_num,
	loop2_read_records,
	NULL,
	loop2_ack_records,
};
