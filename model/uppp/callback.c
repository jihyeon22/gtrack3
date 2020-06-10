#include <stdio.h>
#include <unistd.h>
#include <string.h>

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
#include <logd_rpc.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "logd/logd_rpc.h"

#include <at/at_util.h>
#include <board/board_system.h>
#include <include/defines.h>
#include <mdsapi/mds_api.h>

#define LOG_TARGET eSVC_MODEL


static int flag_run_thread_main = 1;
static void chk_apps_port();
static void tl500_network_interface_down();
static void set_uart_chk_flag_for_bootstrap();

void init_model_callback(void)
{
	configurationModel_t *conf = get_config_model();

	printf("gtrack calback ::: init_model_callback !!!\r\n");
	//thread_network_set_warn_timeout(MAX(conf->model.report_interval_keyon, conf->model.report_interval_keyoff) * 2);
	set_max_network_fail_reset_cnt(0);
	chk_apps_port();
	tl500_network_interface_down();
}

void network_on_callback(void)
{
	printf("gtrack calback ::: network_on_callback !!!\r\n");
}

void button1_callback(void)
{
	printf("gtrack calback ::: button1_callback !!!\r\n");
}

void button2_callback(void)
{
	printf("gtrack calback ::: button2_callback !!!\r\n");
}

void ignition_on_callback(void)
{
	printf("gtrack calback ::: ignition_on_callback !!!\r\n");
}

void ignition_off_callback(void)
{
	printf("gtrack calback ::: ignition_off_callback !!!\r\n");

}

void power_on_callback(void)
{	
	printf("gtrack calback ::: power_on_callback !!!\r\n");

}

void power_off_callback(void)
{
	printf("gtrack calback ::: power_off_callback !!!\r\n");
	poweroff(NULL, 0);
}

void gps_parse_one_context_callback(void)
{
	printf("gtrack calback ::: gps_parse_one_context_callback !!!\r\n");

}

#define NETWORK_DISABLE_INTERVAL_SEC 	30
#define ECHO_AT_INTERVAL_SEC 			3600

void main_loop_callback(void)
{
	int main_cnt = 0;
	int echo_at_cnt = 0;
	
	tl500_network_interface_down();
	set_uart_chk_flag_for_bootstrap();
	at_set_apn_form_cgdcont(1, AT_APN_IP_TYPE_IPV4, "privatelte.ktfwing.com");

	while(flag_run_thread_main)
	{
		printf("gtrack calback ::: main_loop_callback !!!\r\n");
		watchdog_set_cur_ktime(eWdMain);
		watchdog_process();

		LOGI(LOG_TARGET, "[uppp-model] net disable cnt !!! [%d]/[%d]\n", main_cnt, NETWORK_DISABLE_INTERVAL_SEC );

		if ( main_cnt > NETWORK_DISABLE_INTERVAL_SEC)
		{
			tl500_network_interface_down();
			main_cnt = 0;
		}

		// ppp ����Ǹ�, at noti �߻����� ����
		// �׷��� �ֱ������� at �� �����ش�.
		if ( echo_at_cnt > ECHO_AT_INTERVAL_SEC )
		{
			send_at_cmd("at");
			echo_at_cnt = 0;
		}
		
		main_cnt++;
		echo_at_cnt++;
		sleep(1);
	}
}

void terminate_app_callback(void)
{
	printf("gtrack calback ::: terminate_app_callback !!!\r\n");
}

void exit_main_loop(void)
{
	printf("gtrack calback ::: exit_main_loop !!!\r\n");

	flag_run_thread_main = 0;
}


#define AT_CMD_CHK_MAX_RETRY 10
static void set_uart_chk_flag_for_bootstrap()
{
	char touch_cmd[128] = {0,};

	sprintf(touch_cmd, "touch %s &", CHK_UART_FLAG_PATH);
	system(touch_cmd);
}

static void chk_apps_port()
{
	int i = 0;
	int apps_port = 0;
	// ------------- CHK UART DS SERVICE ---------------
	for ( i = 0 ; i < AT_CMD_CHK_MAX_RETRY ; i++ )
	{
		if (get_apps_port_tl500(&apps_port) == AT_RET_SUCCESS )
		{
			printf("[uppp-model] apps_port is [%d]\r\n", apps_port);
			LOGI(LOG_TARGET, "[uppp-model] apps_port is [%d]\r\n", apps_port);
			break;
		}	
	}
	
	if ( apps_port != DS_PORT_TO_UART1 )
	{
		devel_webdm_send_log( "APPS PORT CHANGE : NEED TO CHANGE PORT");
		printf("[uppp-model] need to chage apps port [%d]\n", apps_port);
		LOGI(LOG_TARGET, "[uppp-model] need to chage apps port [%d]\n", apps_port);

		for ( i = 0 ; i < AT_CMD_CHK_MAX_RETRY ; i++ )
		{
			if ( set_apps_port_tl500(DS_PORT_TO_UART1) == AT_RET_SUCCESS )
			{
				devel_webdm_send_log( "APPS PORT CHANGE : POWEROFF SEQ");

				printf("[uppp-model] poweroff.. [%d]\n", apps_port);
				LOGI(LOG_TARGET, "[uppp-model] poweroff.. [%d]\n", apps_port);

				poweroff(NULL,0);
				break;
			}
		}
	}
}

static void tl500_network_interface_down()
{
	static int force_disconn = 0 ;
	static int disconn_fail_cnt = 0;
	LOGI(LOG_TARGET, "[uppp-model] net disable !!! disconn fail cnt [%d] \n", disconn_fail_cnt );
	if ( ( force_disconn++ > 10) || (nettool_get_state() == DEFINES_MDS_OK) )
	{
		system(NETIF_DOWN_CMD);
		send_at_cmd("at$$apcall=0");
		force_disconn = 0;

	}

	// ifdown ������ ������ ��Ʈ��ũ �������̽��� �ִ�.
	if (nettool_get_state() == DEFINES_MDS_OK)
		disconn_fail_cnt ++;
	else
		disconn_fail_cnt = 0;

	if ( disconn_fail_cnt > 20 )
		poweroff("null", 0);
}
