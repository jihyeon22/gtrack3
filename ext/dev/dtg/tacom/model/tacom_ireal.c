#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include "ireal.h"
#include <wrapper/dtg_log.h>
#include <tacom_internal.h>
#include <tacom_protocol.h>
#include <convtools.h>
#include <common/power.h>
#include "utill.h"

struct tacom_setup ireal_setup = {
	.tacom_dev				= "TACOM IREAL : ",
	.cmd_rl					= NULL,
	.cmd_rs					= NULL,
	.cmd_rq					= NULL,
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
	.max_records_size		= (MAX_IREAL_DATA * MAX_IREAL_DATA_PACK), // this constant value have to set over max_records_per_once's value.
	.max_records_per_once	= 3000,
	.conf_flag				= 0x06,
};

static tacom_ireal_hdr_t ireal_header = {0, };
static pthread_mutex_t ireal_hdr_mutex;

#define IREAL_HEADER_EMPTY 0
#define IREAL_HEADER_FULL 1
static int ireal_header_status = IREAL_HEADER_EMPTY;

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_IREAL_DATA_PACK;
static ireal_data_pack_t *recv_bank;
static pthread_mutex_t ireal_cmd_mutex;

static tacom_ireal_data_t *read_curr_buf;
static char *recvbuf;

extern int dtg_uart_fd;
extern TACOM *tm;
extern dtg_status_t dtg_status;

#include "ireal_cmd.c"

static void store_recv_bank(char *buf, int size)
{
	tacom_ireal_data_t *ireal_data_recv_buf;

	if(recv_bank[end].count >= MAX_IREAL_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(ireal_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(ireal_data_pack_t));
	}

	if ((size == sizeof(tacom_ireal_data_t)) && (recv_avail_cnt > 0)) {
		ireal_data_recv_buf = (tacom_ireal_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)ireal_data_recv_buf, buf, sizeof(tacom_ireal_data_t));
		recv_bank[end].count++;
		if (recv_bank[end].count >= MAX_IREAL_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_IREAL_DATA_PACK)
					end = 0;

				memset(&recv_bank[end], 0x00, sizeof(ireal_data_pack_t));
			}
		} else {
			recv_bank[end].status = DATA_PACK_AVAILABLE;
		}
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


