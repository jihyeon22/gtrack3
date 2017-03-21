/**
* @file tacom_loop.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-12-11
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <termios.h>
#include <fcntl.h>
#include <wrapper/dtg_log.h>
#include <iconv.h>
#include <errno.h>

#include <common/crc16.h>
#include <tacom_internal.h>
#include <tacom_protocol.h>
#include <convtools.h>
#include <common/power.h>
#include <common/gpio.h>

#include "utill.h"

struct tacom_setup loop_setup  = {
	.tacom_dev				= "TACOM LOOP : ",
	.cmd_rl					= "RL",
	.cmd_rs					= "RS",
	.cmd_rq					= "RQ",
	.cmd_rr					= NULL,
	.cmd_rc					= "RC",
	.cmd_ack				= NULL,
	.data_rl				= 1,
	.data_rs				= 2,
	.data_rq				= 3,
	.start_mark				= '>',
	.end_mark				= '<',
	.head_length			= 79,
	.head_delim				= ',',
	.head_delim_index		= 80,
	.head_delim_length		= 1,
	.record_length			= 0,
	.max_records_size		= (MAX_LOOP_DATA * MAX_LOOP_DATA_PACK), // this constant value have to set over max_records_per_once's value.
	.max_records_per_once	= 3000,
	.conf_flag				= 0x6,
};

static tacom_loop_hdr_t loop_header;
static pthread_mutex_t loop_hdr_mutex;

#define LOOP_HEADER_EMPTY 0
#define LOOP_HEADER_FULL 1
static int loop_header_status = LOOP_HEADER_EMPTY;

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_LOOP_DATA_PACK;
static loop_data_pack_t *recv_bank;
extern pthread_mutex_t cmd_mutex;

static tacom_loop_data_t *read_curr_buf;

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

static void store_recv_bank(char *buf, int size)
{
	tacom_loop_data_t *loop_data_recv_buf;

	if(recv_bank[end].count >= MAX_LOOP_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(loop_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(loop_data_pack_t));
	}

	if ((size == sizeof(tacom_loop_data_t)) && (recv_avail_cnt > 0)) {
		loop_data_recv_buf = (tacom_loop_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)loop_data_recv_buf, buf, sizeof(tacom_loop_data_t));
		recv_bank[end].count++;
		if (recv_bank[end].count >= MAX_LOOP_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_LOOP_DATA_PACK)
					end = 0;
				
				memset(&recv_bank[end], 0x00, sizeof(loop_data_pack_t));
			}
		} else {
			//recv_bank[end].status = DATA_PACK_AVAILABLE;
			recv_bank[end].status = DATA_PACK_EMPTY;
		}
	}
}

void save_record_data()
{
	int i;
	tacom_loop_data_t *loop_data;
	int retry_cnt = 5;
	FILE *fptr = NULL;

	//jwrho file save patch ++
	while(retry_cnt-- > 0)
	{
		fptr = fopen("/var/lp_stored_records", "w" );
		if(fptr != NULL)
			break;
		sleep(1);
	}

	if(fptr == NULL)
		return;

	//jwrho file save patch --
	curr_idx = curr;
	while ((MAX_LOOP_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			loop_data = &recv_bank[curr_idx].buf[i];
			fwrite(loop_data, 1, sizeof(tacom_loop_data_t), fptr);
		}
		recv_bank[curr_idx].status = DATA_PACK_EMPTY;

		curr_idx++;
		if  (curr_idx == MAX_LOOP_DATA_PACK) {
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

int loop_data_filter_func(unsigned char *buf, int len)
{
	unsigned char *pbuf;
	int i, j;

	if(len < 10) {
		printf("skip loop filter as under 10 length\n");
		return len;
	}

	if(buf[0] != '>' || buf[len-1] != '<') {
		printf("fiilter error #1\n");
		return len;
	}

	pbuf = malloc(len);

	if(pbuf != NULL) {

		for(i = 0, j = 0; i < len-3; i++) {
			if(buf[i] != 0)
				pbuf[j++] = buf[i];
		}
		memcpy(&pbuf[j], &buf[i], 3);
		len = j+3;
		memcpy(buf, pbuf, len);              

		free(pbuf);
	}

	return len;
}

extern int dtg_uart_fd;
extern TACOM *tm;
extern dtg_status_t dtg_status;

int check_link_file(char *file)
{
	int count = 10;

	while(count--) {
		if(access(file, F_OK) == 0)
			return 0;

		sleep(1);
	}
	fprintf(stderr, "\n=============================== \n");
	fprintf(stderr, "%s dont exist \n", file);
	fprintf(stderr, "=============================== \n\n");
	return -1;
}

void run_initaitive_cmd()
{
	char tmp_cmd[64];
	int init_action;
	int i;
	int retry = 0;
	unsigned char outbuf[1024];
	int ret;
fprintf(stderr, "=================>\n");
fprintf(stderr, "run_initaitive_cmd call\n");
fprintf(stderr, "=================>\n");

	init_action = 0;
	memset(tmp_cmd, 0x00, sizeof(tmp_cmd));
	if(check_link_file("/var/dtg_initiative_action") == 0) { 
		//file exist
		init_action = open("/var/dtg_initiative_action", O_RDONLY , 0644);
		if(init_action > 0) {
			read(init_action, tmp_cmd, 60);
			close(init_action);
		}
	}
	if(tmp_cmd[0] != 0x00) {
		do {
fprintf(stderr, "=================>1\n");
			ret = send_cmd(tmp_cmd, &loop_setup, __func__);
			if (ret > 0){
fprintf(stderr, "=================>2\n");
			ret = recv_data(outbuf, 1024, 0, 512);
				if(ret > 0){
fprintf(stderr, "=================>3 : ret[%d]\n", ret);
					FILE *init_log = NULL;
					init_log = fopen("/var/initiative_log", "w");//O_RDWR | O_CREAT , 0644);
					if(init_log != NULL) {
fprintf(stderr, "=================>4\n");
						for(i = 0; i < ret; i++) {
							//fprintf(stderr, "0x%02x\n", outbuf[i]);
							#if (1)
							if (!(isprint(outbuf[i]))) {				/* If not printable */
								fprintf(init_log, ".");      			/* print a dot */
							} else {
								fprintf(init_log, "%c",outbuf[i]);		/* else display it */
							}    
							#endif
						}
