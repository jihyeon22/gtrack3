/**
* @file tacom_choyoung.c
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

#include <tacom/tacom_internal.h>
#include <tacom/tacom_protocol.h>
#include <tacom/model/tacom_choyoung_protocol.h>

#include <wrapper/dtg_log.h>
#include <board/board_system.h>

#include <mdsapi/mds_api.h>
#include <board/power.h>

#define UART_REOPEN_ENABLE_FLAG

extern int dtg_uart_fd;
static int valid_check(char *buf, int size);
int cy_ack_records( int readed_bytes);

struct tacom_setup cy_setup  = {
	.tacom_dev				= "TACOM CHOYOUNG : ",
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
	.max_records_size		= (MAX_CY_DATA * MAX_CY_DATA_PACK), // this constant value have to set over max_records_per_once's value.

#if defined(SERVER_MODEL_ETRS) || defined(SERVER_MODEL_ETRS_TB)
	.max_records_per_once	= 200,
#else
	.max_records_per_once	= 470,
#endif

	.conf_flag				= 0x5
};

static tacom_cy_hdr_t cy_header;
static tacom_cy_hdr_t cy_header_tmp;

#define CY_HEADER_EMPTY 0
#define CY_HEADER_FULL 1
static int cy_header_status = CY_HEADER_EMPTY;
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
				//cy_header_status = CY_HEADER_EMPTY;
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

static void *cy_send_cmd_thread (void *pargs)
{
	int i, idx = 0;
	char cmdstr[32] = {0};
	char vrn_num[14];
	int result;
	unsigned short crc16_value;
	unsigned char *crc_data;
	unsigned char tmp[2] = {0};
	DTG_LOGT("%s : %s : Start choyoung send commmand +++", __FILE__, __func__);
	
	saved_data_recovery();

	memcpy(cmdstr, "$800201", 7);
	idx += 7;
	if (cy_setup.conf_flag & (0x1 << TACOM_CRC16_BIT)) {
		crc16_value = mds_api_crc16_get(NULL, 0);
		crc16_value = mds_api_crc16_get("01", 2);
		crc_data = (unsigned char *)&crc16_value;
		tmp[0] = crc_data[(cy_setup.conf_flag >> 
								TACOM_CRC_ENDIAN1_BIT) & 0x1];
		tmp[1] = crc_data[(cy_setup.conf_flag >> 
								TACOM_CRC_ENDIAN0_BIT) & 0x1];
		unsigned short *tmp_short = (unsigned short *)tmp;
		sprintf(crc_data, "%04x", *tmp_short);
		memcpy(&cmdstr[idx], crc_data, 4);
		idx += 4;
	}
	cmdstr[idx++] = 0x0d;
	cmdstr[idx++] = 0x0a;
	
	//
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	while (1) {
		wait_taco_unill_power_on();

		if(cy_header_status == CY_HEADER_FULL) {
			memset(vrn_num, 0x00, sizeof(vrn_num));
			strncpy(vrn_num, cy_header.regist_num, 12);
			DTG_LOGI("cy_header is get : %s!!", vrn_num);
			strncpy(vrn_num, cy_header_tmp.regist_num, 12);
			DTG_LOGI("cy_header_temp is get : %s!!", vrn_num);
			DTG_LOGI("cy_unreaded_records_num = [%d]\n", cy_unreaded_records_num(NULL));
			sleep(10);
			
		}

		DTG_LOGD("send header request command");
		for (i = 0; i < idx; i++) {
			result = -1;
			if(dtg_uart_fd > 0)
				result = write(dtg_uart_fd, &cmdstr[i], 1);

			if (result < 0)
				DTG_LOGE("%s: %s: uart_write fail", __FILE__, __func__);
			usleep(10000);
		}
		sleep(10);
	}
#else
	while (cy_header_status == CY_HEADER_EMPTY) {

		for (i = 0; i < idx; i++) {
			result = -1;
			if(dtg_uart_fd > 0)
				result = write(dtg_uart_fd, &cmdstr[i], 1);

			if (result < 0)
				DTG_LOGE("%s: %s: uart_write fail", __FILE__, __func__);
			usleep(10000);
		}
		sleep(10);
	}
#endif
	return NULL;
}

static unsigned int curr = 0;
static unsigned int curr_idx = 0;
static unsigned int end = 0;
static unsigned int recv_avail_cnt = MAX_CY_DATA_PACK;
static cy_data_pack_t *recv_bank;

static tacom_cy_data_t read_curr_buf;

static void store_recv_bank(char *buf, int size)
{
	//int i;
	tacom_cy_data_t *cy_data_recv_buf;

	if(recv_bank[end].count >= MAX_CY_DATA) {
		fprintf(stderr, "%s ---> patch #1\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(cy_data_pack_t));
	}

	if(recv_bank[end].status > DATA_PACK_FULL) {
		fprintf(stderr, "%s ---> patch #2\n", __func__);
		memset(&recv_bank[end], 0x00, sizeof(cy_data_pack_t));
	}

	if ((size == sizeof(tacom_cy_data_t)) && (recv_avail_cnt > 0)) {
		cy_data_recv_buf = (tacom_cy_data_t *)&recv_bank[end].buf[recv_bank[end].count];
		memcpy((char *)cy_data_recv_buf, buf, sizeof(tacom_cy_data_t));
		memcpy(&read_curr_buf, buf, sizeof(tacom_cy_data_t));
		recv_bank[end].count++;

//printf("Insert Data : available count [%d]", recv_avail_cnt);
//for(i = 0; i < 14; i++)
//	printf("%c", cy_data_recv_buf->date_time[i]);
//printf("\n");

		if (recv_bank[end].count >= MAX_CY_DATA) {
			recv_bank[end].status = DATA_PACK_FULL;
			recv_avail_cnt--;
			if (recv_avail_cnt > 0) {
				end++;
				if (end >= MAX_CY_DATA_PACK)
					end = 0;
				
				memset(&recv_bank[end], 0x00, sizeof(cy_data_pack_t));
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
	//int len;
	int ret;
	char buf[128] = {0};

	
	if (mds_api_check_exist_file("/var/ch_stored_records",1) == DEFINES_MDS_API_OK)
	{
		fd = open("/var/ch_stored_records", O_RDONLY, 0644 );
		if(fd > 0) {
			//read(fd, &len, sizeof(int));

			while(1) {
				ret = read(fd, buf, sizeof(tacom_cy_data_t));
				if(ret == sizeof(tacom_cy_data_t)) {
					if(recv_avail_cnt >  0) {
						store_recv_bank(buf, sizeof(tacom_cy_data_t));
					}
				} else {
					break;
				}
			}
			close(fd);
		}
		unlink("/var/ch_stored_records");
	}
}

#define DTG_INFO_FILE_PATH	"/var/cy_dtg_hd"

int dtg_info_file_save(char *file_path, tacom_cy_hdr_t new_header)
{
	int fd;
	DTG_LOGI("dtg_info_file_save================>");
	fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd > 0) {
		write(fd, &new_header, sizeof(tacom_cy_hdr_t));
		close(fd);
	} else {
		DTG_LOGE("%s file save error", file_path);
	}
	return 0;
}

int dtg_info_file_read(char *file_path)
{
	int ret = 0;
	int dtg_fd = -1;
	if(mds_api_check_exist_file(file_path,1) == DEFINES_MDS_API_OK) {
		dtg_fd = open(file_path, O_RDONLY, 0644);
	}
	else {
		DTG_LOGE("%s file don't exist", file_path);
	}
	if(dtg_fd > 0) {
		ret = read(dtg_fd, &cy_header, sizeof(tacom_cy_hdr_t));
		close(dtg_fd);
	}

	if(ret > 0) {
		DTG_LOGI("%s:%d> Read Header Info From File\n", __func__, __LINE__);
		cy_header_status = CY_HEADER_FULL;
	}	

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

int data_extract(unsigned char *dest, int dest_len, unsigned char *un_do_buf, int un_do_buf_size, int current_undo_bufer_length)
{
	char tmp_buf1[20];
	char tmp_buf2[20];
	static int invalid_data_cnt = 0;
	static int led_init = 0;
	static int led_flag = 0;
	tacom_cy_data_t *p_cydata;
	char op_data_buf[1024];
	int i;
	int ret;

	DTG_LOGD("dtg data recv [%s]/[%d]\r\n", dest, dest_len);
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
		//if( (i + sizeof(tacom_cy_data_t) ) > dest_len)
		if( (i + sizeof(tacom_cy_hdr_t) ) > dest_len)
		{
			memset(un_do_buf, 0x00, un_do_buf_size);
			memcpy(un_do_buf, &op_data_buf[i], dest_len-i);
			//printf("rest data [%s]\n", un_do_buf);
			return (dest_len-i);
		}

		if(op_data_buf[i] == '$')
		{
			if( (i + 2) < dest_len)
			{
				if(op_data_buf[i+1] == '2' && op_data_buf[i+2] == '0') //data
				{
					if( (i + sizeof(tacom_cy_data_t)) <= dest_len)
					{
						ret = valid_check(&op_data_buf[i], sizeof(tacom_cy_data_t));
						if(ret >= 0) {
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
							invalid_data_cnt = 0;
							if(cy_header_status == CY_HEADER_FULL)
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
#endif
							p_cydata = (tacom_cy_data_t *)&op_data_buf[i];
							/*******************************************************************
							strcmp(a, b)
							a == b -->  0
							a > b  -->  1
							a < b  --> -1
							*******************************************************************/
							//if(memcmp(p_cydata->date_time, read_curr_buf.date_time, 14) != 0)
							if(strncmp(p_cydata->date_time, read_curr_buf.date_time, 14)  > 0)
								store_recv_bank(&op_data_buf[i], sizeof(tacom_cy_data_t));
							else {
								DTG_LOGE("============================================");
								DTG_LOGE("RECEIVED SAME DATE FROM DTG");
								memset(tmp_buf1, 0x00, sizeof(tmp_buf1));
								memcpy(tmp_buf1, p_cydata->date_time, 14);

								memset(tmp_buf2, 0x00, sizeof(tmp_buf2));
								memcpy(tmp_buf2, read_curr_buf.date_time, 14);
								DTG_LOGE("%s / %s", tmp_buf1, tmp_buf2);
								DTG_LOGE("============================================");
							}
							i += sizeof(tacom_cy_data_t)-1;
						}
						else
						{
							if(invalid_data_cnt > 30)
							{
//								// w200_led_set_color(W200_LED_GPS, W200_LED_R); // TODO: api fix
//								// w200_led_on(W200_LED_GPS); // TODO: api fix
							}
							else
							{
								invalid_data_cnt += 1;
							}
						}
					}
				}
				else if(op_data_buf[i+1] == '0' && op_data_buf[i+2] == '1') //header
				{
					if( (i + sizeof(tacom_cy_hdr_t)) <= dest_len)
					{
						if(cy_header_status == CY_HEADER_EMPTY)
						{
							memcpy(&cy_header, &op_data_buf[i], sizeof(tacom_cy_hdr_t));
							cy_header_status = CY_HEADER_FULL;
							dtg_info_file_save(DTG_INFO_FILE_PATH, cy_header);
						}
						else
						{
							memcpy(&cy_header_tmp, &op_data_buf[i], sizeof(tacom_cy_hdr_t));
							_header_tmp = 1;
						}

						i += sizeof(tacom_cy_hdr_t)-1;

					}
				}
			}
		}
	}

	return 0;
}

