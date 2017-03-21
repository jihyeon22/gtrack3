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
#include <fcntl.h>


#include <tacom_internal.h>
#include <tacom_protocol.h>
#include <convtools.h>
#include <common/power.h>

#include "uart.h"
#include "utill.h"

struct tacom_setup cj_setup  = {
	.tacom_dev				= "TACOM CJ : ",
	.cmd_rl					= "RL",
	.cmd_rs					= "RS",
	.cmd_rq					= "RQ",
	.cmd_rr					= NULL,
	.cmd_rc					= NULL,
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
	.max_records_size		= (MAX_CJ_DATA * MAX_CJ_DATA_PACK), // this constant value have to set over max_records_per_once's value.
	.max_records_per_once	= 7200,
	.conf_flag				= 0x6,
};

static tacom_cj_hdr_t cj_header;
static pthread_mutex_t cj_hdr_mutex;

#define CJ_HEADER_EMPTY 0
#define CJ_HEADER_FULL 1
static int cj_header_status = CJ_HEADER_EMPTY;

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_CJ_DATA_PACK;
static cj_data_pack_t *recv_bank;
extern pthread_mutex_t cmd_mutex;

static tacom_cj_data_t *read_curr_buf;

static void store_recv_bank(char *buf, int size)
{
	tacom_cj_data_t *cj_data_recv_buf;

	if(recv_bank[end].count >= MAX_CJ_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(cj_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(cj_data_pack_t));
	}

	if ((size == (int)(sizeof(tacom_cj_data_t))) && (recv_avail_cnt > 0)) {
		cj_data_recv_buf = (tacom_cj_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)cj_data_recv_buf, buf, sizeof(tacom_cj_data_t));
		recv_bank[end].count++;
		if (recv_bank[end].count >= MAX_CJ_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			end++;
			if (end >= MAX_CJ_DATA_PACK)
				end = 0;

			memset(&recv_bank[end], 0x00, sizeof(cj_data_pack_t));
		} else {
			recv_bank[end].status = DATA_PACK_AVAILABLE;
		}
	}
}

void saved_data_recovery()
{
	int fd;
	int len;
	int ret;
	tacom_cj_data_t tmp;

	if(check_file_exist("/var/stored_records") == 0) {
		fd = open("/var/stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, &tmp, sizeof(tacom_cj_data_t));
				if(ret == sizeof(tacom_cj_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(&tmp, sizeof(tacom_cj_data_t));
					}
				} else {
					break;
				}
			}
			close(fd);
		}
		unlink("/var/stored_records");
	}
}

