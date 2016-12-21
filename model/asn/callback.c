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

static int flag_run_thread_main = 1;

#define UART_MAX_RECEIVE_LENGTH		(8*1024)
#define UART_MAX_WAIT_TIME			(2)
#define UART_RETRY_COUNT			(5)

static int g_uart_fd = -1;

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	LOGD(eSVC_MODEL, "gtrack calback ::: init_model_callback !!!\r\n");
	//thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	thread_network_set_warn_timeout(conf->model.network_not_use_warning_time);
}

void network_on_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: network_on_callback !!!\r\n");
}

void button1_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: button1_callback !!!\r\n");
}

void button2_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: button2_callback !!!\r\n");
}

void ignition_on_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: ignition_on_callback !!!\r\n");
}

void ignition_off_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: ignition_off_callback !!!\r\n");

}

void power_on_callback(void)
{	
	LOGD(eSVC_MODEL, "gtrack calback ::: power_on_callback !!!\r\n");

}

void power_off_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: power_off_callback !!!\r\n");

}

void gps_parse_one_context_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: gps_parse_one_context_callback !!!\r\n");

}

void ans_cmd_forward(char *cmd, int cmd_len)
{
	if(cmd != NULL && cmd_len > 0)
		uart_write(g_uart_fd, cmd, cmd_len);
}
void main_loop_callback(void)
{
	int ret;
	int readcnt = 0;
	int read_err_cnt = UART_RETRY_COUNT;
	int uart_wait_time = 10;
	unsigned char uart_buf[512];
	unsigned char time_req_recv_data[REQ_TIME_RESPONSE_LENGTH];
	int i;
	/*
	char *p_test = (char *)-1;

	printf("==========================================\n");
	printf("null pointer test ...\n");
	printf("%d\n", *p_test);
	printf("==========================================\n");
	*/
	check_internal_uart_use();

	setting_network_param();
	
	ret = create_watchdog("asn.main_loop_callback", 10*60); //3min
	create_watchdog("gtrack.daily.reset", 60*60*48);
	watchdog_set_cur_ktime(eWdMain);


	while(flag_run_thread_main && nettool_get_state() != DEFINES_MDS_OK) {
		//LOGI(LOG_TARGET, "%s : first time wating untill network enable..\n", __func__);
		LOGD(eSVC_MODEL, "%s : first time wating untill network enable..\n", __func__);
		watchdog_keepalive_id("asn.main_loop_callback");
		sleep(3);
	}

	while(flag_run_thread_main)
	{
		LOGD(eSVC_MODEL, "%s> main loop...\r\n", __func__);
		watchdog_keepalive_id("asn.main_loop_callback");
		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();
		
		if(check_uart_fd(&g_uart_fd) < 0) {
			LOGD(eSVC_MODEL, "s> uart handle open error\r\n", __func__);
			sleep(1);
			continue;
		}

		readcnt = uart_read(g_uart_fd, uart_buf, sizeof(uart_buf), uart_wait_time);
		if(readcnt <= 0) //read error 
		{
			uart_buffer_data_network_push(); //buffering uart data network transfer
			uart_wait_time = 10;

			if(read_err_cnt-- < 0) {
				read_err_cnt = UART_RETRY_COUNT;
				if(g_uart_fd > 0) {
					LOGD(eSVC_MODEL, "No Received Data while %d sec\n", UART_MAX_WAIT_TIME*UART_RETRY_COUNT);
					close(g_uart_fd);
					g_uart_fd = -1;
				}
				continue;
			}
			
		}
		else
		{
			uart_wait_time = 1;
			read_err_cnt = UART_RETRY_COUNT;
			if(check_time_request_packet(uart_buf, readcnt) == 0) { //packet detect
	
				watchdog_keepalive_id("asn.main_loop_callback");
				LOGT(eSVC_MODEL, "check_time_request_packet OK : uart len[%d]\n", readcnt);

				dump_data("request_server_time", uart_buf, readcnt);
				memset(time_req_recv_data, 0x00, sizeof(time_req_recv_data));

				if(request_server_time(uart_buf, readcnt, time_req_recv_data, sizeof(time_req_recv_data)) == 0) { //packet swing success.
					dump_data("request_server_response", time_req_recv_data, sizeof(time_req_recv_data));

					uart_write(g_uart_fd, time_req_recv_data, sizeof(time_req_recv_data));
					continue;
				}
			}
			else if(check_cu_power_on_reset_packet(uart_buf, readcnt) == 0) { //packet detect
				dump_data("power_on_reset", uart_buf, readcnt);

				LOGT(eSVC_MODEL, "check_cu_power_on_reset_packet OK : uart len[%d]\n", readcnt);
				continue;
			}
			else {
				if(readcnt < 10) {
					LOGE(eSVC_MODEL, "default uart data filter under 10 length data\n");
					continue;
				}

				LOGT(eSVC_MODEL, "default uart data : uart len[%d]\n", readcnt);
				dump_data("normal uart data", uart_buf, readcnt);

				add_uart_buffer(uart_buf, readcnt);
			}
		}
	}
}

void terminate_app_callback(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: terminate_app_callback !!!\r\n");
}

void exit_main_loop(void)
{
	LOGD(eSVC_MODEL, "gtrack calback ::: exit_main_loop !!!\r\n");

	flag_run_thread_main = 0;
}
