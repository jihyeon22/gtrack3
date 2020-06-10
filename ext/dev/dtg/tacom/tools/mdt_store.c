#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <wrapper/dtg_log.h>
#include <tacom_internal.h>

#if defined(DEVICE_MODEL_INNOCAR) || defined(DEVICE_MODEL_INNOSNS) || defined(DEVICE_MODEL_INNOSNS_DCU)
	#include "tacom_innocar_protocol.h"
#else
	#error "dtg type none define"
#endif

#include "convtools.h"
#include "mdt_store.h"
#include "utill.h"

static unsigned int g_dtg_curr = 0;
static unsigned int g_dtg_curr_idx = 0;
static unsigned int g_dtg_end = 0;
static unsigned int g_dtg_recv_avail_cnt = 0;
static mdt_store_pack_t *g_dtg_recv_bank = NULL;
static unsigned int g_dtg_last_read_num = 0;
static int g_dtg_max_pack_size = 0;


//caller : create_store_taco(sizeof(tacom_inno_data_t), MAX_INNO_DATA_PACK
int create_mdt_data_store(int max_pack_size)
{
	g_dtg_max_pack_size = max_pack_size;
	g_dtg_recv_avail_cnt = max_pack_size;
	g_dtg_curr = 0;
	g_dtg_curr_idx = 0;
	g_dtg_end = 0;

	g_dtg_recv_bank = (mdt_store_pack_t *)malloc(sizeof(mdt_store_pack_t) * g_dtg_max_pack_size);
	if(g_dtg_recv_bank == NULL)
		return -1;

	memset(g_dtg_recv_bank, 0, sizeof(mdt_store_pack_t) * g_dtg_max_pack_size);

	return 0;
}

void destory_mdt_store()
{
	if(g_dtg_recv_bank != NULL)
	{
		free(g_dtg_recv_bank);
		g_dtg_recv_bank = NULL;
	}
}

void mdt_store_recv_bank(unsigned char *buf, int size)
{
	tacom_dtg_data_type_t *data_recv_buf;

	if(g_dtg_recv_bank[g_dtg_end].count >= MDT_MAX_ONE_DATA_COUNT) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&g_dtg_recv_bank[g_dtg_end], 0x00, sizeof(mdt_store_pack_t));
	}

	if(g_dtg_recv_bank[g_dtg_end].status > MDT_DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&g_dtg_recv_bank[g_dtg_end], 0x00, sizeof(mdt_store_pack_t));
	}

	if ((size == sizeof(tacom_dtg_data_type_t)) && (g_dtg_recv_avail_cnt > 0)) {
		data_recv_buf = (tacom_dtg_data_type_t *)&g_dtg_recv_bank[g_dtg_end].buf[g_dtg_recv_bank[g_dtg_end].count];
		memcpy((char *)data_recv_buf, buf, size);
		g_dtg_recv_bank[g_dtg_end].count++;
		if (g_dtg_recv_bank[g_dtg_end].count >= MDT_MAX_ONE_DATA_COUNT) {
			g_dtg_recv_bank[g_dtg_end].status = MDT_DATA_PACK_FULL;
			g_dtg_recv_avail_cnt--;
			if (g_dtg_recv_avail_cnt > 0) {
				g_dtg_end++;
				if (g_dtg_end >= g_dtg_max_pack_size) {
					g_dtg_end = 0;
				}
				memset(&g_dtg_recv_bank[g_dtg_end], 0x00, sizeof(mdt_store_pack_t));
			}
			else
			{
				g_dtg_end = g_dtg_curr;
				memset(&g_dtg_recv_bank[g_dtg_curr], 0x00, sizeof(mdt_store_pack_t));
				g_dtg_curr += 1;
				g_dtg_recv_avail_cnt += 1;
				if  (g_dtg_curr == g_dtg_max_pack_size)
					g_dtg_curr = 0;
			}
		} else {
			g_dtg_recv_bank[g_dtg_end].status = MDT_DATA_PACK_EMPTY;
		}
	}
}