fprintf(stderr, "=================>5\n");
						fprintf(init_log, "\n");
						fclose(init_log);
fprintf(stderr, "=================>6\n");
						remove("/var/dtg_initiative_action");
fprintf(stderr, "=================>7\n");
						break;
					}
				}

			} else {
				retry++;
				sleep(2);
			}
		} while(ret < 0 && retry < 3);
		
	}

	if(ret < 0)
		dtg_status.status |= (0x1 << INITIATIVE_FAIL);
	else
		dtg_status.status &= ~(0x1 << INITIATIVE_FAIL);
	store_dtg_status();
}


void saved_data_recovery()
{
	int fd;
	int len;
	int ret;
	tacom_loop_data_t tmp;

	if(check_file_exist("/var/lp_stored_records") == 0) {
		fd = open("/var/lp_stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, &tmp, sizeof(tacom_loop_data_t));
				if(ret == sizeof(tacom_loop_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(&tmp, sizeof(tacom_loop_data_t));
					}
				} else {
					break;
				}
			}
			close(fd);
		}
		unlink("/var/lp_stored_records");
	}
}
static void *loop_recv_data_thread (void)
{
	unsigned char *outbuf;
	unsigned char rl_buf[64];
	int ret, d_time, idx;
	int crc_err_cnt = 0;
	int retry = 0;
	time_t time_ent, time_out;
	

	outbuf = malloc(1024*300);
	memset(outbuf, 0, 1024 * 300);
	memset(&loop_header, 0, sizeof(tacom_loop_hdr_t));
	

	load_dtg_status();

	retry = 0;
	fprintf(stderr, "thread cmd_mutex===========================> lock\n");
	pthread_mutex_lock(&cmd_mutex);

	run_initaitive_cmd();

	do {
		ret = send_cmd("RL", &loop_setup, "loop_recv_data_thread #1");
		if(ret > 0) {
			ret = recv_data(rl_buf, 64, '<', 64);
			if (ret > 0) {
				if(crc_check((unsigned char *)rl_buf, ret, &loop_setup) >= 0) {
					ret = char_mbtol(&rl_buf[1], ret-4);
				} else {
					ret = -1;
				}
			}
		}
		if(ret < 0 && retry < 10){
			retry++;
			sleep(2);
		}
	} while (ret < 0 && retry < 10);
	pthread_mutex_unlock(&cmd_mutex);

	if((ret > 5000 * 10000) || ret < 0)
		dtg_status.status |= (0x1 << RL_COMMAND_ALERT);
	else
		dtg_status.status &= ~(0x1 << RL_COMMAND_ALERT);
	dtg_status.records = ret;
	store_dtg_status();

	saved_data_recovery();
	DTG_LOGD("Init Available Bank Count [%d]\n", recv_avail_cnt);
	retry = 0;
	while(1){
		#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
			wait_taco_unill_power_on();
		#endif

		time(&time_ent);
		pthread_mutex_lock(&cmd_mutex);
		ret = send_cmd("RS", &loop_setup, "loop_recv_data_thread #2");
		if (ret > 0){
			ret = recv_data(outbuf, 1024 * 300, '<', 1024*16);
			ret = loop_data_filter_func(outbuf, ret);
			if (ret > 0){
				dtg_status.status &= ~(0x1 << RS_COMMAND_ALERT);
				store_dtg_status();
				if(ret > sizeof(tacom_loop_hdr_t) + sizeof(tacom_loop_data_t)) {
					if(crc_check((unsigned char *)outbuf, ret, &loop_setup) < 0) {
						if (crc_err_cnt < 3) {
							memset(outbuf, 0, 1024 * 300);
							crc_err_cnt++;
							pthread_mutex_unlock(&cmd_mutex);
							sleep(2);
							continue;
						}
						crc_err_cnt = 0;
						dtg_status.status |= (0x1 << CRC_ERROR_ALERT);
						store_dtg_status();
					} else {
						dtg_status.status &= ~(0x1 << CRC_ERROR_ALERT);
						store_dtg_status();
						crc_err_cnt = 0;
						idx = 0;
						if(outbuf[idx++] == '>'){
							pthread_mutex_lock(&loop_hdr_mutex);
							memcpy(&loop_header, outbuf+idx, sizeof(tacom_loop_hdr_t));
							loop_header_status = LOOP_HEADER_FULL;
							pthread_mutex_unlock(&loop_hdr_mutex);
							memcpy(dtg_status.vrn, loop_header.regist_num, 12);
							store_dtg_status();
							DTG_LOGD("Available Bank Count [%d]\n", recv_avail_cnt);
							if (ret > ((recv_avail_cnt - 1) * 
								(sizeof(tacom_loop_data_t) * MAX_LOOP_DATA))) {
								memset(outbuf, 0, 1024 * 300);
								pthread_mutex_unlock(&cmd_mutex);
								DTG_LOGE("Buffer is Full.......");
								sleep(2);
								continue;
							} else {
								idx += sizeof(tacom_loop_hdr_t);
								while((ret - idx) >= sizeof(tacom_loop_data_t)){
									store_recv_bank(outbuf+idx, sizeof(tacom_loop_data_t));
									idx += sizeof(tacom_loop_data_t);
								}
							}
						}
					}
				}
				ret = send_cmd("RQ", &loop_setup, "loop_recv_data_thread #3");
				ret = recv_data(outbuf, 64, '<', 64);
				time(&time_out);
				d_time = difftime(time_out, time_ent);
				pthread_mutex_unlock(&cmd_mutex);
				if ((30 - d_time) > 0)
					sleep(30 - d_time);
			} else {
				pthread_mutex_unlock(&cmd_mutex);
				retry++;
				if (retry > 5) {
					retry = 0;
					dtg_status.status |= (0x1 << RS_COMMAND_ALERT);
					store_dtg_status();
				}
			}
			memset(outbuf, 0, 1024 * 300);
		} else {
			pthread_mutex_unlock(&cmd_mutex);
		}
	}
	free(recv_bank);
}

