<<<<<<< HEAD
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/stackdump.h>
#include <logd_rpc.h>

#include <board/board_system.h>
#include <board/modem-time.h>
#include <board/uart.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "welding_machine.h"
#include "data-list.h"
#include "custom.h"

#define LOG_TARGET eSVC_MODEL

static WELDING_MACHINE_CMD_t g_cmd_list;
static int g_welding_machine_uard_fd = 0;
static char g_welding_machine_id[128];
static int g_welding_machine_noworking = 0;

int welding_machine_config_setup()
{
	configurationModel_t *conf = get_config_model();

	memset(&g_cmd_list, 0x00, sizeof(WELDING_MACHINE_CMD_t));

	g_cmd_list.IVS_cmd.enable = 1;
	g_cmd_list.IVS_cmd.interval = 1;
	if(conf->model.InV_T_collection_interval > 0) {
		g_cmd_list.InV_T_cmd.enable = 1;
		g_cmd_list.InV_T_cmd.interval = conf->model.InV_T_collection_interval;
		g_cmd_list.InV_T_cmd.inv_prev_time = 0;
		g_cmd_list.InV_T_cmd.t_prev_time = 0;
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
	int n_try = 1000;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	tout.tv_sec = ftime;
	tout.tv_usec = 0;
	result = select(fd + 1, &reads, 0, 0, &tout);
	if(result <= 0) //time out & select error
		return len;

	while(n_try-- > 0)
	{
		uart_len = read(fd, &(buf[len]), buf_len-len);
		if(uart_len <= 0)
			break;
		
		len += uart_len;
	}

	return len;
}


static int g_uart_rev_thread_flag = 0;


int get_id_data(char *src_buf, char *dest_buf, int dest_len)
{
	char *temp_bp = NULL;
	char token_1[ ] = "]";
	char *tr;

	tr = strtok_r(src_buf, token_1, &temp_bp);
	if(tr == NULL) {
		printf("%s> can't find ']' character : %s\n", __func__, src_buf);
		LOGE(eSVC_MODEL, "%s> can't find ']' character : %s\n", __func__, src_buf);
		return -1;
	}
			
	memset(dest_buf, 0x00, dest_len);
	strcpy(dest_buf, tr);
	strcat(dest_buf, "]");

	printf("welding_machine_id : %s\n", dest_buf);
	LOGI(eSVC_MODEL, "welding_machine_id : %s\n", dest_buf);
	g_cmd_list.ID_cmd.initial_flag = 1;

	return 1;
}

int get_ivs_data(char *src_buf, char *dest_buf, int dest_len)
{
	char *temp_bp = NULL;
	char token_0[ ] = ",";
	char ivs_backup[128];
	char *tr;
	time_t system_time;
	struct tm *timeinfo = NULL;
	struct tm cur_time;

	memset(ivs_backup, 0x00, sizeof(ivs_backup));
	memcpy(ivs_backup, src_buf, sizeof(ivs_backup)-1);

	tr = strtok_r(src_buf, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	
	if(atoi(tr) == 0) {
		g_welding_machine_noworking = 1;
		printf("IVS 4th data zero : %s \n", ivs_backup);
		LOGE(eSVC_MODEL, "IVS 4th data zero : %s \n", ivs_backup);
		return -1;
	}
	
	g_welding_machine_noworking = 0;

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	if(timeinfo == NULL) {
		printf("get time info is NULL error\n");
		LOGE(eSVC_MODEL, "get time info is NULL error\n");
		return -2;
	}
	//timeinfo->tm_year+1900, 
	//timeinfo->tm_mon+1,
	//timeinfo->tm_mday,
	//timeinfo->tm_hour, 
	//timeinfo->tm_min, 
	//timeinfo->tm_sec,
	sprintf(dest_buf, "[#,%02d%02d%02d]%s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ivs_backup);
	return 0;

}
int check_command(char *src_buf, char *dest_buf, int dest_len)
{
	int cmd_type;
	char *cmd_list[] = {"[ID,", "[IVS,", "[InV,", "[T,", NULL};
	int i;
	char *p_cmd = NULL;

	i = 0;
	while(1) {
		if(cmd_list[i] == NULL) {
			printf("%s> unknown cmd no find error : %s\n", __func__, src_buf);
			LOGI(eSVC_MODEL, "%s> unknown cmd no find error : %s\n", __func__, src_buf);
			break;
		}

		p_cmd = strstr(src_buf, cmd_list[i]);
		if(p_cmd != NULL) {
			cmd_type = i;
			break;
		}
		i += 1;

	}

	switch(cmd_type) {
		case eID_CMD_CODE:
			if(get_id_data(src_buf, dest_buf, dest_len) < 0)
				return -2;
			break;
		case eIVS_CMD_CODE:
			if(get_ivs_data(src_buf, dest_buf, dest_len) < 0)
				return -3;
			break;
		case eInV_CMD_CODE:
			if(g_welding_machine_noworking == 1 || g_cmd_list.IVS_cmd.enable == 0) {
				LOGE(eSVC_MODEL, "g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable [%d, %d]\n", g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable);
				return -4;
			}
		
			strcpy(dest_buf, src_buf);
			break;
		case eT_CMD_CODE:
			if(g_welding_machine_noworking == 1 || g_cmd_list.IVS_cmd.enable == 0) {
				LOGE(eSVC_MODEL, "g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable [%d, %d]\n", g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable);
				return -5;
			}

			strcpy(dest_buf, src_buf);
			break;
		default:
			cmd_type = eUnknown_CMD_CODE;
			strcpy(dest_buf, src_buf);
			break;
	}

	return cmd_type;

}

void *do_welding_machine_uart_rev_thread()
{
	configurationModel_t *conf = get_config_model();
	char buf[512];
	char dest[512];
	int len;
	time_t currnet_time;
	int cmd_type;
	char *p_packet_data;
	int thread_debug = 0;

	while(1) {

		conf = get_config_model();

		if(g_welding_machine_uard_fd <= 0) {
			g_welding_machine_uard_fd = init_uart(UART0_DEV_NAME, conf->model.board_rate);
			if(g_welding_machine_uard_fd < 0) {
				LOGE(eSVC_MODEL, "%s> %s device open error \n", __func__, UART0_DEV_NAME);
				sleep(1);
				continue;
			}
		}

		memset(buf, 0x00, sizeof(buf));
		if((len = _wait_read(g_welding_machine_uard_fd, (unsigned char *)buf, sizeof(buf), 30)) <= 0) {
			usleep(1000*100); //100ms delay
			continue;
		}

		if(thread_debug++ > 20) {
			thread_debug = 0;
			LOGD(eSVC_MODEL, "Thread Alive = [%d]\n", len);
		}

		LOGD(eSVC_MODEL, "UART Received org data = [%s]\n", buf);

		currnet_time = get_modem_time_utc_sec();

		memset(dest, 0x00, sizeof(dest));
		cmd_type = check_command(buf, dest, sizeof(dest));
		if(cmd_type < 0) {
			printf("check_command error return [%d]\n", cmd_type);
			LOGE(eSVC_MODEL, "check_command error return [%d]\n", cmd_type);
			continue;
		}

		switch(cmd_type) {
			case eID_CMD_CODE:
				LOGI(eSVC_MODEL, "eID_CMD_CODE Detection\n");
				memset(g_welding_machine_id, 0x00, sizeof(g_welding_machine_id));
				strcpy(g_welding_machine_id, dest);
				printf("g_welding_machine_id : %s\n", g_welding_machine_id);
				break;

			case eIVS_CMD_CODE:
				LOGI(eSVC_MODEL, "eIVS_CMD_CODE Detection\n");
				if(g_cmd_list.IVS_cmd.enable == 0) {
					printf("IVSSTP_cmd Enable, Skip IVS Collection\n");
					LOGI(eSVC_MODEL, "IVSSTP_cmd Enable, Skip IVS Collection\n");
					continue;
				}

				if(currnet_time - g_cmd_list.IVS_cmd.prev_time >= g_cmd_list.IVS_cmd.interval) {
					g_cmd_list.IVS_cmd.prev_time = currnet_time;
					
					p_packet_data = (char *)malloc(strlen(dest)+2);
					if(p_packet_data == NULL) {
						LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
						continue;
					}
					strcpy(p_packet_data, dest);
					if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
					{
						LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
						free(p_packet_data);
					}

					printf("[IVS] collect data : %s\n", dest);
					LOGD(eSVC_MODEL, "[IVS] collect data : %s\n", dest);
					printf("get_data_count = [%d]\n", get_data_count());
				} 
				break;

			case eInV_CMD_CODE:
				LOGI(eSVC_MODEL, "eInV_CMD_CODE Detection\n");
				printf("g_cmd_list.InV_T_cmd.interval : [%d]\n", g_cmd_list.InV_T_cmd.interval);

				if( (g_cmd_list.InV_T_cmd.interval == 0) || (currnet_time - g_cmd_list.InV_T_cmd.inv_prev_time >= g_cmd_list.InV_T_cmd.interval) ) {
					g_cmd_list.InV_T_cmd.inv_prev_time = currnet_time;
					
					p_packet_data = (char *)malloc(strlen(dest)+2);
					if(p_packet_data == NULL) {
						LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
						continue;
					}
					strcpy(p_packet_data, dest);
					if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
					{
						LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
						free(p_packet_data);
					}

					printf("[InV] collect data : %s\n", dest);
					LOGD(eSVC_MODEL, "[InV] collect data : %s\n", dest);
					printf("get_data_count = [%d]\n", get_data_count());
				}
				break;
			
			case eT_CMD_CODE:
				LOGI(eSVC_MODEL, "eT_CMD_CODE Detection\n");
				if( (g_cmd_list.InV_T_cmd.interval == 0) || (currnet_time - g_cmd_list.InV_T_cmd.t_prev_time >= g_cmd_list.InV_T_cmd.interval) ) {
					g_cmd_list.InV_T_cmd.t_prev_time = currnet_time;
					
					p_packet_data = (char *)malloc(strlen(dest)+2);
					if(p_packet_data == NULL) {
						LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
						continue;
					}
					strcpy(p_packet_data, dest);
					if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
					{
						LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
						free(p_packet_data);
					}
					printf("[T] collect data : %s\n", dest);
					LOGD(eSVC_MODEL, "[T] collect data : %s\n", dest);
					printf("get_data_count = [%d]\n", get_data_count());
				}
				break;
			
			default: // eUnknown_CMD_CODE
				LOGI(eSVC_MODEL, "eUnknown_CMD_CODE : %s\n", dest);
				p_packet_data = (char *)malloc(strlen(dest)+2);
				if(p_packet_data == NULL) {
					LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
					continue;
				}
				strcpy(p_packet_data, dest);
				if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
				{
					LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
					free(p_packet_data);
				}
				printf("[Unkown Command] collect data : %s, %s\n", dest, p_packet_data);
				LOGD(eSVC_MODEL, "[Unkown Command] collect data : %s\n", dest);
				printf("get_data_count = [%d]\n", get_data_count());

				break;
		} //end switch

	} //end while

	return NULL;
}

int cmd_debug = 0;
int welding_machine_process(int debug_num)
{

	configurationModel_t *conf = get_config_model();
	int result = 0;
	time_t current_time;
	pthread_t p_thread_action;
		
	if(g_uart_rev_thread_flag == 0) {
		if (pthread_create(&p_thread_action, NULL, do_welding_machine_uart_rev_thread, NULL) < 0) {
			fprintf(stderr, "cannot create p_thread_action thread\n");
			exit(1);
		}

		g_uart_rev_thread_flag = 1;
	}

	current_time = get_modem_time_utc_sec();
	if(current_time <= 0) { //meaning : funtion error or time isn't set up yet.
		LOGE(eSVC_MODEL, "%s> time isn't set up yet.\n", __func__);
		return -1;
	}


	if(g_welding_machine_uard_fd <= 0) {
		g_welding_machine_uard_fd = init_uart(UART0_DEV_NAME, conf->model.board_rate);
		if(g_welding_machine_uard_fd < 0) {
			LOGE(eSVC_MODEL, "%s> %s device open error \n", __func__, UART0_DEV_NAME);
			return -2;
		}
		sleep(3);
	}

	if(g_cmd_list.ID_cmd.initial_flag == 0) {
		result = uart_write(g_welding_machine_uard_fd, "[ID]", strlen("[ID]"));
		LOGD(eSVC_MODEL, "[ID] uart_write result = [%d]\n", result);

		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [ID] command request error \n", __func__);
			return -3;
		}
		sleep(3);
	}

	if(g_cmd_list.ID_cmd.initial_flag == 0) {
		LOGE(eSVC_MODEL, "[%d] %s> welding machind id don't get yet. \n", debug_num, __func__);
		return 0; //welding machind id don't get yet.
	}
	
	//current_time

	

	//LOGD(eSVC_MODEL, "[%d] %s> [IVS] command request\n", debug_num, __func__);
	result = uart_write(g_welding_machine_uard_fd, "[IVS]", strlen("[IVS]"));
	LOGD(eSVC_MODEL, "[IVS] uart_write result = [%d]\n", result);
	if(result <= 0) {
		LOGE(eSVC_MODEL, "%s> [IVS] command request error \n", __func__);
		return -3;
	}
	


	if(g_cmd_list.InV_T_cmd.enable == 1) {
		if(g_cmd_list.InV_T_cmd.interval == 0) { //one time control command
			g_cmd_list.InV_T_cmd.enable = 0;
		}

		usleep(1000*50); //50ms delay
		//LOGD(eSVC_MODEL, "[%d] %s> [InV] command request\n", debug_num, __func__);
		result = uart_write(g_welding_machine_uard_fd, "[InV]", strlen("[InV]"));
		LOGD(eSVC_MODEL, "[InV] uart_write result = [%d]\n", result);
		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [InV] command request error \n", __func__);
			return -3;
		}

		usleep(1000*50); //50ms delay
		//LOGD(eSVC_MODEL, "[%d] %s> [T] command request\n", debug_num, __func__);
		result = uart_write(g_welding_machine_uard_fd, "[T]", strlen("[T]"));
		//printf("[T] uart_write result = [%d]\n", result);
		LOGD(eSVC_MODEL, "[T] uart_write result = [%d]\n", result);
		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [T] command request error \n", __func__);
			return -3;
		}
		
	}

	if(g_cmd_list.ADDCLT_cmd.enable == 1) {
		g_cmd_list.ADDCLT_cmd.enable = 0;
		usleep(1000*50); //50ms delay
		result = uart_write(g_welding_machine_uard_fd, g_cmd_list.ADDCLT_cmd.user_cmd, strlen(g_cmd_list.ADDCLT_cmd.user_cmd));
		//printf("%s uart_write result = [%d]\n", g_cmd_list.ADDCLT_cmd.user_cmd, result);
		LOGD(eSVC_MODEL, "%s uart_write result = [%d]\n", g_cmd_list.ADDCLT_cmd.user_cmd, result);
		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [InV] command request error \n", __func__);
			return -3;
		}
	}

	return 0;
}


