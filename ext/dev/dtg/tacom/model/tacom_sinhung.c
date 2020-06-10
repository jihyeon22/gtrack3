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
#include "utill.h"

struct tacom_setup sh_setup  = {
	.tacom_dev				= "TACOM SINHUNG : ",
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
	.max_records_size		= (MAX_SH_DATA * MAX_SH_DATA_PACK), // this constant value have to set over max_records_per_once's value.
	.max_records_per_once	= 3000,
	.conf_flag				= 0x6,
};

static tacom_sh_hdr_t sh_header;
static pthread_mutex_t sh_hdr_mutex;

#define SH_HEADER_EMPTY 0
#define SH_HEADER_FULL 1
static int sh_header_status = SH_HEADER_EMPTY;

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_SH_DATA_PACK;
static sh_data_pack_t *recv_bank;
extern pthread_mutex_t cmd_mutex;

static tacom_sh_data_t *read_curr_buf;


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
				cy_header_status = CY_HEADER_EMPTY;
				return;
			}

			DTG_LOGE("taco thread wait for ign off\n");
			for(i = 0; i < 10; i++)
			{
				read(dtg_uart_fd, buf, 128); //uart flush
				sleep(1);
			}
		}
	}
}
#endif	


static void store_recv_bank(char *buf, int size)
{
	tacom_sh_data_t *sh_data_recv_buf;

	if(recv_bank[end].count >= MAX_SH_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(sh_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(sh_data_pack_t));
	}

	if ((size == sizeof(tacom_sh_data_t)) && (recv_avail_cnt > 0)) {
		sh_data_recv_buf = (tacom_sh_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)sh_data_recv_buf, buf, sizeof(tacom_sh_data_t));
		recv_bank[end].count++;

		if (recv_bank[end].count >= MAX_SH_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_SH_DATA_PACK)
					end = 0;
				
				memset(&recv_bank[end], 0x00, sizeof(sh_data_pack_t));
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
				memset(&recv_bank[curr], 0x00, sizeof(sh_data_pack_t));
				curr += 1;
				recv_avail_cnt += 1;
				if  (curr == MAX_SH_DATA_PACK)
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
	int len;
	int ret;
	tacom_sh_data_t tmp;

	if(check_file_exist("/var/stored_records") == 0) {
		fd = open("/var/stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, &tmp, sizeof(tacom_sh_data_t));
				if(ret == sizeof(tacom_sh_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(&tmp, sizeof(tacom_sh_data_t));
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
	tacom_sh_data_t *sh_data;
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
	while ((MAX_SH_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			sh_data = &recv_bank[curr_idx].buf[i];
			fwrite(sh_data, 1, sizeof(tacom_sh_data_t), fptr);
		}
		recv_bank[curr_idx].status = DATA_PACK_EMPTY;

		curr_idx++;
		if  (curr_idx == MAX_SH_DATA_PACK) {
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


extern int dtg_uart_fd;
extern TACOM *tm;
extern dtg_status_t dtg_status;
static void *sh_recv_data_thread (void)
{
	char *outbuf;
	char *rl_buf;
	int ret, d_time, idx;
	int crc_err_cnt = 0;
	int retry = 0;
	time_t time_ent, time_out;
	char *tmp_cmd;
	int init_action;

	outbuf = malloc(1024*300);
	rl_buf = malloc(64);
	memset(outbuf, 0, 1024 * 300);
	memset(&sh_header, 0, sizeof(tacom_sh_hdr_t));

	load_dtg_status();

	retry = 0;
	pthread_mutex_lock(&cmd_mutex);
	do {
		ret = send_cmd("RL", &sh_setup, NULL);
		if(ret > 0) {
			ret = recv_data(rl_buf, 64, '<', 64);
			if (ret > 0) {
				if(crc_check((unsigned char *)rl_buf, ret, &sh_setup) >= 0) {
					ret = char_mbtol(&rl_buf[1], ret-4);
				} else {
					ret = -1;
				}
			}
		}
		if(ret < 0 && retry < 3){
			retry++;
			sleep(2);
		}
	} while (ret < 0 && retry < 3);
	pthread_mutex_unlock(&cmd_mutex);
	free(rl_buf);

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
		time(&time_ent);
		pthread_mutex_lock(&cmd_mutex);
		ret = send_cmd("RS", &sh_setup, NULL);
		if (ret > 0){
			ret = recv_data(outbuf, 1024 * 300, '<', 1024*16);
			if (ret > 0){
				dtg_status.status &= ~(0x1 << RS_COMMAND_ALERT);
				store_dtg_status();
				if(ret > sizeof(tacom_sh_hdr_t) + sizeof(tacom_sh_data_t)) {
					if(crc_check((unsigned char *)outbuf, ret, &sh_setup) < 0) {
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
							pthread_mutex_lock(&sh_hdr_mutex);
							memcpy(&sh_header, outbuf+idx, sizeof(tacom_sh_hdr_t));
							sh_header_status = SH_HEADER_FULL;
							pthread_mutex_unlock(&sh_hdr_mutex);
							memcpy(dtg_status.vrn, sh_header.regist_num, 12);
							store_dtg_status();
							if (ret > ((recv_avail_cnt - 1) * 
								(sizeof(tacom_sh_data_t) * MAX_SH_DATA))) {
								memset(outbuf, 0, 1024 * 300);
								pthread_mutex_unlock(&cmd_mutex);
								sleep(2);
								continue;
							} else {
								idx += sizeof(tacom_sh_hdr_t);
								while((ret - idx) >= sizeof(tacom_sh_data_t)){
									store_recv_bank(outbuf+idx, sizeof(tacom_sh_data_t));
									idx += sizeof(tacom_sh_data_t);
								}
							}
						}
					}
				}
				ret = send_cmd("RQ", &sh_setup, NULL);
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

int sh_init_process(TACOM *tm)
{
	pthread_mutex_init(&sh_hdr_mutex, NULL);
	recv_bank = (sh_data_pack_t *)malloc(sizeof(sh_data_pack_t) * MAX_SH_DATA_PACK);
	memset(recv_bank, 0, sizeof(sh_data_pack_t) * MAX_SH_DATA_PACK);
	if (pthread_create(&tid_recv_data, NULL, (void *)sh_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create sh_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int sh_unreaded_records_num (TACOM *tm)
{
	int retry_cnt = 0;
	//jwrho ++
#if (0) 
	while ((MAX_SH_DATA_PACK <= recv_avail_cnt) && (retry_cnt < 5)) {
		DTG_LOGD("sh_unreaded_records_num> recv_avail_cnt[%d]\n", recv_avail_cnt);
		sleep(3);
		retry_cnt++;
	}
#else
	if(MAX_SH_DATA_PACK <= recv_avail_cnt)
		return 0;
#endif
	//jwrho --


	if (recv_avail_cnt > 0)
		return (MAX_SH_DATA_PACK - recv_avail_cnt) * MAX_SH_DATA + recv_bank[end].count;
	else
		return (MAX_SH_DATA_PACK - recv_avail_cnt) * MAX_SH_DATA;
}

static int data_convert(tacom_std_data_t *std_data, tacom_sh_data_t *sh_data) {
	memset(std_data, '0', sizeof(tacom_std_data_t));

	memcpy(std_data->day_run_distance, sh_data->day_run_distance, 4);
	memcpy(std_data->cumulative_run_distance, sh_data->cumulative_run_distance, 7);
	memcpy(std_data->date_time, sh_data->date_time, 14);
	memcpy(std_data->speed, sh_data->speed, 3);
	memcpy(std_data->rpm, sh_data->rpm, 4);
	std_data->bs = sh_data->bs;
	memcpy(std_data->gps_x, sh_data->gps_x, 9);
	memcpy(std_data->gps_y, sh_data->gps_y, 9);
	memcpy(std_data->azimuth, sh_data->azimuth, 3);
	memcpy(std_data->accelation_x, sh_data->accelation_x, 6);
	memcpy(std_data->accelation_y, sh_data->accelation_y, 6);
	memcpy(std_data->status, sh_data->status, 2);

	memset(std_data->day_oil_usage, '0', sizeof(std_data->day_oil_usage)); //?�일 ?�류?�용??-> ?�흥?� ?�음 '0'?�로 채�?
	memcpy(&(std_data->cumulative_oil_usage[2]), sh_data->cumulative_oil_usage, 7); //?�적 ?�류?�용??7)
	memcpy(&(std_data->residual_oil[4]), sh_data->residual_oil, 3); //?�류?�량(3)

	if (sh_data->temperature_A[0] == '1' || sh_data->temperature_A[0] == '0') {
		if (sh_data->temperature_A[0] == '1')
			std_data->temperature_A[0] = '-';
		else
			std_data->temperature_A[0] = '+';

		std_data->temperature_A[1] = sh_data->temperature_A[1];
		std_data->temperature_A[2] = sh_data->temperature_A[2];
		std_data->temperature_A[3] = '.';
		std_data->temperature_A[4] = sh_data->temperature_A[3];
	} else {
		memset(std_data->temperature_A, '-', sizeof(std_data->temperature_A));
		memcpy(std_data->temperature_A, sh_data->temperature_A, 4);
	}
	
	if (sh_data->temperature_B[0] == '1' || sh_data->temperature_B[0] == '0') {
		if (sh_data->temperature_B[0] == '1')
			std_data->temperature_B[0] = '-';
		else
			std_data->temperature_B[0] = '+';

		std_data->temperature_B[1] = sh_data->temperature_B[1];
		std_data->temperature_B[2] = sh_data->temperature_B[2];
		std_data->temperature_B[3] = '.';
		std_data->temperature_B[4] = sh_data->temperature_B[3];
	} else {
		memset(std_data->temperature_B, '-', sizeof(std_data->temperature_B));
		memcpy(std_data->temperature_B, sh_data->temperature_B, 4);
	}

	return 0;
}

static int last_read_num;	
static int std_parsing(TACOM *tm, int request_num,int file_save_flag)
{
	int dest_idx = 0;
	int r_num = 0;
	int ret, i;
	
	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;
	tacom_sh_data_t *sh_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	//jwrho ++
	DTG_LOGD("std_parsing> sh_unreaded_records_num  = [%d]\n", sh_unreaded_records_num (tm));
	if(sh_unreaded_records_num (tm) <= 0)
		return -1;

	while (sh_header_status == SH_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> sh_header_status is SH_HEADER_EMPTY\n");
		sleep(2);
	}
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	pthread_mutex_lock(&sh_hdr_mutex);
	memcpy(std_hdr->vehicle_model, sh_header.dtg_model, 20);
	memcpy(std_hdr->vehicle_id_num, sh_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, sh_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, sh_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, sh_header.business_num, 10);
	memcpy(std_hdr->driver_code, sh_header.driver_code, 18);
	pthread_mutex_unlock(&sh_hdr_mutex);

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
		sh_data = read_curr_buf;
		ret = data_convert(std_data, sh_data);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;
		while ((MAX_SH_DATA_PACK > recv_avail_cnt) && 
			(recv_bank[curr_idx].status == DATA_PACK_FULL) &&
			//((request_num + MAX_SH_DATA) <= tm->tm_setup->max_records_per_once)) {
			(r_num <= tm->tm_setup->max_records_per_once)) {

			for (i = 0; i < recv_bank[curr_idx].count; i++) {
				std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
				sh_data = &recv_bank[curr_idx].buf[i];
				ret = data_convert(std_data, sh_data);
				if (ret < 0)
					continue;//return ret;
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_SH_DATA_PACK) {
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

int sh_read_current(TACOM *tm)
{
	int ret = 0;

	pthread_mutex_lock(&cmd_mutex);
	ret = read_current();
	pthread_mutex_unlock(&cmd_mutex);

	return ret;
}

int sh_current_record_parsing(char *buf, int buf_len, char *destbuf)
{
	if (buf[0] != sh_setup.start_mark) return -1;
	if (sh_header_status == SH_HEADER_EMPTY) {
		pthread_mutex_lock(&sh_hdr_mutex);
		memcpy(&sh_header, buf+1, sizeof(tacom_sh_hdr_t));
		sh_header_status = SH_HEADER_FULL;
		pthread_mutex_unlock(&sh_hdr_mutex);
		memcpy(dtg_status.vrn, sh_header.regist_num, 12);
		store_dtg_status();
	}
	read_curr_buf = (tacom_sh_data_t *)(buf + 1 + sizeof(tacom_sh_hdr_t));
	return std_parsing(tm, 1, 0);
}

int sh_read_records (TACOM *tm_arg, int r_num) {
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

int sh_ack_records(TACOM *tm, int readed_bytes)
{
	int r_num = 0;
	r_num = last_read_num;

	if (curr_idx == curr) {
		DTG_LOGD("Bank full flush.");
		memset(recv_bank, 0, MAX_SH_DATA_PACK * sizeof(sh_data_pack_t));
		end = 0;
		curr = 0;
		curr_idx = 0;
		recv_avail_cnt = MAX_SH_DATA_PACK;
	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_SH_DATA_PACK - curr) * sizeof(sh_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(sh_data_pack_t));
		recv_avail_cnt += (r_num / MAX_SH_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(sh_data_pack_t));
		recv_avail_cnt += (r_num / MAX_SH_DATA);
		curr = curr_idx;
	}
	return 0;
}

static int sh_recv_data(char *data, int len, char eoc)
{
	return recv_data(data, len, eoc, 4096);
}

const struct tm_ops sh_ops = {
	sh_init_process,
	sh_recv_data,
	send_cmd,
	NULL,
	NULL,
	sh_read_current,
	sh_current_record_parsing,
	sh_unreaded_records_num,
	sh_read_records,
	NULL,
	sh_ack_records,
};