void saved_mdt_data_recovery(char *file_name)
{
	int fd;
	int ret;
	//tacom_dtg_data_type_t buf;
	unsigned char buf[256] = {0};

	if(check_file_exist(file_name) == 0) {
		fd = open(file_name, O_RDONLY, 0644 );
		if(fd > 0) {
			while(1) {
				ret = read(fd, &buf, sizeof(tacom_dtg_data_type_t));
				if(ret == sizeof(tacom_dtg_data_type_t)) {
					if(g_dtg_recv_avail_cnt >  0) {
						mdt_store_recv_bank(buf, sizeof(tacom_dtg_data_type_t));
					}
				} else {
					break;
				}
			}
			close(fd);
		}
		unlink(file_name);
	}
}

void save_mdt_record_data_taco(char *file_name)
{
	int i;
	tacom_dtg_data_type_t *p_data;
	int retry_cnt = 5;
	FILE *fptr = NULL;

	//jwrho file save patch ++
	while(retry_cnt-- > 0)
	{
		fptr = fopen(file_name, "w" );
		if(fptr != NULL)
			break;
		sleep(1);
	}

	if(fptr == NULL)
		return;

	//jwrho file save patch --
	g_dtg_curr_idx = g_dtg_curr;
	while ((g_dtg_max_pack_size > g_dtg_recv_avail_cnt) && (g_dtg_recv_bank[g_dtg_curr_idx].status == MDT_DATA_PACK_FULL))
	{
		for (i = 0; i < g_dtg_recv_bank[g_dtg_curr_idx].count; i++) {
			p_data = (tacom_dtg_data_type_t *)&g_dtg_recv_bank[g_dtg_curr_idx].buf[i];
			fwrite(p_data, 1, sizeof(tacom_dtg_data_type_t), fptr);
		}
		g_dtg_recv_bank[g_dtg_curr_idx].status = MDT_DATA_PACK_EMPTY;

		g_dtg_curr_idx++;
		if  (g_dtg_curr_idx == g_dtg_max_pack_size) {
			g_dtg_curr_idx = 0;
		}
	}

	if (fptr != NULL) {
		fflush(fptr);
		sync();
		fclose(fptr); fptr = NULL;
		sleep(10); //jwrho 2015.01.17
	}
}

int get_mdt_current_count()
{
	if(g_dtg_max_pack_size <= g_dtg_recv_avail_cnt)
		return 0;

	if (g_dtg_recv_avail_cnt > 0)
		return (g_dtg_max_pack_size - g_dtg_recv_avail_cnt) * MDT_MAX_ONE_DATA_COUNT + g_dtg_recv_bank[g_dtg_end ].count;
	

	return (g_dtg_max_pack_size - g_dtg_recv_avail_cnt) * MDT_MAX_ONE_DATA_COUNT;
}


int mdt_data_clear()
{
	int num = 0;
	int i;
	int unread_bank_cnt = 0;

	num = g_dtg_last_read_num;
	DTG_LOGT("%s:%d> end[%d] curr_idx[%d] : curr[%d] : recv_avail_cnt[%d]\n", __func__, __LINE__, g_dtg_end , g_dtg_curr_idx, g_dtg_curr, g_dtg_recv_avail_cnt);

	if (g_dtg_curr_idx == g_dtg_curr) {
		DTG_LOGE("Bank full flush. end[%d], curr_idx[%d], curr[%d]", g_dtg_end, g_dtg_curr_idx, g_dtg_curr);
		g_dtg_end = g_dtg_curr_idx = g_dtg_curr;
	} else if (g_dtg_curr_idx < g_dtg_curr) {
		memset(&g_dtg_recv_bank[g_dtg_curr], 0, (g_dtg_max_pack_size  - g_dtg_curr) * sizeof(mdt_store_pack_t));
		memset(g_dtg_recv_bank, 0, g_dtg_curr * sizeof(mdt_store_pack_t));
		g_dtg_recv_avail_cnt += (num / MDT_MAX_ONE_DATA_COUNT);
		g_dtg_curr = g_dtg_curr_idx;
	} else {
		memset(&g_dtg_recv_bank[g_dtg_curr], 0, (g_dtg_curr_idx - g_dtg_curr) * sizeof(mdt_store_pack_t));
		g_dtg_recv_avail_cnt += (num / MDT_MAX_ONE_DATA_COUNT);
		g_dtg_curr = g_dtg_curr_idx;
	}

	//jwrho 2015.01.21++
	unread_bank_cnt = 0;
	for(i = 0; i < g_dtg_max_pack_size ; i++)
		if(g_dtg_recv_bank[i].status == MDT_DATA_PACK_FULL)
			unread_bank_cnt += 1;

	if( (g_dtg_max_pack_size - unread_bank_cnt) != g_dtg_recv_avail_cnt)
	{
		DTG_LOGE("patch #3 recv_avail_cnt : [%d] -> [%d]", g_dtg_recv_avail_cnt, (g_dtg_max_pack_size - unread_bank_cnt));
		g_dtg_recv_avail_cnt = (g_dtg_max_pack_size - unread_bank_cnt);
	}
	//jwrho 2015.01.21--

	return 0;
}