char *get_welding_machine_id()
{
	if(g_cmd_list.ID_cmd.initial_flag != 1)
		return NULL;

	return g_welding_machine_id;
}


void power_off_collect_stop()
{
	g_cmd_list.ADDCLT_cmd.enable = 0;
	g_cmd_list.IVS_cmd.enable = 0;
	g_cmd_list.InV_T_cmd.enable = 0;

}

=======
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/stackdump.h>
#include <logd_rpc.h>

#include <board/board_system.h>
#include <board/modem-time.h>
#include <board/uart.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "welding_machine.h"
#include "data-list.h"
#include "custom.h"

#define LOG_TARGET eSVC_MODEL

static WELDING_MACHINE_CMD_t g_cmd_list;
static int g_welding_machine_uard_fd = 0;
static char g_welding_machine_id[128];
static int g_welding_machine_noworking = 0;

int welding_machine_config_setup()
{
	configurationModel_t *conf = get_config_model();

	memset(&g_cmd_list, 0x00, sizeof(WELDING_MACHINE_CMD_t));

	g_cmd_list.IVS_cmd.enable = 1;
	g_cmd_list.IVS_cmd.interval = 1;
	if(conf->model.InV_T_collection_interval > 0) {
		g_cmd_list.InV_T_cmd.enable = 1;
		g_cmd_list.InV_T_cmd.interval = conf->model.InV_T_collection_interval;
		g_cmd_list.InV_T_cmd.inv_prev_time = 0;
		g_cmd_list.InV_T_cmd.t_prev_time = 0;
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
	int n_try = 1000;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	tout.tv_sec = ftime;
	tout.tv_usec = 0;
	result = select(fd + 1, &reads, 0, 0, &tout);
	if(result <= 0) //time out & select error
		return len;

	while(n_try-- > 0)
	{
		uart_len = read(fd, &(buf[len]), buf_len-len);
		if(uart_len <= 0)
			break;
		
		len += uart_len;
	}

	return len;
}


static int g_uart_rev_thread_flag = 0;


int get_id_data(char *src_buf, char *dest_buf, int dest_len)
{
	char *temp_bp = NULL;
	char token_1[ ] = "]";
	char *tr;

	tr = strtok_r(src_buf, token_1, &temp_bp);
	if(tr == NULL) {
		printf("%s> can't find ']' character : %s\n", __func__, src_buf);
		LOGE(eSVC_MODEL, "%s> can't find ']' character : %s\n", __func__, src_buf);
		return -1;
	}
			
	memset(dest_buf, 0x00, dest_len);
	strcpy(dest_buf, tr);
	strcat(dest_buf, "]");

	printf("welding_machine_id : %s\n", dest_buf);
	LOGI(eSVC_MODEL, "welding_machine_id : %s\n", dest_buf);
	g_cmd_list.ID_cmd.initial_flag = 1;

	return 1;
}

int get_ivs_data(char *src_buf, char *dest_buf, int dest_len)
{
	char *temp_bp = NULL;
	char token_0[ ] = ",";
	char ivs_backup[128];
	char *tr;
	time_t system_time;
	struct tm *timeinfo = NULL;
	struct tm cur_time;

	memset(ivs_backup, 0x00, sizeof(ivs_backup));
	memcpy(ivs_backup, src_buf, sizeof(ivs_backup)-1);

	tr = strtok_r(src_buf, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_0, &temp_bp);
	if(tr == NULL) return -1;
	
	if(atoi(tr) == 0) {
		g_welding_machine_noworking = 1;
		printf("IVS 4th data zero : %s \n", ivs_backup);
		LOGE(eSVC_MODEL, "IVS 4th data zero : %s \n", ivs_backup);
		return -1;
	}
	
	g_welding_machine_noworking = 0;

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	if(timeinfo == NULL) {
		printf("get time info is NULL error\n");
		LOGE(eSVC_MODEL, "get time info is NULL error\n");
		return -2;
	}
	//timeinfo->tm_year+1900, 
	//timeinfo->tm_mon+1,
	//timeinfo->tm_mday,
	//timeinfo->tm_hour, 
	//timeinfo->tm_min, 
	//timeinfo->tm_sec,
	sprintf(dest_buf, "[#,%02d%02d%02d]%s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, ivs_backup);
	return 0;

}
int check_command(char *src_buf, char *dest_buf, int dest_len)
{
	int cmd_type;
	char *cmd_list[] = {"[ID,", "[IVS,", "[InV,", "[T,", NULL};
	int i;
	char *p_cmd = NULL;

	i = 0;
	while(1) {
		if(cmd_list[i] == NULL) {
			printf("%s> unknown cmd no find error : %s\n", __func__, src_buf);
			LOGI(eSVC_MODEL, "%s> unknown cmd no find error : %s\n", __func__, src_buf);
			break;
		}

		p_cmd = strstr(src_buf, cmd_list[i]);
		if(p_cmd != NULL) {
			cmd_type = i;
			break;
		}
		i += 1;

	}

	switch(cmd_type) {
		case eID_CMD_CODE:
			if(get_id_data(src_buf, dest_buf, dest_len) < 0)
				return -2;
			break;
		case eIVS_CMD_CODE:
			if(get_ivs_data(src_buf, dest_buf, dest_len) < 0)
				return -3;
			break;
		case eInV_CMD_CODE:
			if(g_welding_machine_noworking == 1 || g_cmd_list.IVS_cmd.enable == 0) {
				LOGE(eSVC_MODEL, "g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable [%d, %d]\n", g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable);
				return -4;
			}
		
			strcpy(dest_buf, src_buf);
			break;
		case eT_CMD_CODE:
			if(g_welding_machine_noworking == 1 || g_cmd_list.IVS_cmd.enable == 0) {
				LOGE(eSVC_MODEL, "g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable [%d, %d]\n", g_welding_machine_noworking, g_cmd_list.IVS_cmd.enable);
				return -5;
			}

			strcpy(dest_buf, src_buf);
			break;
		default:
			cmd_type = eUnknown_CMD_CODE;
			strcpy(dest_buf, src_buf);
			break;
	}

	return cmd_type;

}

void *do_welding_machine_uart_rev_thread()
{
	configurationModel_t *conf = get_config_model();
	char buf[512];
	char dest[512];
	int len;
	time_t currnet_time;
	int cmd_type;
	char *p_packet_data;
	int thread_debug = 0;

	while(1) {

		conf = get_config_model();

		if(g_welding_machine_uard_fd <= 0) {
			g_welding_machine_uard_fd = init_uart(UART0_DEV_NAME, conf->model.board_rate);
			if(g_welding_machine_uard_fd < 0) {
				LOGE(eSVC_MODEL, "%s> %s device open error \n", __func__, UART0_DEV_NAME);
				sleep(1);
				continue;
			}
		}

		memset(buf, 0x00, sizeof(buf));
		if((len = _wait_read(g_welding_machine_uard_fd, (unsigned char *)buf, sizeof(buf), 30)) <= 0) {
			usleep(1000*100); //100ms delay
			continue;
		}

		if(thread_debug++ > 20) {
			thread_debug = 0;
			LOGD(eSVC_MODEL, "Thread Alive = [%d]\n", len);
		}

		LOGD(eSVC_MODEL, "UART Received org data = [%s]\n", buf);

		currnet_time = get_modem_time_utc_sec();

		memset(dest, 0x00, sizeof(dest));
		cmd_type = check_command(buf, dest, sizeof(dest));
		if(cmd_type < 0) {
			printf("check_command error return [%d]\n", cmd_type);
			LOGE(eSVC_MODEL, "check_command error return [%d]\n", cmd_type);
			continue;
		}

		switch(cmd_type) {
			case eID_CMD_CODE:
				LOGI(eSVC_MODEL, "eID_CMD_CODE Detection\n");
				memset(g_welding_machine_id, 0x00, sizeof(g_welding_machine_id));
				strcpy(g_welding_machine_id, dest);
				printf("g_welding_machine_id : %s\n", g_welding_machine_id);
				break;

			case eIVS_CMD_CODE:
				LOGI(eSVC_MODEL, "eIVS_CMD_CODE Detection\n");
				if(g_cmd_list.IVS_cmd.enable == 0) {
					printf("IVSSTP_cmd Enable, Skip IVS Collection\n");
					LOGI(eSVC_MODEL, "IVSSTP_cmd Enable, Skip IVS Collection\n");
					continue;
				}

				if(currnet_time - g_cmd_list.IVS_cmd.prev_time >= g_cmd_list.IVS_cmd.interval) {
					g_cmd_list.IVS_cmd.prev_time = currnet_time;
					
					p_packet_data = (char *)malloc(strlen(dest)+2);
					if(p_packet_data == NULL) {
						LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
						continue;
					}
					strcpy(p_packet_data, dest);
					if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
					{
						LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
						free(p_packet_data);
					}

					printf("[IVS] collect data : %s\n", dest);
					LOGD(eSVC_MODEL, "[IVS] collect data : %s\n", dest);
					printf("get_data_count = [%d]\n", get_data_count());
				} 
				break;

			case eInV_CMD_CODE:
				LOGI(eSVC_MODEL, "eInV_CMD_CODE Detection\n");
				printf("g_cmd_list.InV_T_cmd.interval : [%d]\n", g_cmd_list.InV_T_cmd.interval);

				if( (g_cmd_list.InV_T_cmd.interval == 0) || (currnet_time - g_cmd_list.InV_T_cmd.inv_prev_time >= g_cmd_list.InV_T_cmd.interval) ) {
					g_cmd_list.InV_T_cmd.inv_prev_time = currnet_time;
					
					p_packet_data = (char *)malloc(strlen(dest)+2);
					if(p_packet_data == NULL) {
						LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
						continue;
					}
					strcpy(p_packet_data, dest);
					if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
					{
						LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
						free(p_packet_data);
					}

					printf("[InV] collect data : %s\n", dest);
					LOGD(eSVC_MODEL, "[InV] collect data : %s\n", dest);
					printf("get_data_count = [%d]\n", get_data_count());
				}
				break;
			
			case eT_CMD_CODE:
				LOGI(eSVC_MODEL, "eT_CMD_CODE Detection\n");
				if( (g_cmd_list.InV_T_cmd.interval == 0) || (currnet_time - g_cmd_list.InV_T_cmd.t_prev_time >= g_cmd_list.InV_T_cmd.interval) ) {
					g_cmd_list.InV_T_cmd.t_prev_time = currnet_time;
					
					p_packet_data = (char *)malloc(strlen(dest)+2);
					if(p_packet_data == NULL) {
						LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
						continue;
					}
					strcpy(p_packet_data, dest);
					if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
					{
						LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
						free(p_packet_data);
					}
					printf("[T] collect data : %s\n", dest);
					LOGD(eSVC_MODEL, "[T] collect data : %s\n", dest);
					printf("get_data_count = [%d]\n", get_data_count());
				}
				break;
			
			default: // eUnknown_CMD_CODE
				LOGI(eSVC_MODEL, "eUnknown_CMD_CODE : %s\n", dest);
				p_packet_data = (char *)malloc(strlen(dest)+2);
				if(p_packet_data == NULL) {
					LOGE(LOG_TARGET, "%s> report packet malloc error : %d\n", __func__, errno);
					continue;
				}
				strcpy(p_packet_data, dest);
				if(list_add(&welding_machine_buffer_list, p_packet_data) < 0)
				{
					LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
					free(p_packet_data);
				}
				printf("[Unkown Command] collect data : %s, %s\n", dest, p_packet_data);
				LOGD(eSVC_MODEL, "[Unkown Command] collect data : %s\n", dest);
				printf("get_data_count = [%d]\n", get_data_count());

				break;
		} //end switch

	} //end while

	return NULL;
}

int cmd_debug = 0;
int welding_machine_process(int debug_num)
{

	configurationModel_t *conf = get_config_model();
	int result = 0;
	time_t current_time;
	pthread_t p_thread_action;
		
	if(g_uart_rev_thread_flag == 0) {
		if (pthread_create(&p_thread_action, NULL, do_welding_machine_uart_rev_thread, NULL) < 0) {
			fprintf(stderr, "cannot create p_thread_action thread\n");
			exit(1);
		}

		g_uart_rev_thread_flag = 1;
	}

	current_time = get_modem_time_utc_sec();
	if(current_time <= 0) { //meaning : funtion error or time isn't set up yet.
		LOGE(eSVC_MODEL, "%s> time isn't set up yet.\n", __func__);
		return -1;
	}


	if(g_welding_machine_uard_fd <= 0) {
		g_welding_machine_uard_fd = init_uart(UART0_DEV_NAME, conf->model.board_rate);
		if(g_welding_machine_uard_fd < 0) {
			LOGE(eSVC_MODEL, "%s> %s device open error \n", __func__, UART0_DEV_NAME);
			return -2;
		}
		sleep(3);
	}

	if(g_cmd_list.ID_cmd.initial_flag == 0) {
		result = uart_write(g_welding_machine_uard_fd, "[ID]", strlen("[ID]"));
		LOGD(eSVC_MODEL, "[ID] uart_write result = [%d]\n", result);

		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [ID] command request error \n", __func__);
			return -3;
		}
		sleep(3);
	}

	if(g_cmd_list.ID_cmd.initial_flag == 0) {
		LOGE(eSVC_MODEL, "[%d] %s> welding machind id don't get yet. \n", debug_num, __func__);
		return 0; //welding machind id don't get yet.
	}
	
	//current_time

	

	//LOGD(eSVC_MODEL, "[%d] %s> [IVS] command request\n", debug_num, __func__);
	result = uart_write(g_welding_machine_uard_fd, "[IVS]", strlen("[IVS]"));
	LOGD(eSVC_MODEL, "[IVS] uart_write result = [%d]\n", result);
	if(result <= 0) {
		LOGE(eSVC_MODEL, "%s> [IVS] command request error \n", __func__);
		return -3;
	}
	


	if(g_cmd_list.InV_T_cmd.enable == 1) {
		if(g_cmd_list.InV_T_cmd.interval == 0) { //one time control command
			g_cmd_list.InV_T_cmd.enable = 0;
		}

		usleep(1000*50); //50ms delay
		//LOGD(eSVC_MODEL, "[%d] %s> [InV] command request\n", debug_num, __func__);
		result = uart_write(g_welding_machine_uard_fd, "[InV]", strlen("[InV]"));
		LOGD(eSVC_MODEL, "[InV] uart_write result = [%d]\n", result);
		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [InV] command request error \n", __func__);
			return -3;
		}

		usleep(1000*50); //50ms delay
		//LOGD(eSVC_MODEL, "[%d] %s> [T] command request\n", debug_num, __func__);
		result = uart_write(g_welding_machine_uard_fd, "[T]", strlen("[T]"));
		//printf("[T] uart_write result = [%d]\n", result);
		LOGD(eSVC_MODEL, "[T] uart_write result = [%d]\n", result);
		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [T] command request error \n", __func__);
			return -3;
		}
		
	}

	if(g_cmd_list.ADDCLT_cmd.enable == 1) {
		g_cmd_list.ADDCLT_cmd.enable = 0;
		usleep(1000*50); //50ms delay
		result = uart_write(g_welding_machine_uard_fd, g_cmd_list.ADDCLT_cmd.user_cmd, strlen(g_cmd_list.ADDCLT_cmd.user_cmd));
		//printf("%s uart_write result = [%d]\n", g_cmd_list.ADDCLT_cmd.user_cmd, result);
		LOGD(eSVC_MODEL, "%s uart_write result = [%d]\n", g_cmd_list.ADDCLT_cmd.user_cmd, result);
		if(result <= 0) {
			LOGE(eSVC_MODEL, "%s> [InV] command request error \n", __func__);
			return -3;
		}
	}

	return 0;
}


char *get_welding_machine_id()
{
	if(g_cmd_list.ID_cmd.initial_flag != 1)
		return NULL;

	return g_welding_machine_id;
}


void power_off_collect_stop()
{
	g_cmd_list.ADDCLT_cmd.enable = 0;
	g_cmd_list.IVS_cmd.enable = 0;
	g_cmd_list.InV_T_cmd.enable = 0;

}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