void saved_data_recovery()
{
	int fd;
	int len;
	int ret;
	tacom_ireal_data_t tmp;

	if(check_file_exist("/var/stored_records") == 0) {
		fd = open("/var/stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, &tmp, sizeof(tacom_ireal_data_t));
				if(ret == sizeof(tacom_ireal_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(&tmp, sizeof(tacom_ireal_data_t));
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
	tacom_ireal_data_t *ireal_data;
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
	while ((MAX_IREAL_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			ireal_data = &recv_bank[curr_idx].buf[i];
			fwrite(ireal_data, 1, sizeof(tacom_ireal_data_t), fptr);
		}
		curr_idx++;
		if  (curr_idx == MAX_IREAL_DATA_PACK) {
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

static void *ireal_recv_data_thread (void)
{
	int i, ret, d_time, idx;
	int retry = 0;
	time_t time_ent, time_out;

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif

	ireal_init_cmd();

	memset(&ireal_header, 0, sizeof(tacom_ireal_hdr_t));

	load_dtg_status();

	saved_data_recovery();

	while(1){
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	wait_taco_unill_power_on();
#endif
		time(&time_ent);
		pthread_mutex_lock(&ireal_cmd_mutex);
		ret = wrap_ireal_rs(60);
		if (ret > 0){
			if(ret > sizeof(tacom_ireal_hdr_t) + sizeof(tacom_ireal_data_t)) {
				idx = 0;
				if (ret > ((recv_avail_cnt - 1) * 
					(sizeof(tacom_ireal_data_t) * MAX_IREAL_DATA))) {
					memset(recvbuf, 0, 1024 * 512);
					pthread_mutex_unlock(&ireal_cmd_mutex);
					sleep(2);
					continue;
				} else {
					while((ret - idx) >= sizeof(tacom_ireal_data_t)){
						store_recv_bank(recvbuf+idx, sizeof(tacom_ireal_data_t));
						idx += sizeof(tacom_ireal_data_t);
					}
				}
			}
			ret = wrap_ireal_rq();
			time(&time_out);
			d_time = difftime(time_out, time_ent);
			pthread_mutex_unlock(&ireal_cmd_mutex);
			if ((10 - d_time) > 0)
				sleep(10 - d_time);
		} else {
			pthread_mutex_unlock(&ireal_cmd_mutex);
			retry++;
			if (retry > 5) {
				retry = 0;
				dtg_status.status |= 0x2;
				store_dtg_status();
			}
		}
		memset(recvbuf, 0, 1024 * 512);
	}

	free(recv_bank);
}

pthread_t tid_recv_data;

int ireal_init_process()
{
	pthread_mutex_init(&ireal_hdr_mutex, NULL);
	pthread_mutex_init(&ireal_cmd_mutex, NULL);
	recv_bank = (ireal_data_pack_t *)malloc(sizeof(ireal_data_pack_t) * MAX_IREAL_DATA_PACK);
	memset(recv_bank, 0, sizeof(ireal_data_pack_t) * MAX_IREAL_DATA_PACK);
	if (pthread_create(&tid_recv_data, NULL, ireal_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create ireal_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int ireal_unreaded_records_num ()
{
	int retry_cnt = 0;
	//jwrho ++
#if (0) 
	while ((MAX_IREAL_DATA_PACK <= recv_avail_cnt) && (retry_cnt < 5)) {
		DTG_LOGD("ireal_unreaded_records_num> recv_avail_cnt[%d]\n", recv_avail_cnt);
		sleep(3);
		retry_cnt++;
	}
#else
	if(MAX_IREAL_DATA_PACK <= recv_avail_cnt)
		return 0;
#endif
	//jwrho --


	if (recv_avail_cnt > 0)
		return (MAX_IREAL_DATA_PACK - recv_avail_cnt) * MAX_IREAL_DATA + recv_bank[end].count;
	else
		return (MAX_IREAL_DATA_PACK - recv_avail_cnt) * MAX_IREAL_DATA;
}

static int data_convert(tacom_std_data_t *std_data, tacom_ireal_data_t *ireal_data) {
	memset(std_data, '0', sizeof(tacom_std_data_t));

	sprintf(std_data->day_run_distance, "%04d", ireal_data->DistanceAday);
	sprintf(std_data->cumulative_run_distance, "%07d", ireal_data->DistanceAll);
	sprintf(std_data->date_time, "%02x%02x%02x%02x%02x%02x%02x", 
			ireal_data->Date.Year, ireal_data->Date.Mon, ireal_data->Date.Day, 
			ireal_data->Date.Hour, ireal_data->Date.Min, ireal_data->Date.Sec,
			ireal_data->Date.mSec);
	sprintf(std_data->speed, "%03d", ireal_data->Speed);
	sprintf(std_data->rpm, "%04d", ireal_data->RPM);
	std_data->bs = ireal_data->Status & 0x0001 ? 1 : 0;
	sprintf(std_data->gps_x, "%09d", ireal_data->GPS_X);
	sprintf(std_data->gps_y, "%09d", ireal_data->GPS_Y);
	sprintf(std_data->azimuth, "%03d", ireal_data->Azimuth);

	long acc_x = (long)((float)ireal_data->Accelation_X * (16.0/256.0) * 9.80665) * 10;
	long acc_y = (long)((float)ireal_data->Accelation_Y * (16.0/256.0) * 9.80665) * 10;
	if(acc_x >= 0){
		sprintf(std_data->accelation_x, "+%03d.%d", acc_x/10, acc_x%10);
	} else {
		sprintf(std_data->accelation_x,"%04d.%d", acc_x/10, abs(acc_x)%10);
	}
	if(acc_y >= 0){
		sprintf(std_data->accelation_y, "+%03d.%d", acc_y/10, acc_y%10);
	} else {
		sprintf(std_data->accelation_y, "%04d.%d", acc_y/10, abs(acc_y)%10);
	}
	uint8_t dtg_status = 0;
	switch (ireal_data->Status)
	{
		case 0x0001: dtg_status = 0; break;	// 브레?�크 on
		case 0x0002: dtg_status = 12; break;	// ?�도?�서 ?�상
		case 0x0004: dtg_status = 13; break;	// RPM?�서 ?�상
		case 0x0008: dtg_status = 0; break;	// 가?�도?�서 ?�상 (?�누리엔???�음)
		case 0x0010: dtg_status = 11; break;	// GPS ?�상
		case 0x0020: dtg_status = 99; break;	// ?�원공급?�상
		case 0x0100: dtg_status = 14; break;	// 브레?�크 ?�호감�? ?�서 ?�상
		case 0x0200: dtg_status = 21; break;	// ?�서 ?�력부 ?�치 ?�상
		case 0x0400: dtg_status = 0; break;	// KEY on/off
		default: dtg_status = 0; break;
	}
	sprintf(std_data->status, "%02d", dtg_status);

	/* STD Body Extended */
	memset(std_data->day_oil_usage, '0', 9);	
	memset(std_data->cumulative_oil_usage, '0', 9);
	memcpy(std_data->temperature_A, "+00.0", 5);
	memcpy(std_data->temperature_B, "+00.0", 5);
	memset(std_data->residual_oil, '0', 7);

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
	tacom_ireal_data_t *ireal_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	//jwrho ++
	DTG_LOGD("std_parsing> ireal_unreaded_records_num  = [%d]\n", ireal_unreaded_records_num (tm));
	if(ireal_unreaded_records_num () <= 0)
		return -1;

	while (ireal_header_status == IREAL_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> ireal_header_status is IREAL_HEADER_EMPTY\n");
		sleep(2);
	}
	//jwrho --


	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	pthread_mutex_lock(&ireal_hdr_mutex);
	memcpy(std_hdr->vehicle_model, ireal_header.DTGModel, 20);
	memcpy(std_hdr->vehicle_id_num, ireal_header.VIN, 17);
	memcpy(std_hdr->vehicle_type, ireal_header.VechicleType, 2);
	memcpy(std_hdr->registration_num, ireal_header.VRN, 12);
	memcpy(std_hdr->business_license_num, ireal_header.BRN, 10);
	memcpy(std_hdr->driver_code, ireal_header.DCode, 18);
	pthread_mutex_unlock(&ireal_hdr_mutex);

	dest_idx += sizeof(tacom_std_hdr_t);

	if (request_num == 1){
		std_data = &tm->tm_strm.stream[dest_idx];
		ireal_data = read_curr_buf;
		ret = data_convert(std_data, ireal_data);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;
		while ((MAX_IREAL_DATA_PACK > recv_avail_cnt) && 
			(recv_bank[curr_idx].status == DATA_PACK_FULL) &&
			//((request_num + MAX_IREAL_DATA) <= tm->tm_setup->max_records_per_once)) {
			(r_num <= tm->tm_setup->max_records_per_once)) {

			for (i = 0; i < recv_bank[curr_idx].count; i++) {
				std_data = &tm->tm_strm.stream[dest_idx];
				ireal_data = &recv_bank[curr_idx].buf[i];
				ret = data_convert(std_data, ireal_data);
				if (ret < 0)
					continue;//return ret;
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_IREAL_DATA_PACK) {
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

int ireal_read_current()
{
	int ret = 0;

	pthread_mutex_lock(&ireal_cmd_mutex);
	ret = wrap_ireal_rc();
	pthread_mutex_unlock(&ireal_cmd_mutex);
		
	return ret;
}

int ireal_read_records (int r_num) {
	int ret;

	TACOM *tm_arg = = tacom_get_cur_context();
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

int ireal_ack_records(int readed_bytes)
{
	int r_num = 0;
	TACOM *tm = tacom_get_cur_context();
	
	r_num = last_read_num;

	if (curr_idx == curr) {
		DTG_LOGD("Bank full flush.");
		memset(recv_bank, 0, MAX_IREAL_DATA_PACK * sizeof(ireal_data_pack_t));
		end = 0;
		curr = 0;
		curr_idx = 0;
		recv_avail_cnt = MAX_IREAL_DATA_PACK;
	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_IREAL_DATA_PACK - curr) * sizeof(ireal_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(ireal_data_pack_t));
		recv_avail_cnt += (r_num / MAX_IREAL_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(ireal_data_pack_t));
		recv_avail_cnt += (r_num / MAX_IREAL_DATA);
		curr = curr_idx;
	}
	DTG_LOGD("Bank flush : %d", r_num);
	return 0;
}

const struct tm_ops ireal_ops = {
	ireal_init_process,
	NULL,
	send_cmd,
	NULL,
	NULL,
	ireal_read_current,
	NULL,
	ireal_unreaded_records_num,
	ireal_read_records,
	NULL,
	ireal_ack_records,
};

