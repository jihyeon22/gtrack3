/**
* @file tacom_ucar.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-12-12
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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
#include "utill.h"

struct tacom_setup ucar_setup  = {
	.tacom_dev				= "TACOM UCAR : ",
	.cmd_rl					= "RL",
	.cmd_rs					= "RS",
	.cmd_rq					= "RQ",
	.cmd_rr					= "RR",
	.cmd_rc					= "RC",	
	.cmd_ack				= "ACK",
	.data_rl				= 1,
	.data_rs				= 2,
	.data_rq				= 3,
	.start_mark				= '>',
	.end_mark				= '<',
	.head_length			= 79,
	.head_delim				= ',',
	.head_delim_index		= 80,
	.head_delim_length		= 1,
	.record_length			= 53,
	.max_records_size		= (MAX_UCAR_DATA * MAX_UCAR_DATA_PACK),
	.max_records_per_once	= 3000,
	.conf_flag				= 0x5,
	//.conf_flag				= 0x25,
};

extern pthread_mutex_t cmd_mutex;

int ucar_read_summary(TACOM *tm)
{
    DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
    int bytes;
    char *buf = tm->tm_strm.stream;
    int bufsize = tm->tm_strm.size;
    
	pthread_mutex_lock(&cmd_mutex);
    /* Send RR Command */
    if (tm->tm_ops->tm_send_cmd(tm->tm_setup->cmd_rr, tm->tm_setup, __func__) < 0) {
        DTG_LOGE("%sRR ERROR", tm->tm_setup->tacom_dev);
        return -1;
    }
    
    memset(buf, 0, bufsize);
    
    /* Recv Data */
    bytes = tm->tm_ops->tm_recv_data(buf, bufsize, 0x00);
    if (bytes < 0) {
        return -1;
    }

    DTG_LOGD("READ SUMMARY LEN[%d], End of Data[%c][%c][%c]", bytes, buf[bytes-3], buf[bytes-2], buf[bytes-1]);

	pthread_mutex_unlock(&cmd_mutex);
    return bytes;
}

int ucar_ack_summary(TACOM *tm)
{
    DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
    int result;

	pthread_mutex_lock(&cmd_mutex);

    if (tm->tm_ops->tm_send_cmd(tm->tm_setup->cmd_ack, tm->tm_setup, __func__) < 0) {
        DTG_LOGE("%sACK ERROR", tm->tm_setup->tacom_dev);
        return -1;
    }

    DTG_LOGD("%sACK SUMMARY SUCCESS", tm->tm_setup->tacom_dev);
	pthread_mutex_unlock(&cmd_mutex);
    return 1;
    
}

static tacom_ucar_hdr_t ucar_header = {0, };
static tacom_ucar_hdr_t ucar_header_status = {0, };
static pthread_mutex_t ucar_hdr_mutex;

#define UCAR_HEADER_EMPTY 0
#define UCAR_HEADER_FULL 1
static int ucar_header_status = UCAR_HEADER_EMPTY;
static int _header_tmp = 0;
static int _data_command_flag = 0;

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_UCAR_DATA_PACK;
static ucar_data_pack_t *recv_bank;

static tacom_ucar_data_t read_curr_buf;
static pthread_mutex_t ucar_curr_mutex;

extern int dtg_uart_fd;
extern TACOM *tm;
extern dtg_status_t dtg_status;

static void store_recv_bank(char *buf, int size)
{
	tacom_ucar_data_t *ucar_data_recv_buf;

	if(recv_bank[end].count >= MAX_UCAR_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(ucar_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(ucar_data_pack_t));
	}

	if ((size == sizeof(tacom_ucar_data_t)) && (recv_avail_cnt > 0)) {
		ucar_data_recv_buf = (tacom_ucar_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)ucar_data_recv_buf, buf, sizeof(tacom_ucar_data_t));
		memcpy(&read_curr_buf, buf, sizeof(tacom_ucar_data_t));
		recv_bank[end].count++;
		if (recv_bank[end].count >= MAX_UCAR_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_UCAR_DATA_PACK)
					end = 0;
				
				memset(&recv_bank[end], 0x00, sizeof(ucar_data_pack_t));
			}
		} else {
			recv_bank[end].status = DATA_PACK_AVAILABLE;
		}
	}
}

void saved_data_recovery()
{
	unlink("/var/stored_records");//no need recovery. why broadcast way.
}