unsigned int get_data_utc_time(unsigned char *date)
{
	struct tm data_time;
	data_time.tm_year  = char_mbtol(date,   2) + 100;
	data_time.tm_mon   = char_mbtol(date+2, 2) - 1;
	data_time.tm_mday  = char_mbtol(date+4, 2);
	data_time.tm_hour  = char_mbtol(date+6, 2);
	data_time.tm_min   = char_mbtol(date+8, 2);
	data_time.tm_sec   = char_mbtol(date+10,2);
	//printf("TACO DTG DATE : [%04d/%02d/%02d %02d:%02d:%02d]\n", data_time.tm_year+1900, data_time.tm_mon+1 , data_time.tm_mday, data_time.tm_hour, data_time.tm_min, data_time.tm_sec);

	return mktime(&data_time);
}
int mdt_dtg_data(TACOM *tm, int dest_idx, int create_period)
{
	int i;
	int num = 0;
	int ret = 0;
	tacom_std_data_t		*std_data;
	tacom_dtg_data_type_t	*dtg_data;
	unsigned int			old_time = 0;
	unsigned int			cur_time = 0;

	//TO DO : ���� �ֱ⿡�� ���� �ð��� �߻��ϸ� �׸� �д´�.


	num = 0;
	g_dtg_curr_idx = g_dtg_curr;
	while(1) 
	{
		if(g_dtg_recv_bank[g_dtg_curr_idx].status != MDT_DATA_PACK_FULL)
		{
			DTG_LOGE("bank status is not full pack...r_num[%d]\n", num);
			if(num <= 0) {
				mdt_data_clear();
			}
			break;
		}
		if(num > tm->tm_setup->max_records_per_once)
		{
			DTG_LOGE("once max data count over[%d].\n", tm->tm_setup->max_records_per_once);
			break;
		}

		for (i = 0; i < g_dtg_recv_bank[g_dtg_curr_idx].count; i++) {
			std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
			dtg_data = (tacom_dtg_data_type_t *)&g_dtg_recv_bank[g_dtg_curr_idx].buf[i];
			ret = data_convert(std_data, (unsigned char *)dtg_data, num);
			if (ret < 0)
				continue;//return ret;
		
			if(i == 0) 
			{
				old_time = cur_time = get_data_utc_time(std_data->date_time);
			}
			else
			{
				cur_time = get_data_utc_time(std_data->date_time);
				if(abs((cur_time - old_time) - create_period) > 10) //if gab time between A data and A1 data is over 10 sec than create period time, A1 data will send next time.
					break;

				old_time = cur_time;
			}

			dest_idx += sizeof(tacom_std_data_t);
			num++;
		}
		g_dtg_curr_idx++;
		if  (g_dtg_curr_idx == g_dtg_max_pack_size ) {
			g_dtg_curr_idx = 0;
		}
	} //end while
	g_dtg_last_read_num = num;

	return dest_idx;
}

//	static int data_convert(tacom_std_data_t *std_data, unsigned char *dtg_data, int debug_flag) {
//		tacom_inno_data_t *inno_data = (tacom_inno_data_t *)dtg_data