pthread_t tid_recv_data;

int loop_init_process(TACOM *tm)
{
	pthread_mutex_init(&loop_hdr_mutex, NULL);
	recv_bank = (loop_data_pack_t *)malloc(sizeof(loop_data_pack_t) * MAX_LOOP_DATA_PACK);
	memset(recv_bank, 0, sizeof(loop_data_pack_t) * MAX_LOOP_DATA_PACK);
	if (pthread_create(&tid_recv_data, NULL, (void *)loop_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create loop_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int loop_unreaded_records_num (TACOM *tm)
{
	int retry_cnt = 0;

	if(MAX_LOOP_DATA_PACK <= recv_avail_cnt)
		return 0;

	if (recv_avail_cnt > 0)
		return (MAX_LOOP_DATA_PACK - recv_avail_cnt) * MAX_LOOP_DATA + recv_bank[end].count;
	else
		return (MAX_LOOP_DATA_PACK - recv_avail_cnt) * MAX_LOOP_DATA;
}

static int data_convert(tacom_std_data_t *std_data, tacom_loop_data_t *loop_data) {
	memset(std_data, '0', sizeof(tacom_std_data_t));

	memcpy(std_data->day_run_distance, loop_data->day_run_dist, 4);
	memcpy(std_data->cumulative_run_distance, loop_data->acumul_run_dist, 7);
	memcpy(std_data->date_time, loop_data->date_time, 14);
#if (0)
	fprintf(stderr, "%c%c%c%c-%c%c-%c%c %c%c:%c%c:%c%c\n",  
												'2', '0', std_data->date_time[0], std_data->date_time[1], //year
												std_data->date_time[2], std_data->date_time[3], //month
												std_data->date_time[4], std_data->date_time[5],	//day	
												std_data->date_time[6], std_data->date_time[7],	//hour	
												std_data->date_time[8], std_data->date_time[9],//min		
												std_data->date_time[10], std_data->date_time[11] //sec   
												);
#endif
	memcpy(std_data->speed, loop_data->speed, 3);
	memcpy(std_data->rpm, loop_data->rpm, 4);
	std_data->bs = loop_data->bs;
	memcpy(std_data->gps_x, loop_data->gps_x, 9);
	memcpy(std_data->gps_y, loop_data->gps_y, 9);
	memcpy(std_data->azimuth, loop_data->azimuth, 3);
	memcpy(std_data->accelation_x, loop_data->accelation_x, 6);
	memcpy(std_data->accelation_y, loop_data->accelation_y, 6);
	memcpy(std_data->status, loop_data->status, 2);

	memset(std_data->day_oil_usage, '0', sizeof(std_data->day_oil_usage));
	memcpy(&(std_data->day_oil_usage[4]), loop_data->day_oil_usage, 5);

	memset(std_data->cumulative_oil_usage, '0', sizeof(std_data->cumulative_oil_usage));
	memcpy(&(std_data->cumulative_oil_usage[1]), loop_data->cumul_oil_usage, 8);

	if (loop_data->temper_a[0] == '1' || loop_data->temper_a[0] == '0') {
		if (loop_data->temper_a[0] == '1')
			std_data->temperature_A[0] = '-';
		else
			std_data->temperature_A[0] = '+';

		std_data->temperature_A[1] = loop_data->temper_a[1];
		std_data->temperature_A[2] = loop_data->temper_a[2];
		std_data->temperature_A[3] = '.';
		std_data->temperature_A[4] = loop_data->temper_a[3];
	} else {
		memset(std_data->temperature_A, '-', sizeof(std_data->temperature_A));
		memcpy(std_data->temperature_A, loop_data->temper_a, 4);
	}
	
	if (loop_data->temper_b[0] == '1' || loop_data->temper_b[0] == '0') {
		if (loop_data->temper_b[0] == '1')
			std_data->temperature_B[0] = '-';
		else
			std_data->temperature_B[0] = '+';

		std_data->temperature_B[1] = loop_data->temper_b[1];
		std_data->temperature_B[2] = loop_data->temper_b[2];
		std_data->temperature_B[3] = '.';
		std_data->temperature_B[4] = loop_data->temper_b[3];
	} else {
		memset(std_data->temperature_B, '-', sizeof(std_data->temperature_B));
		memcpy(std_data->temperature_B, loop_data->temper_b, 4);
	}

	memset(std_data->residual_oil, '0', sizeof(std_data->residual_oil));

	return 0;
}

static int last_read_num;	
static int std_parsing(TACOM *tm, int request_num, int file_save_flag)
{
	int dest_idx = 0;
	int r_num = 0;
	int ret, i;
	int unread_count = 0;
	
	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;

	tacom_loop_data_t *loop_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	
	//jwrho ++
	unread_count = loop_unreaded_records_num(tm);
	DTG_LOGD("std_parsing> loop_unreaded_records_num = [%d]\n", unread_count);
	if(unread_count <= 0)
		return 0;

	while (loop_header_status == LOOP_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> loop_header_status is LOOP_HEADER_EMPTY\n");
		sleep(2);
	}
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	pthread_mutex_lock(&loop_hdr_mutex);
	memset(std_hdr->vehicle_model, '#', 20);
	memcpy(std_hdr->vehicle_model + 11, loop_header.dtg_model, 9);
	memcpy(std_hdr->vehicle_id_num, loop_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, loop_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, loop_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, loop_header.business_num, 10);
	memcpy(std_hdr->driver_code, loop_header.driver_code, 18);
	pthread_mutex_unlock(&loop_hdr_mutex);

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
		loop_data = read_curr_buf;
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
					loop_ack_records(tm, 0);
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
			if  (curr_idx == MAX_LOOP_DATA_PACK) {
				curr_idx = 0;
			}
		}

		if(unread_count > 500 && r_num < 100) {
			DTG_LOGE("unread_count = [%d]", unread_count);
			DTG_LOGE("MAX_LOOP_DATA_PACK/recv_avail_cnt = [%d/%d]", MAX_LOOP_DATA_PACK, recv_avail_cnt);
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

int loop_read_current(TACOM *tm)
{
	int ret = 0;

	pthread_mutex_lock(&cmd_mutex);
	ret = read_current();
	pthread_mutex_unlock(&cmd_mutex);

	return ret;
}

int loop_current_record_parsing(char *buf, int buf_len, char *destbuf)
{
	if (buf[0] != loop_setup.start_mark) return -1;
	if (loop_header_status == LOOP_HEADER_EMPTY) {
		pthread_mutex_lock(&loop_hdr_mutex);
		memcpy(&loop_header, buf+1, sizeof(tacom_loop_hdr_t));
		loop_header_status = LOOP_HEADER_FULL;
		pthread_mutex_unlock(&loop_hdr_mutex);
		memcpy(dtg_status.vrn, loop_header.regist_num, 12);
		store_dtg_status();
	}
	read_curr_buf = (tacom_loop_data_t *)(buf + 1 + sizeof(tacom_loop_hdr_t));
	return std_parsing(tm, 1, 0);
}

int loop_read_records (TACOM *tm_arg, int r_num) {

	int ret;

	DTG_LOGD("r_num---------------->[%d]:[%0x%x]\n", r_num, r_num);
	if (r_num == 0x10000000)
	{
		ret = std_parsing(tm, r_num, 1);
	}
	else
	{
		ret = std_parsing(tm, r_num, 0);
	}
	//return std_parsing(tm_arg, r_num);

	return ret;
}

int loop_ack_records(TACOM *tm, int readed_bytes)
{
	int r_num = 0;
	int i;
	int unread_bank_cnt = 0;
	r_num = last_read_num;

	DTG_LOGT("%s:%d> end[%d] curr_idx[%d] : curr[%d] : recv_avail_cnt[%d]\n", __func__, __LINE__, end, curr_idx, curr, recv_avail_cnt);
	if (curr_idx == curr) {
		DTG_LOGE("Bank full flush. end[%d], curr_idx[%d], curr[%d]", end, curr_idx, curr);
		curr = curr_idx = end;

	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_LOOP_DATA_PACK - curr) * sizeof(loop_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(loop_data_pack_t));
		recv_avail_cnt += (r_num / MAX_LOOP_DATA);
		curr = curr_idx;

	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(loop_data_pack_t));
		recv_avail_cnt += (r_num / MAX_LOOP_DATA);
		curr = curr_idx;
	}

	//jwrho 2015.01.21++
	unread_bank_cnt = 0;
	for(i = 0; i < MAX_LOOP_DATA_PACK; i++)
		if(recv_bank[i].status == DATA_PACK_FULL)
			unread_bank_cnt += 1;

	if( (MAX_LOOP_DATA_PACK - unread_bank_cnt) != recv_avail_cnt)
	{
		DTG_LOGE("patch #3 recv_avail_cnt : [%d] -> [%d]", recv_avail_cnt, (MAX_LOOP_DATA_PACK - unread_bank_cnt));
		recv_avail_cnt = (MAX_LOOP_DATA_PACK - unread_bank_cnt);
	}
	//jwrho 2015.01.21--

	return 0;
}

static int loop_recv_data(char *data, int len, char eoc)
{
	return recv_data(data, len, eoc, 4096);
}

const struct tm_ops loop_ops = {
	loop_init_process,
	loop_recv_data,
	send_cmd,
	NULL,
	NULL,
	loop_read_current,
	loop_current_record_parsing,
	loop_unreaded_records_num,
	loop_read_records,
	NULL,
	loop_ack_records,
};