static void *cy_recv_data_thread (void *pargs)
{
	DTG_LOGT("%s : %s : Start choyoung receive data thread +++", __FILE__, __func__);
	char uart_buf[1024] = {0};
	//int idx = 0;
	int readcnt = 0;
	//int nuart_cnt = 0;
	unsigned char rest_buf[516];
	int rest_buf_len = 0;
	int read_err_cnt = 5;

	dtg_info_file_read(DTG_INFO_FILE_PATH);

	while(1) {
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		wait_taco_unill_power_on();
#endif


#ifdef UART_REOPEN_ENABLE_FLAG
		if(dtg_uart_fd < 0) {
			dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, 115200);
			if(dtg_uart_fd < 0) {
				DTG_LOGE("UART OPEN ERROR CY");
				sleep(1);
				continue;
			}
		}
#endif

		readcnt = _wait_read(dtg_uart_fd, uart_buf, sizeof(uart_buf), 3);


#ifdef UART_REOPEN_ENABLE_FLAG
		if(readcnt == 0) //read error 
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

pthread_t tid_send_cmd;
pthread_t tid_recv_data;

int cy_init_process()
{
	memset(&cy_header, 0x00, sizeof(tacom_cy_hdr_t));
	memset(&cy_header_tmp, 0x00, sizeof(tacom_cy_hdr_t));

	recv_bank = (cy_data_pack_t *)malloc(sizeof(cy_data_pack_t) * MAX_CY_DATA_PACK);
	memset(recv_bank, 0, sizeof(cy_data_pack_t) * MAX_CY_DATA_PACK);
	if (pthread_create(&tid_send_cmd, NULL, cy_send_cmd_thread, NULL) < 0) {
		fprintf(stderr, "cannot create cy_send_cmd_thread thread\n");
		exit(1);
	}
	if (pthread_create(&tid_recv_data, NULL, cy_recv_data_thread, NULL) < 0){
		fprintf(stderr, "cannot create cy_recv_data_thread thread\n");
		exit(1);
	}
	return 0;
}

int cy_unreaded_records_num (TACOM *tm)
{
	if(MAX_CY_DATA_PACK <= recv_avail_cnt)
		return 0;

	if (recv_avail_cnt > 0)
		return (MAX_CY_DATA_PACK - recv_avail_cnt) * MAX_CY_DATA + recv_bank[end].count;
	else
		return (MAX_CY_DATA_PACK - recv_avail_cnt) * MAX_CY_DATA;
}

static int valid_check(char *buf, int size){
	char tmp_len[2] = {0};
	char tmp_crc[4] = {0};
	long data_len = 0;
	unsigned short  data_crc;
	unsigned short crc16_value;
	unsigned char *crc_data;
	//char *tmp_data;
	char *ptr;

/*
	DTG_LOGE("Valid Check TRUE for Test...\n");
	return 0;
*/

	memcpy(tmp_crc, &buf[size-6], 4);
	data_len = strtol(tmp_crc, &ptr, 16);
	if(data_len == 0 || ptr == NULL){
		return -1;
	}
	data_crc = (unsigned short)data_len;

	data_len = 0;
	memcpy(tmp_len, &buf[3], 2);
	data_len = strtol(tmp_len, &ptr, 10);
	if(data_len == 0 || ptr == NULL || (data_len != size -11)){
		return -1;
	}

	ptr = (char *)&data_crc;
	mds_api_crc16_get(NULL, 0);
	crc16_value = mds_api_crc16_get(&buf[5], size - 11);
	crc_data = (unsigned char *) &crc16_value;

	if (crc_data[(cy_setup.conf_flag >> TACOM_CRC_ENDIAN1_BIT) & 0x1] != ptr[0] || 
		crc_data[(cy_setup.conf_flag >> TACOM_CRC_ENDIAN0_BIT) & 0x1] != ptr[1])
	{
		DTG_LOGE("%sCRC ERROR", cy_setup.tacom_dev);
		DTG_LOGE("%sRECEIVED CRC = [0x%2x][0x%2x]", cy_setup.tacom_dev, 
			ptr[0], ptr[1]);
		DTG_LOGE("%sCALCULATED CRC = [0x%2x][0x%2x]", cy_setup.tacom_dev, 
			crc_data[(cy_setup.conf_flag >> TACOM_CRC_ENDIAN1_BIT) & 0x1], 
            crc_data[(cy_setup.conf_flag >> TACOM_CRC_ENDIAN0_BIT) & 0x1]);
		return -1;
	}
	return 0;
}

//#define SIMULATTION_DATA_ENABLE
#ifdef SIMULATTION_DATA_ENABLE
	static int g_test_data_seq = 0; //for test
#endif
static int data_convert(tacom_std_data_t *std_data, tacom_cy_data_t *cy_data) {
	int ret = 0;
#ifdef SIMULATTION_DATA_ENABLE
	char tmp_buf[20]; //for test
#endif

	ret = valid_check((char *)cy_data, sizeof(tacom_cy_data_t));
	if(ret < 0) {
		DTG_LOGE("%s: wrong dtg data", __func__);
		return ret;
	}

#ifdef SIMULATTION_DATA_ENABLE
//gps test
	memcpy(cy_data->gps_x, "127101627 ", 9);
	memcpy(cy_data->gps_y, "037399783", 9);
	memcpy(cy_data->azimuth, "255", 3);
	memcpy(cy_data->speed, "120", 3);
	sprintf(tmp_buf, "%07d", g_test_data_seq++);
	memcpy(cy_data->acumul_run_dist, tmp_buf, 7);
#endif


	memcpy(std_data->day_run_distance, cy_data->day_run_dist, 4);
	memcpy(std_data->cumulative_run_distance, cy_data->acumul_run_dist, 7);
	memcpy(std_data->date_time, cy_data->date_time, 14);

/*
	int i;
	for(i = 0; i < 14; i++)
		printf("%c", std_data->date_time[i]);
	printf("\n");
*/

	memcpy(std_data->speed, cy_data->speed, 3);
	memcpy(std_data->rpm, cy_data->rpm, 4);
	std_data->bs = cy_data->bs;
	memcpy(std_data->gps_x, cy_data->gps_x, 9);
	memcpy(std_data->gps_y, cy_data->gps_y, 9);
	memcpy(std_data->azimuth, cy_data->azimuth, 3);

	std_data->accelation_x[0] = cy_data->accelation_x[0];
	std_data->accelation_x[1] = cy_data->accelation_x[1];
	std_data->accelation_x[2] = cy_data->accelation_x[2];
	std_data->accelation_x[3] = cy_data->accelation_x[3];
	std_data->accelation_x[4] = '.';
	std_data->accelation_x[5] = cy_data->accelation_x[4];

	std_data->accelation_y[0] = cy_data->accelation_y[0];
	std_data->accelation_y[1] = cy_data->accelation_y[1];
	std_data->accelation_y[2] = cy_data->accelation_y[2];
	std_data->accelation_y[3] = cy_data->accelation_y[3];
	std_data->accelation_y[4] = '.';
	std_data->accelation_y[5] = cy_data->accelation_y[4];

	memcpy(std_data->status, cy_data->status, 2);

	/* choyoung's dtg does not have external data. */
	/* so external data is '0'. */

	memset(std_data->day_oil_usage, '0', 9);
	memset(std_data->cumulative_oil_usage, '0', 9);
	memcpy(std_data->temperature_A, "+00.0", 5);
	memcpy(std_data->temperature_B, "+00.0", 5);
	memset(std_data->residual_oil, '0', 7);
	return ret;
}

static int last_read_num = 0;	

void save_record_data()
{
	int i;
	tacom_cy_data_t *cy_data;
	int retry_cnt = 5;
	FILE *fptr = NULL;

	//jwrho file save patch ++
	while(retry_cnt-- > 0)
	{
		fptr = fopen("/var/ch_stored_records", "w" );
		if(fptr != NULL)
			break;
		sleep(1);
	}

	if(fptr == NULL)
		return;

	//jwrho file save patch --
	curr_idx = curr;
	while ((MAX_CY_DATA_PACK > recv_avail_cnt) && (recv_bank[curr_idx].status == DATA_PACK_FULL))
	{
		for (i = 0; i < recv_bank[curr_idx].count; i++) {
			cy_data = &recv_bank[curr_idx].buf[i];
			fwrite(cy_data, 1, sizeof(tacom_cy_data_t), fptr);
		}
		recv_bank[curr_idx].status = DATA_PACK_EMPTY;

		curr_idx++;
		if  (curr_idx == MAX_CY_DATA_PACK) {
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
	tacom_cy_data_t *cy_data;

	if(file_save_flag == 1)
	{
		save_record_data();
		return 1;
	}

	//jwrho ++
	unread_count = cy_unreaded_records_num(tm);
	DTG_LOGD("std_parsing> cj_unreaded_records_num = [%d]\n", unread_count);
	if(unread_count <= 0)
		return -1;

	while (cy_header_status == CY_HEADER_EMPTY) {
		DTG_LOGD("std_parsing> cy_header_status is CY_HEADER_EMPTY\n");
		sleep(2);
	}
	do {
		ret = valid_check((char *)&cy_header, sizeof(tacom_cy_hdr_t));
		if (ret < 0) {
			DTG_LOGE("wrong format dtg header");
			cy_header_status = CY_HEADER_EMPTY;

		}
		else 
		{
			//already success
			if(_header_tmp == 1) 
			{
				ret = valid_check((char *)&cy_header_tmp, sizeof(tacom_cy_hdr_t));
				if(ret >= 0) {
					//if(memcmp(cy_header.regist_num, cy_header_tmp.regist_num, 12)) {
					if(memcmp(&cy_header, &cy_header_tmp, sizeof(tacom_cy_hdr_t))) {						
						memcpy(&cy_header, &cy_header_tmp, sizeof(tacom_cy_hdr_t));
						dtg_info_file_save(DTG_INFO_FILE_PATH, cy_header);
					}
				}
			}

			break;
		}
	} while (ret < 0);
	//jwrho --

	std_hdr = (tacom_std_hdr_t *)&tm->tm_strm.stream[dest_idx];
	memcpy(std_hdr->vehicle_model, cy_header.dtg_model, 20);
	memcpy(std_hdr->vehicle_id_num, cy_header.vehicle_id_num, 17);
	memcpy(std_hdr->vehicle_type, cy_header.vehicle_type, 2);
	memcpy(std_hdr->registration_num, cy_header.regist_num, 12);
	memcpy(std_hdr->business_license_num, cy_header.business_lsn, 10);
	memcpy(std_hdr->driver_code, cy_header.driver_code, 18);

	dest_idx += sizeof(tacom_std_hdr_t);
	if (request_num == 1) {
		std_data = (tacom_std_data_t *)&tm->tm_strm.stream[dest_idx];
		cy_data = &read_curr_buf;
		ret = data_convert(std_data, cy_data);
		if (ret < 0)
			return ret;
		dest_idx += sizeof(tacom_std_data_t);
		r_num++;
	} else {
		curr_idx = curr;

/*
		while ((MAX_CY_DATA_PACK > recv_avail_cnt) && 
			(recv_bank[curr_idx].status == DATA_PACK_FULL) &&
			//((request_num + MAX_CY_DATA) <= tm->tm_setup->max_records_per_once)) {
			(r_num <= tm->tm_setup->max_records_per_once)) {
*/
		r_num = 0;
		while(1)
		{
			if(recv_bank[curr_idx].status != DATA_PACK_FULL)
			{
				DTG_LOGE("bank status is not full pack...r_num[%d]\n", r_num);
				if(r_num <= 0) {
					cy_ack_records(0);
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
				cy_data = (tacom_cy_data_t *)&recv_bank[curr_idx].buf[i];

				ret = data_convert(std_data, cy_data);
				if (ret < 0)
					continue;//return ret; //jwrho 2015-01-17
				dest_idx += sizeof(tacom_std_data_t);
				r_num++;
			}
			curr_idx++;
			if  (curr_idx == MAX_CY_DATA_PACK) {
				curr_idx = 0;
			}
		}

		if(unread_count > 500 && r_num < 100) {
			DTG_LOGE("unread_count = [%d]", unread_count);
			DTG_LOGE("MAX_CY_DATA_PACK/recv_avail_cnt = [%d/%d]", MAX_CY_DATA_PACK, recv_avail_cnt);
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

int cy_read_current(){
	int ret = 0;

	TACOM *tm = tacom_get_cur_context();

	ret = valid_check((char *)&read_curr_buf, sizeof(tacom_cy_data_t));
	if(ret < 0) {
		DTG_LOGE("%s: wrong dtg data", __func__);
		return -1;
	}

	return std_parsing(tm, 1, 0);
}

int cy_read_records (int r_num)
{
	int ret;
	
	TACOM *tm = tacom_get_cur_context();

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

int cy_ack_records(int readed_bytes)
{
	int r_num = 0;
	int i;
	int unread_bank_cnt = 0;

	TACOM *tm = tacom_get_cur_context();

	//printf("WARNING TEST____CODE %s Just Return............\n", __func__);
	//return 0;

	r_num = last_read_num;
	DTG_LOGT("%s:%d> end[%d] curr_idx[%d] : curr[%d] : recv_avail_cnt[%d]\n", __func__, __LINE__, end, curr_idx, curr, recv_avail_cnt);

	if (curr_idx == curr) {
		DTG_LOGE("Bank full flush. end[%d], curr_idx[%d], curr[%d]", end, curr_idx, curr);
		curr = curr_idx = end;
	} else if (curr_idx < curr) {
		memset(&recv_bank[curr], 0, (MAX_CY_DATA_PACK - curr) * sizeof(cy_data_pack_t));
		memset(recv_bank, 0, curr_idx * sizeof(cy_data_pack_t));
		recv_avail_cnt += (r_num / MAX_CY_DATA);
		curr = curr_idx;
	} else {
		memset(&recv_bank[curr], 0, (curr_idx - curr) * sizeof(cy_data_pack_t));
		recv_avail_cnt += (r_num / MAX_CY_DATA);
		curr = curr_idx;
	}

	DTG_LOGT("%s:%d> trace\n", __func__, __LINE__);
	//jwrho 2015.01.21++
	unread_bank_cnt = 0;
	for(i = 0; i < MAX_CY_DATA_PACK; i++)
		if(recv_bank[i].status == DATA_PACK_FULL)
			unread_bank_cnt += 1;

			DTG_LOGT("%s:%d> trace\n", __func__, __LINE__);
	if( (MAX_CY_DATA_PACK - unread_bank_cnt) != recv_avail_cnt)
	{
		DTG_LOGE("patch #3 recv_avail_cnt : [%d] -> [%d]", recv_avail_cnt, (MAX_CY_DATA_PACK - unread_bank_cnt));
		recv_avail_cnt = (MAX_CY_DATA_PACK - unread_bank_cnt);
	}
	//jwrho 2015.01.21--

	DTG_LOGT("%s:%d> finish\n", __func__, __LINE__);
	return 0;
}



const struct tm_ops cy_ops = {
	cy_init_process,
	NULL,
	NULL,
	NULL,
	NULL,
	cy_read_current,
	NULL,
	cy_unreaded_records_num,
	cy_read_records,
	NULL,
	cy_ack_records,
};