void save_record_data()
{
	int i;
	tacom_cj_data_t *cj_data;
	int retry_cnt = 5;
	FILE *fptr = NULL;

	//jwrho file save patch ++
	while(retry_cnt-- > 0)
	{
		fptr = fopen("/var/stored_records", "w" );
		if(fptr != NULL)
			break;
		sleep(1);
	}

	if(fptr == NULL)
		return;

	//jwrho file save patch --
	curr_idx = curr;
	while ((MAX_CJ_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			cj_data = &recv_bank[curr_idx].buf[i];
			fwrite(cj_data, 1, sizeof(tacom_cj_data_t), fptr);
		}
		curr_idx++;
		if  (curr_idx == MAX_CJ_DATA_PACK) {
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


#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
void wait_taco_unill_power_on()
{
	int ign_status;
	if(power_get_ignition_status() == POWER_IGNITION_OFF)
	{
		while(1)
		{
			ign_status = power_get_ignition_status();
			if(ign_status == POWER_IGNITION_ON)
				return;

			DTG_LOGE("taco thread wait for ign off\n");
			sleep(30);
		}
	}
}
#endif	

extern int dtg_uart_fd;
extern TACOM *tm;
extern dtg_status_t dtg_status;
static void *cj_recv_data_thread (void)
{
	char *outbuf;
	int ret, d_time, idx;
	int retry = 0;
	int cmd_retry = 0;
	time_t time_ent, time_out;
	char *tmp_cmd;
	int init_action;

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif

	outbuf = malloc(1024 * 613);
	memset(outbuf, 0, 1024 * 613);
	memset(&cj_header, 0, sizeof(tacom_cj_hdr_t));

	load_dtg_status();

/*
	do {
		init_action = open("/var/dtg_initiative_action", O_RDONLY , 0644);
		sleep(1);
	} while (init_action < 0 && errno != ENOENT);

	if (init_action > 0){
		do {
			tmp_cmd = malloc(64);
			memset(tmp_cmd, 0, 64);
			memset(tmp_cmd, '>', 1);
			ret = read(init_action, tmp_cmd, 62);
			memset(&tmp_cmd[ret+1], '<', 1);
			ret = uart_write(dtg_uart_fd, tmp_cmd, 4);
			if (ret > 0){
				ret = recv_data(outbuf, 1024 * 613, '<', 1024 * 16);
			}
			close(init_action);
			if (ret < 0 && retry < 3) {
				init_action = open("/var/dtg_initiative_action", O_RDONLY , 0644);
				retry++;
				sleep(2);
			}
		} while(ret < 0 && retry < 3);
		remove("/var/dtg_initiative_action");
	}

	if(ret < 0)
		dtg_status.status |= (0x1 << INITIATIVE_FAIL);
	else
		dtg_status.status &= ~(0x1 << INITIATIVE_FAIL);
	store_dtg_status();
*/


	retry = 0;
	do {
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif
		DTG_LOGD("Send >RL< command.");
		ret = uart_write(dtg_uart_fd, ">RL<", 4);
		if(ret > 0) {
			ret = recv_data(outbuf, 1024*613, '<', 1024*16);
			if (ret > 0) {
				ret = char_mbtol(&outbuf[1], ret-2);
			}
		}
		if(ret < 0 && retry < 3){
			retry++;
			sleep(2);
		}
	} while (ret < 0 && retry < 3);
	if((ret > 5000 * 10000) || ret < 0)
		dtg_status.status |= (0x1 << RL_COMMAND_ALERT);
	else
		dtg_status.status &= ~(0x1 << RL_COMMAND_ALERT);
	dtg_status.records = ret;
	store_dtg_status();

	saved_data_recovery();
	DTG_LOGD("Init Available Bank Count [%d]\n", recv_avail_cnt);

	while(1){
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif

		time(&time_ent);
		memset(outbuf, 0, 1024 * 613);
		pthread_mutex_lock(&cmd_mutex);
		sleep(45);
		DTG_LOGD("Send >RS< command.");
		ret = uart_write(dtg_uart_fd, ">RS<", 4);
		if (ret > 0){
			ret = recv_data(outbuf, 1024 * 613, '<', 1024 * 16);
			if (ret > 0){
				dtg_status.status &= ~(0x1 << RS_COMMAND_ALERT);
				store_dtg_status();
				if(ret > sizeof(tacom_cj_hdr_t) + sizeof(tacom_cj_data_t)) {
					idx = 0;
					if(outbuf[idx++] == '>'){
						pthread_mutex_lock(&cj_hdr_mutex);
						memcpy(&cj_header, outbuf+idx, sizeof(tacom_cj_hdr_t));
						cj_header_status = CJ_HEADER_FULL;
						pthread_mutex_unlock(&cj_hdr_mutex);
						memcpy(dtg_status.vrn, cj_header.regist_num, 12);
						store_dtg_status();
						if (ret > ((recv_avail_cnt - 1) * 
							(sizeof(tacom_cj_data_t) * MAX_CJ_DATA))) {
							LOGW("recv_bank is full.");
							pthread_mutex_unlock(&cmd_mutex);
							continue;
						} else {
							DTG_LOGD("recv_bank is available.");
							idx += sizeof(tacom_cj_hdr_t);
							while((ret - idx) >= (signed int)(sizeof(tacom_cj_data_t))){
								store_recv_bank(outbuf+idx, sizeof(tacom_cj_data_t));
								idx += (sizeof(tacom_cj_data_t) + 1);
							}
						}
					} else {
						LOGW("Failed start_mark.");
						pthread_mutex_unlock(&cmd_mutex);
						uart_close(dtg_uart_fd);
						sleep(180);
						do {
							dtg_uart_fd = uart_open(DTG_TTY_DEV_NAME, B115200);
							if(dtg_uart_fd < 0)
								perror("uart open fail\n");
						} while (dtg_uart_fd < 0);
						continue;
					}
				} else {
					LOGW("Record is not enough.");
					pthread_mutex_unlock(&cmd_mutex);
					sleep(60);
					continue;
				}
				sleep(45);
				retry = 0;
				do {
					memset(outbuf, 0, 1024 * 613);
					DTG_LOGD("Send >RQ< command.");
					ret = uart_write(dtg_uart_fd, ">RQ<", 4);
					if(ret > 0) {
						ret = recv_data(outbuf, 1024 * 613, '<', 1024 * 16);
						if (ret > 0) {
							ret = char_mbtol(&outbuf[1], ret-2);
						}
					}
					if(ret < 0 && retry < 3){
						retry++;
						sleep(15);
					}
				} while (ret < 0 && retry < 3);
				if((ret > 5000 * 10000) || ret < 0)
					dtg_status.status |= (0x1 << RL_COMMAND_ALERT);
				else
					dtg_status.status &= ~(0x1 << RL_COMMAND_ALERT);
				dtg_status.records = ret;
				store_dtg_status();

				time(&time_out);
				d_time = difftime(time_out, time_ent);
				pthread_mutex_unlock(&cmd_mutex);
				if ((120 - d_time) > 0)
					sleep(120 - d_time);
				cmd_retry = 0;
			} else {
				pthread_mutex_unlock(&cmd_mutex);
				cmd_retry++;
				if (cmd_retry > 5) {
					cmd_retry = 0;
					dtg_status.status |= (0x1 << RS_COMMAND_ALERT);
					store_dtg_status();
				}
			}
		} else {
			pthread_mutex_unlock(&cmd_mutex);
		}
	}

	free(recv_bank);
}

pthread_t tid_recv_data;

int cj_init_process(TACOM *tm)
{
	pthread_mutex_init(&cj_hdr_mutex, NULL);
	recv_bank = (cj_data_pack_t *)malloc(sizeof(cj_data_pack_t) * MAX_CJ_DATA_PACK);
	memset(recv_bank, 0, sizeof(cj_data_pack_t) * MAX_CJ_DATA_PACK);
	sleep(5);
	if (pthread_create(&tid_recv_data, NULL, (void *)cj_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create cj_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int cj_unreaded_records_num (TACOM *tm)
{
	int retry_cnt = 0;
	//jwrho ++
#if (0) 
	while ((MAX_CJ_DATA_PACK <= recv_avail_cnt) && (retry_cnt < 5)) {
		DTG_LOGD("cj_unreaded_records_num> recv_avail_cnt[%d]\n", recv_avail_cnt);
		sleep(3);
		retry_cnt++;
	}
#else
	if(MAX_CJ_DATA_PACK <= recv_avail_cnt)
		return 0;
#endif
	//jwrho --


	if (recv_avail_cnt > 0)
		return (MAX_CJ_DATA_PACK - recv_avail_cnt) * MAX_CJ_DATA + recv_bank[end].count;
	else
		return (MAX_CJ_DATA_PACK - recv_avail_cnt) * MAX_CJ_DATA;
}

static int data_convert(tacom_std_data_t *std_data, tacom_cj_data_t *cj_data) {
	memset(std_data, '0', sizeof(tacom_std_data_t));

	memcpy(std_data->day_run_distance, cj_data->day_run_distance, 4);
	memcpy(std_data->cumulative_run_distance, cj_data->cumulative_run_distance, 7);
	memcpy(std_data->date_time, cj_data->date_time, 14);
	memcpy(std_data->speed, cj_data->speed, 3);
	memcpy(std_data->rpm, cj_data->rpm, 4);
	std_data->bs = cj_data->bs;
	memcpy(std_data->gps_x, cj_data->gps_y, 9);
	memcpy(std_data->gps_y, cj_data->gps_x, 9);
	memcpy(std_data->azimuth, cj_data->azimuth, 3);
	memcpy(std_data->accelation_x, cj_data->accelation_x, 6);
	memcpy(std_data->accelation_y, cj_data->accelation_y, 6);
	memcpy(std_data->status, cj_data->status, 2);

	memset(std_data->day_oil_usage, '0', sizeof(std_data->day_oil_usage)); //?�일 ?�류?�용??-> ?�흥?� ?�음 '0'?�로 채�?
	memcpy(&(std_data->cumulative_oil_usage[2]), cj_data->cumulative_oil_usage, 7); //?�적 ?�류?�용??7)
	memcpy(&(std_data->residual_oil[4]), cj_data->residual_oil, 3); //?�류?�량(3)

	if (cj_data->temperature_A[0] == '1' || cj_data->temperature_A[0] == '0') {
		if (cj_data->temperature_A[0] == '1')
			std_data->temperature_A[0] = '-';
		else
			std_data->temperature_A[0] = '+';

		std_data->temperature_A[1] = cj_data->temperature_A[1];
		std_data->temperature_A[2] = cj_data->temperature_A[2];
		std_data->temperature_A[3] = '.';
		std_data->temperature_A[4] = cj_data->temperature_A[3];
	} else {
		memset(std_data->temperature_A, '-', sizeof(std_data->temperature_A));
		memcpy(std_data->temperature_A, cj_data->temperature_A, 4);
	}
	
	if (cj_data->temperature_B[0] == '1' || cj_data->temperature_B[0] == '0') {
		if (cj_data->temperature_B[0] == '1')
			std_data->temperature_B[0] = '-';
		else
			std_data->temperature_B[0] = '+';

		std_data->temperature_B[1] = cj_data->temperature_B[1];
		std_data->temperature_B[2] = cj_data->temperature_B[2];
		std_data->temperature_B[3] = '.';
		std_data->temperature_B[4] = cj_data->temperature_B[3];
	} else {
		memset(std_data->temperature_B, '-', sizeof(std_data->temperature_B));
		memcpy(std_data->temperature_B, cj_data->temperature_B, 4);
	}

	return 0;
}

static int last_read_num;	
static int std_parsing(TACOM *tm, int request_num, int file_save_flag)
{
	int dest_idx = 0;
	int r_num = 0;
	int ret, i;
	
	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;
	tacom_cj_data_t *cj_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}


	//jwrho ++
	DTG_LOGD("std_parsing> cj_unreaded_records_num = [%d]\n", cj_unreaded_records_num(tm));
	if(cj_unreaded_records_num(tm) <= 0)
		return -1;

	while (cj_header_status == CJ_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> cj_header_status is CJ_HEADER_EMPTY\n");
		sleep(2);
	}
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	pthread_mutex_lock(&cj_hdr_mutex);
	memcpy(std_hdr->vehicle_model , cj_header.dtg_model, 20);
	memcpy(std_hdr->vehicle_id_num, cj_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, cj_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, cj_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, cj_header.business_num, 10);
	memcpy(std_hdr->driver_code, cj_header.driver_code, 18);
	pthread_mutex_unlock(&cj_hdr_mutex);

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
		cj_data = read_curr_buf;
		ret = data_convert(std_data, cj_data);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;
		while ((MAX_CJ_DATA_PACK > recv_avail_cnt) && 
			(recv_bank[curr_idx].status == DATA_PACK_FULL) &&
			//((request_num + MAX_CJ_DATA) <= tm->tm_setup->max_records_per_once)) {
			(r_num <= tm->tm_setup->max_records_per_once)) {

			for (i = 0; i < recv_bank[curr_idx].count; i++) {
				std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
				cj_data = &recv_bank[curr_idx].buf[i];
				ret = data_convert(std_data, cj_data);
				if (ret < 0)
					continue;//return ret;
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_CJ_DATA_PACK) {
				curr_idx = 0;
			}
		}
		last_read_num = r_num;
	}

	DTG_LOGD("Size : [%d]", dest_idx);

	if (dest_idx == sizeof(tacom_std_hdr_t))
		return -1;
	else
		return dest_idx;
}

int cj_read_current(TACOM *tm)
{
	int ret = 0;

	pthread_mutex_lock(&cmd_mutex);
	ret = read_current();
	pthread_mutex_unlock(&cmd_mutex);

	return ret;
}

int cj_current_record_parsing(char *buf, int buf_len, char *destbuf)
{
	if (buf[0] != cj_setup.start_mark) return -1;
	if (cj_header_status == CJ_HEADER_EMPTY) {
		pthread_mutex_lock(&cj_hdr_mutex);
		memcpy(&cj_header, buf+1, sizeof(tacom_cj_hdr_t));
		cj_header_status = CJ_HEADER_FULL;
		pthread_mutex_unlock(&cj_hdr_mutex);
		memcpy(dtg_status.vrn, cj_header.regist_num, 12);
		store_dtg_status();
	}
	read_curr_buf = (tacom_cj_data_t *)(buf + 1 + sizeof(tacom_cj_hdr_t));
	return std_parsing(tm, 1, 0);
}

int cj_read_records (TACOM *tm_arg, int r_num) {
	int ret;
	/*
	if (r_num == 0x20000000) //for abort test
	{
		char *test = NULL;
		memset(test, 0x00, 1024);
	}
	*/
	if (r_num == 0x10000000)
	{
		ret = std_parsing(tm_arg, r_num, 1);
	}
	else
	{
		ret = std_parsing(tm_arg, r_num, 0);
	}

	return ret;

}

int cj_ack_records(TACOM *tm, int readed_bytes)
{
	int r_num = 0;
	r_num = last_read_num;

	if (curr_idx == curr) {
		DTG_LOGD("Bank full flush.");
		memset(recv_bank, 0, MAX_CJ_DATA_PACK * sizeof(cj_data_pack_t));
		end = 0;
		curr = 0;
		curr_idx = 0;
		recv_avail_cnt = MAX_CJ_DATA_PACK;
	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_CJ_DATA_PACK - curr) * sizeof(cj_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(cj_data_pack_t));
		recv_avail_cnt += (r_num / MAX_CJ_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(cj_data_pack_t));
		recv_avail_cnt += (r_num / MAX_CJ_DATA);
		curr = curr_idx;
	}
	DTG_LOGD("Bank flush : %d : %d", r_num, recv_avail_cnt);
	return 0;
}

static int cj_recv_data(char *data, int len, char eoc)
{
	return recv_data(data, len, eoc, 4096);
}

const struct tm_ops cj_ops = {
	cj_init_process,
	cj_recv_data,
	send_cmd,
	NULL,
	NULL,
	cj_read_current,
	cj_current_record_parsing,
	cj_unreaded_records_num,
	cj_read_records,
	NULL,
	cj_ack_records,
};


