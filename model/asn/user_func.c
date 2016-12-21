#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <board/board_system.h>
#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#include <base/mileage.h>
#endif
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <util/tools.h>
#include <board/uart.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/nettool.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include <at/at_util.h>
#include <at/at_log.h>
#include <at/watchdog.h>
#include <logd_rpc.h>
#include "cu_packet.h"
#include "data-list.h"
#include "user_func.h"


#define UART_BUFFER_MAX_SIZE	(1024*1024)
static char *g_uart_dev_name = UART0_DEV_NAME;
static int g_uart_br = 57600;
static char g_phone_number[20] = {0, };
static int g_uart_buffer_idx = 0;
static char g_uart_buffer[UART_BUFFER_MAX_SIZE];

void check_internal_uart_use()
{
	int channel_code;
	if(get_used_uart_channel_3gpp(&channel_code) != AT_RET_SUCCESS) {
		LOGD(eSVC_MODEL, "%s> get_used_uart_channel_3gpp fail\n", __func__);
		return;
	}

	if(channel_code != UART2_PORT_USED) {
		return;
	}


	set_used_uart_channel_3gpp(UART1_PORT_USED);
	system("touch /var/uart_ch.log &");
	
	while(1) {
		LOGD(eSVC_MODEL, "uart channel changing and reset\n");
		system("poweroff &");
		sleep(1);
	}
}

int check_uart_fd(int *fd)
{
	if(*fd < 0) {
		*fd = init_uart(g_uart_dev_name, g_uart_br);
		if(*fd < 0) {
			return -1;
		}
	}
	return 0;
}

int check_time_request_packet(unsigned char *packet_buf, int packet_len)
{
	cu_time_req_packet_body_t  *p_cu_time_req;
	
	p_cu_time_req = (cu_time_req_packet_body_t *)packet_buf;
	
	if(packet_len < sizeof(cu_time_req_packet_body_t))
		return -1;

	if(p_cu_time_req->id[0] != '8' || p_cu_time_req->id[1] != '7')
		return -1;

	if(p_cu_time_req->comma != ',')
		return -1;

	if(p_cu_time_req->crlf[0] != 0x0d || p_cu_time_req->crlf[1] != 0x0a)
		return -1;

	return 0;
}

int request_server_time(unsigned char *req_buf, int req_len, unsigned char *recv_buf, int recv_len)
{
	transferSetting_t network_param;

	LOGT(eSVC_MODEL, "%s> ++\n", __func__);
	memset(&network_param, 0x00, sizeof(transferSetting_t));
	get_time_request_network_param(&network_param);

	LOGT(eSVC_MODEL, "%s> transfer_packet_recv call\n", __func__);

	if(transfer_packet_recv(&network_param, req_buf, req_len, recv_buf, recv_len) < 0)
		return -1;

	return 0;	
}


int check_cu_power_on_reset_packet(unsigned char *packet_buf, int packet_len)
{
	configurationModel_t *conf = get_config_model();
	cu_por_reset_packet_body_t *p_cu_por_packet;
	p_cu_por_packet = (cu_por_reset_packet_body_t *)packet_buf;

	LOGD(eSVC_MODEL, "%s> ++\n", __func__);

	if(packet_len != sizeof(cu_por_reset_packet_body_t) || g_uart_buffer_idx != 0)
		return -1;

	if(p_cu_por_packet->id[0] != '7' || p_cu_por_packet->id[1] != '4')
		return -1;
	if(p_cu_por_packet->crlf_1[0] != 0x0d || p_cu_por_packet->crlf_1[1] != 0x0a)
		return -1;
	if(p_cu_por_packet->seperator != ']')
		return -1;
	if(p_cu_por_packet->crlf_2[0] != 0x0d || p_cu_por_packet->crlf_2[1] != 0x0a)
		return - 1;
									
									//change real-phone number.
	if(g_phone_number[0] == 0) {
		LOGD(eSVC_MODEL, "1.phone_number : %s\n", g_phone_number);
		at_get_phonenum(g_phone_number, sizeof(g_phone_number));
	}
	LOGD(eSVC_MODEL, "2.phone_number : %s\n", g_phone_number);
	strncpy(p_cu_por_packet->phone_num, g_phone_number, sizeof(p_cu_por_packet->phone_num));
	memcpy(&g_uart_buffer[g_uart_buffer_idx], packet_buf, packet_len);
	g_uart_buffer_idx += packet_len;
	uart_buffer_data_network_push(); //CU POR Reset Packet detection

	if(memcmp(conf->model.hubid, p_cu_por_packet->hubid, sizeof(p_cu_por_packet->hubid)))
	{
		memset(conf->model.hubid, 0x00, sizeof(conf->model.hubid));
		strncpy(conf->model.hubid, p_cu_por_packet->hubid, sizeof(p_cu_por_packet->hubid));
		save_config_user("user:HUBID", conf->model.hubid);
	}

	LOGD(eSVC_MODEL, "%s> ++\n", __func__);
	return 0;
}