int dtg_info_file_save(char *file_path, tacom_ucar_hdr_t new_header)
{
	int fd;
	tacom_ucar_hdr_t ucar_header_tmp;

	if(check_file_exist("/var/ucar_dtg_hd") >= 0) {
		fd = open("/var/ucar_dtg_hd", O_RDONLY, 0644);
		if(fd > 0) {
			if(read(fd, &ucar_header_tmp, sizeof(tacom_ucar_hdr_t)) >= sizeof(tacom_ucar_hdr_t)) {
				if(!memcmp(&ucar_header_tmp, &new_header, sizeof(tacom_ucar_hdr_t))) {
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
	fd = open("/var/ucar_dtg_hd", O_WRONLY | O_CREAT, 0644);
	if(fd > 0) {
		write(fd, &new_header, sizeof(tacom_ucar_hdr_t));
		close(fd);
	} else {
		DTG_LOGE("/var/ucar_dtg_hd file save error");
	}
	return 0;
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


static void *ucar_send_cmd_thread (void)
{
	char vrn_num[14];

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	while (1) 
	{
		wait_taco_unill_power_on();

		if(ucar_header_status == UCAR_HEADER_FULL) {
			memset(vrn_num, 0x00, sizeof(vrn_num));
			strncpy(vrn_num, ucar_header.regist_num, 12);
			DTG_LOGD("ucar_header is get : %s!!", vrn_num);
			strncpy(vrn_num, ucar_header_status.regist_num, 12);
			DTG_LOGD("ucar_header_temp is get : %s!!", vrn_num);
			DTG_LOGD("_data_command_flag = [%d]\n", _data_command_flag);
			sleep(10);
			
		}

		if(send_cmd("RI", &ucar_setup, NULL) < 0) 
		{
			DTG_LOGE("%s: %s: uart_write fail", __FILE__, __func__);
		}

		sleep(10);

		if(_data_command_flag == 0) 
		{
			if(send_cmd("RS", &ucar_setup, NULL) < 0)
			{
				DTG_LOGD("%s:%d> RS Send CMD Fail\n", __func__, __LINE__);
			}
		}
	}
#else
	while (ucar_header_status == CY_HEADER_EMPTY) 
	{
		if(send_cmd("RI", &ucar_setup, NULL) < 0) 
		{
			DTG_LOGE("%s: %s: uart_write fail", __FILE__, __func__);
		}
		sleep(10);
		
		if(_data_command_flag == 0) 
		{
			if(send_cmd("RS", &ucar_setup, NULL) < 0)
			{
				DTG_LOGD("%s:%d> RS Send CMD Fail\n", __func__, __LINE__);
			}
		}
	}
#endif
}

static void *ucar_recv_data_thread (void)
{
	int i, ret;
	int retry = 0;
	char *tmp_cmd;
	char buf[512] = {0};
	int idx = 0;
	int readcnt = 0;
	int nuart_cnt = 0;
	int ri_fd = 0;
	int recovery_mode = 0;
	int fuart_check = 0;
	int data_debug_cnt = 0;

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif	


	memset(&ucar_header, 0, sizeof(tacom_ucar_hdr_t));
	load_dtg_status();

	saved_data_recovery();
	
	//ucar_header default value ++
	ucar_header.start_mark = '>';
	memcpy(ucar_header.data_id, "$$RD", 4);;
	memset(ucar_header.dtg_model, '#', 20);;
	memset(ucar_header.vehicle_id_num, '0', 17);
	memset(ucar_header.vehicle_type, '0', 2);
	memset(ucar_header.regist_num, '0', 12);
	memset(ucar_header.business_num, '0', 10);
	memset(ucar_header.driver_code, '0', 18);
	memset(ucar_header.crc, '0', 4);
	ucar_header.end_mark[0] = '<';
	ucar_header.end_mark[1] = 0x0d;
	ucar_header.end_mark[2] = 0x0a;
	//ucar_header default value --

	
	idx = 0;
	readcnt = 0;
	while(1) {
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif
		readcnt = read(dtg_uart_fd, &buf[idx], 1);
		if (readcnt > 0) {
			if (buf[idx] == 0x0a && buf[idx-1] == 0x0d && buf[idx-2] == '<') {
				if (!strncmp(buf, ">$$RD", 5)) {
					//header;
					memcpy(&ucar_header, buf, sizeof(tacom_ucar_hdr_t));
					ucar_header_status = UCAR_HEADER_FULL;
				}
				else if (!strncmp(buf, ">$$RB", 5)) {
					_data_command_flag = 1;
					store_recv_bank(buf, sizeof(tacom_ucar_data_t));

#if (1) //for debugging
					if( (data_debug_cnt % 30) == 0) {
						DTG_LOGD("%s", buf);
						data_debug_cnt = 0;
					}
					data_debug_cnt += 1;
#endif

#if (0) //for debugging
						//data loss check log
						fprintf(stderr, "%c%c%c%c-%c%c-%c%c %c%c:%c%c:%c%c\n",  
																			'2', '0', buf[16], buf[17], //year
																			buf[18], buf[19], //month
																			buf[20], buf[21],	//day	
																			buf[22], buf[23],	//hour	
																			buf[24], buf[25],//min		
																			buf[26], buf[27] //sec   
																		);
#endif
					if(idx > 109){
						DTG_LOGE("Erro #1 : %s", buf);
					} 
				} else {
					DTG_LOGE("Erro #2 : %s", buf);
				}
				memset(buf, 0, 512);
				usleep(1000);
				idx = 0;
			} else {
				idx++;
				if (idx == 511) {
					DTG_LOGE("Erro #3 : %s", buf);
					idx = 0;
				}
			}
		} else {
			nuart_cnt++;
			if (nuart_cnt == 5){
				usleep(50000);
				nuart_cnt = 0;

				//jwrho ++
				if(recovery_mode == 0 && fuart_check == 0) {
					ret = send_cmd("RI", &ucar_setup, "ucar_recv_data_thread #3");
					if ((readcnt = uart_read(dtg_uart_fd, (unsigned char *)buf, 512, 7)) == 91){
						if(!strncmp(buf, ">$$RD", 5)) {
							memcpy(&ucar_header, buf, sizeof(tacom_ucar_hdr_t));
							ucar_header_status = UCAR_HEADER_FULL;
							ret = send_cmd("RS", &ucar_setup, "ucar_recv_data_thread #4");
							recovery_mode = 1;
						}
					}
				}
				//jwrho --
			}
		}
	}
	free(recv_bank);
}

pthread_t tid_recv_data;
pthread_t tid_send_cmd;

int ucar_init_process(TACOM *tm)
{
	pthread_mutex_init(&ucar_hdr_mutex, NULL);
	pthread_mutex_init(&ucar_curr_mutex, NULL);
	recv_bank = (ucar_data_pack_t *)malloc(sizeof(ucar_data_pack_t) * MAX_UCAR_DATA_PACK);
	memset(recv_bank, 0, sizeof(ucar_data_pack_t) * MAX_UCAR_DATA_PACK);
	
	if (pthread_create(&tid_send_cmd, NULL, ucar_send_cmd_thread, NULL) < 0) {
		fprintf(stderr, "cannot create ucar_send_cmd_thread thread\n");
		exit(1);
	}

	if (pthread_create(&tid_recv_data, NULL, ucar_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create ucar_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int ucar_unreaded_records_num (TACOM *tm)
{
	int retry_cnt = 0;
	//jwrho ++
#if (0) 
	while ((MAX_UCAR_DATA_PACK <= recv_avail_cnt) && (retry_cnt < 5)) {
		DTG_LOGD("ucar_unreaded_records_num> recv_avail_cnt[%d]\n", recv_avail_cnt);
		sleep(3);
		retry_cnt++;
	}
#else
	if(MAX_UCAR_DATA_PACK <= recv_avail_cnt)
		return 0;
#endif
	//jwrho --

	if (recv_avail_cnt > 0)
		return (MAX_UCAR_DATA_PACK - recv_avail_cnt) * MAX_UCAR_DATA + recv_bank[end].count;
	else
		return (MAX_UCAR_DATA_PACK - recv_avail_cnt) * MAX_UCAR_DATA;
}

static int data_convert(tacom_std_data_t *std_data, tacom_ucar_data_t *ucar_data) {
	memset(std_data, '0', sizeof(tacom_std_data_t));
	memcpy(std_data->day_run_distance, ucar_data->day_run_dist, 4);
	memcpy(std_data->cumulative_run_distance, ucar_data->acumul_run_dist, 7);
	memcpy(std_data->date_time, ucar_data->date_time, 14);
	memcpy(std_data->speed, ucar_data->speed, 3);
	memcpy(std_data->rpm, ucar_data->rpm, 4);
	std_data->bs = ucar_data->bs;
	memcpy(std_data->gps_x, ucar_data->gps_x, 9);
	memcpy(std_data->gps_y, ucar_data->gps_y, 9);
	memcpy(std_data->azimuth, ucar_data->azimuth, 3);
	memcpy(std_data->accelation_x, ucar_data->accelation_x, 6);
	memcpy(std_data->accelation_y, ucar_data->accelation_y, 6);
	memcpy(std_data->status, ucar_data->status, 2);

	memset(std_data->day_oil_usage, '0', sizeof(std_data->day_oil_usage));
	memcpy(&(std_data->day_oil_usage[3]), ucar_data->day_oil_usage, 7);

	memcpy(std_data->cumulative_oil_usage, &ucar_data->cumul_oil_usage[1], sizeof(std_data->cumulative_oil_usage));

	if (ucar_data->temper_a[0] == '1' || ucar_data->temper_a[0] == '0') {
		if (ucar_data->temper_a[0] == '1')
			std_data->temperature_A[0] = '-';
		else
			std_data->temperature_A[0] = '+';

		std_data->temperature_A[1] = ucar_data->temper_a[1];
		std_data->temperature_A[2] = ucar_data->temper_a[2];
		std_data->temperature_A[3] = '.';
		std_data->temperature_A[4] = ucar_data->temper_a[3];
	} else {
		memset(std_data->temperature_A, '-', sizeof(std_data->temperature_A));
		memcpy(std_data->temperature_A, ucar_data->temper_a, 4);
	}
	
	if (ucar_data->temper_b[0] == '1' || ucar_data->temper_b[0] == '0') {
		if (ucar_data->temper_b[0] == '1')
			std_data->temperature_B[0] = '-';
		else
			std_data->temperature_B[0] = '+';

		std_data->temperature_B[1] = ucar_data->temper_b[1];
		std_data->temperature_B[2] = ucar_data->temper_b[2];
		std_data->temperature_B[3] = '.';
		std_data->temperature_B[4] = ucar_data->temper_b[3];
	} else {
		memset(std_data->temperature_B, '-', sizeof(std_data->temperature_B));
		memcpy(std_data->temperature_B, ucar_data->temper_b, 4);
	}

	memset(std_data->residual_oil, '0', sizeof(std_data->residual_oil));
	memcpy(std_data->residual_oil + 4, ucar_data->residual_oil, 3);

	return 0;
}

static int last_read_num;	
static int std_parsing(TACOM *tm, int request_num)
{
	int dest_idx = 0;
	int r_num = 0;
	int ret, i;
	
	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;

	tacom_ucar_data_t *ucar_data;

	//jwrho ++
	DTG_LOGD("std_parsing> ucar_unreaded_records_num = [%d]\n", ucar_unreaded_records_num(tm));
	if(ucar_unreaded_records_num(tm) <= 0)
		return -1;

	while (ucar_header_status == UCAR_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> ucar_header_status is UCAR_HEADER_EMPTY\n");
		sleep(2);
	}
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	pthread_mutex_lock(&ucar_hdr_mutex);
	memcpy(std_hdr->vehicle_model, ucar_header.dtg_model, 20);
	memcpy(std_hdr->vehicle_id_num, ucar_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, ucar_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, ucar_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, ucar_header.business_num, 10);
	memcpy(std_hdr->driver_code, ucar_header.driver_code, 18);
	pthread_mutex_unlock(&ucar_hdr_mutex);

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = &tm->tm_strm.stream[dest_idx];
		pthread_mutex_lock(&ucar_curr_mutex);
		ret = data_convert(std_data, &read_curr_buf);
		pthread_mutex_unlock(&ucar_curr_mutex);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;
		while ((MAX_UCAR_DATA_PACK > recv_avail_cnt) && 
			(recv_bank[curr_idx].status == DATA_PACK_FULL) &&
			((request_num + MAX_UCAR_DATA) <= tm->tm_setup->max_records_per_once)) {

			for (i = 0; i < recv_bank[curr_idx].count; i++) {
				std_data = &tm->tm_strm.stream[dest_idx];
				ucar_data = &recv_bank[curr_idx].buf[i];
				ret = data_convert(std_data, ucar_data);
				if (ret < 0)
					continue;//return ret;
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_UCAR_DATA_PACK) {
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

int ucar_read_current(TACOM *tm)
{
	return std_parsing(tm, 1);
}

int ucar_current_record_parsing(char *buf, int buf_len, char *destbuf)
{
	int ret = 0;
	return ret;
}

int ucar_read_records (TACOM *tm_arg, int r_num) {
	return std_parsing(tm_arg, r_num);
}

int ucar_ack_records(TACOM *tm, int readed_bytes)
{
	int r_num = 0;
	r_num = last_read_num;

	if (curr_idx == curr) {
		DTG_LOGD("Bank full fluucar.");
		memset(recv_bank, 0, MAX_UCAR_DATA_PACK * sizeof(ucar_data_pack_t));
		end = 0;
		curr = 0;
		curr_idx = 0;
		recv_avail_cnt = MAX_UCAR_DATA_PACK;
	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_UCAR_DATA_PACK - curr) * sizeof(ucar_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(ucar_data_pack_t));
		recv_avail_cnt += (r_num / MAX_UCAR_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(ucar_data_pack_t));
		recv_avail_cnt += (r_num / MAX_UCAR_DATA);
		curr = curr_idx;
	}
	return 0;
}

static int ucar_recv_data(char *data, int len, char eoc)
{
	return recv_data(data, len, eoc, 4096);
}

const struct tm_ops ucar_ops = {
	ucar_init_process,
	ucar_recv_data,
	send_cmd,
	ucar_read_summary,
	ucar_ack_summary,
	ucar_read_current,
	ucar_current_record_parsing,
	ucar_unreaded_records_num,
	ucar_read_records,
	NULL,
	ucar_ack_records,
};