void uart_buffer_data_network_push()
{
	unsigned char *p_packet = NULL;

	if(g_uart_buffer_idx <= 0) {
		LOGD(eSVC_MODEL, "%s> No Data...........\n", __func__);
		return;
	}
	/*
	  packet data structure
	  ------------------------------------------------
	  | data length | data body                      |
      ------------------------------------------------
	  | 4bytes      | variable length                |
	  ------------------------------------------------
	*/
	p_packet = (unsigned char *)malloc(g_uart_buffer_idx+4);
	if(p_packet == NULL) {
		LOGD(eSVC_MODEL, "%s> uart packet malloc error : %d\n", __func__, errno);
		return;
	}
	memcpy(p_packet, &g_uart_buffer_idx, sizeof(g_uart_buffer_idx));
	memcpy(&p_packet[4], g_uart_buffer, g_uart_buffer_idx);
	if(list_add(&cu_buffer_list, p_packet) < 0)
	{
		LOGD(eSVC_MODEL, "%s : list add fail\n", __func__);
		free(p_packet);
	}
				
	memset(g_uart_buffer, 0x00, sizeof(g_uart_buffer));
	g_uart_buffer_idx = 0;

	sender_add_data_to_buffer(0, NULL, ePIPE_1);
}

void add_uart_buffer(unsigned char *packet_buf, int packet_len)
{
	LOGD(eSVC_MODEL, "%s> ++\n", __func__);
	memcpy(&g_uart_buffer[g_uart_buffer_idx], packet_buf, packet_len);
	g_uart_buffer_idx += packet_len;

	if(g_uart_buffer_idx > UART_BUFFER_MAX_SIZE-1024) {
		uart_buffer_data_network_push();
	}
	LOGD(eSVC_MODEL, "%s> --\n", __func__);
}




void dump_data(char *debug_title, unsigned char *data, int data_len) 
{
	int i, j;
	char dump_buf[512];
	int idx;
	printf("%s ==============================>\n", debug_title);
	LOGI(eSVC_MODEL, "%s ==============================>\n", debug_title);
	memset(dump_buf, 0x00, sizeof(dump_buf));
	idx = 0;
	for(i = 0, j = 1; i < data_len; i++, j++) {
		if(data[i] == 0x0d) {
			strcpy(&dump_buf[idx], "<CR>");
			idx += 4;
		}
		else if(data[i] == 0x0a) {
			strcpy(&dump_buf[idx], "<LF>");
			idx += 4;

			printf("%s\n", dump_buf);
			LOGI(eSVC_MODEL, "%s\n", dump_buf);
			memset(dump_buf, 0x00, sizeof(dump_buf));
			idx = 0;
			j = 1;
		} else {
			dump_buf[idx++] = data[i];
		}
		if((j % 60) == 0) {
			printf("%s\n", dump_buf);
			LOGI(eSVC_MODEL, "%s\n", dump_buf);
			memset(dump_buf, 0x00, sizeof(dump_buf));
			idx = 0;
		}
	}
	if(idx > 0) {
		printf("%s\n", dump_buf);
		LOGI(eSVC_MODEL, "%s\n", dump_buf);
	}

	//if(data[0] == '7' && data[1] == '4')
	//	dmmgr_send(eEVENT_LOG, "Alive", 0);
}