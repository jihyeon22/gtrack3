#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <wrapper/dtg_atcmd.h>
#include <board/board_system.h>
#include <board/modem-time.h>
#include <board/battery.h>
#include <board/power.h>
#include <util/tools.h>
#include <util/nettool.h>


#include <standard_protocol.h>

#include <wrapper/dtg_log.h>
//// #include <taco_rpc.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include "sms_msg_process.h"
#include <base/dmmgr.h>

#include <wrapper/dtg_log.h>
#include <tacom/tacom_inc.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_taco_wrapper_rpc_clnt.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>

extern void tacoc_ignition_off();

int g_dtg_no_rcv_count = 0;
int g_kt_fota_dtg_no_rcv_count = 0;

#ifdef SERVER_ABBR_DSKL
int g_key_off_count = 0;
int g_device_boot_time = 0;
#endif

void tacoc_ignition_off_process()
{
#ifdef SERVER_ABBR_DSKL
	int key_off_status_count = 0;
	int key_status;
	int key_off_time = 0;
	key_status = power_get_ignition_status();

	if(key_status == POWER_IGNITION_OFF)
		g_key_off_count += 1;

	while(key_status == POWER_IGNITION_OFF)
	{
		DTG_LOGI("%s> wait ign on status...\n", __func__);
		sleep(5);
		key_status = power_get_ignition_status();

		if(get_modem_time_utc_sec() - g_device_boot_time > (24 * 3600)) {
			dmmgr_send(eEVENT_LOG, "regular poweroff #1", 0);
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
			gpio_set_value(15, 0);
#endif

			while(1) {
				//system("poweroff");
				DTG_LOGI("%s> wait powerorff...\n", __func__);
				poweroff(NULL,0);
				sleep(1);		
			}
		}
		key_off_time += 1;

		if(key_off_time > 600) { //keep key off status about 1 hour.
			key_off_time = 0;
			if(g_key_off_count > 100) {
				dmmgr_send(eEVENT_LOG, "regular poweroff #2", 0);
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
				gpio_set_value(15, 0);
#endif

				while(1) {
					//system("poweroff");
					DTG_LOGI("%s> wait powerorff...\n", __func__);
					poweroff(NULL,0);
					sleep(1);		
				}
			}
		}
		
	}

	if(tools_get_available_memory() < 4000) //4MB 
	{
#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
		gpio_set_value(15, 0);
#endif

		while(1) {
			//system("poweroff");
			DTG_LOGI("%s> wait powerorff...\n", __func__);
			poweroff(NULL,0);
			sleep(1);		
		}
	}
	else
	{
		g_dtg_no_rcv_count = 0;
		g_kt_fota_dtg_no_rcv_count = 0;
		send_device_registration();
		alarm(5);
	}
#endif
}


void clear_key_flag();
void clear_power_flag();
extern int get_server_no_ack_count();

#ifdef SERVER_ABBR_DSKL //this feature >> MDT Packet create with DTG data.
void *do_mdt_report_thread(void *pargs)
{
	unsigned int current_time = 0;
	unsigned int report_time = 0;
	int ret;

	report_time = current_time = get_modem_time_utc_sec();

	while(1)
	{
		if(power_get_ignition_status() == POWER_IGNITION_OFF)
		{
			DTG_LOGI("mdt wait ign on......");
			sleep(10);
			continue;
		}

		if(get_innoca_dtg_setting_enable() > 0) {
			printf("=========================================================>dtg init setting start....\n");
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x20000001, 0, 2);
			printf("ret = [%d]\n", ret);
			clear_innoca_dtg_setting_enable();
		}

		current_time = get_modem_time_utc_sec();
		if(current_time - report_time >= get_mdt_report_period()) {
			report_time = current_time;
			data_req_to_taco_cmd(CURRENT_DATA, 5, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1);
		}
		DTG_LOGI("mdt report time : [%d/%d]", current_time - report_time, get_mdt_report_period());
		sleep(3);
	}
	
}
#endif

void *do_dtg_report_thread(void *pargs)
{
	unsigned int current_time = 0;
	int key_on_report_cnt = 0;
	int key_status;
	int remain_count = 0;
	int send_cnt = 0;
	int double_check_count = 0;
	unsigned int report_time = 0;
	int ret;
	int overhead_time = 0;
	int power_on_report_cnt = 0;
	int key_on_report = 0;

	DTG_LOGD("%s: %s() --", __FILE__, __func__);

#ifdef SERVER_ABBR_DSKL
	g_device_boot_time = get_modem_time_utc_sec();
#endif

	while(power_on_report_cnt < 10) {
		DTG_LOGD("Power On Report...");
		if(data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1) >= 0)
			break;

		power_on_report_cnt += 1;
		sleep(10);
	}
	clear_power_flag();
	

	while(1) {
		current_time = get_modem_time_utc_sec();
		key_status = power_get_ignition_status();

		if(key_status == POWER_IGNITION_OFF)
		{
			tacoc_ignition_off_process();
			key_on_report_cnt = 0;
			key_on_report = 0;
			report_time = current_time;
			double_check_count = 0;
			sleep(10);
			continue;
		}
		else
		{
			if(key_on_report == 0) {
				if(key_on_report_cnt > 10) {
					key_on_report = 1;
					clear_key_flag();
				}

				if(data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1) < 0) {
					key_on_report_cnt += 1;
					sleep(10);
					continue;
				}
				key_on_report = 1;
			}
		}

		if(current_time - report_time >= get_dtg_report_period()) {
			report_time = current_time;
			double_check_count = 0;
		}
		else {
			DTG_LOGI("dtg report period ====> [%d/%d] chk_cnt[%d]\n", current_time - report_time, get_dtg_report_period(), double_check_count);
			double_check_count += 1;
			if(double_check_count < (get_dtg_report_period() / 10) + 2 ) {
				sleep(10);
				continue;
			}
			else {
				double_check_count = 0;
				report_time = current_time;
			}
		}
		
		if(key_status == POWER_IGNITION_ON)
		{
			send_cnt = 0;
			overhead_time = get_modem_time_utc_sec();
			while(1) {
				if(send_cnt > 10)
				{
					DTG_LOGT("send count 10 times over.");
					break;
				}

				remain_count = tacom_unreaded_records_num();
				if(remain_count < 0) {
					g_dtg_no_rcv_count += 1;
					if(g_kt_fota_dtg_no_rcv_count < 2)
						g_kt_fota_dtg_no_rcv_count += 1;

					break;
				}
				else
				{
					g_kt_fota_dtg_no_rcv_count = 0;
					if(send_cnt > 0) {
						//when sending time, if dtg data collected over 50 percent on period time, data send to server.
						if( ( ( 100 * remain_count) / get_dtg_report_period() ) < 50)
							break;
					}
					send_cnt += 1;

					ret = data_req_to_taco_cmd(CURRENT_DATA, 4, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1);
					if (ret < 0) {
						DTG_LOGE("dtg data read error");
						break;
					}

					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_dtg_report_period(), 0, 2);
					DTG_LOGI("%s:%d> ACCUMAL_DATA [%d]", __func__, __LINE__, ret);
					if (ret < 0) {
						DTG_LOGE("dtg data read error");
						if(ret == -2222) {
							DTG_LOGE("dtg data rserver send error");
							sleep(3);
						}
						break; //add by jwrho 2015.01.17
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
						break;
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
						if (ret < 0) {
							DTG_LOGE("cannot clear dtg buffer.");
							break;
						}
						sleep(3);
					}
				}
			} //end while
			overhead_time = get_modem_time_utc_sec() - overhead_time;
			if(overhead_time > get_dtg_report_period())
				overhead_time = 0;

			DTG_LOGD("Packet Overhead time = [%d]", overhead_time);
			report_time = get_modem_time_utc_sec() - overhead_time;
		}
	}
}

int main_process()
{
	pthread_t p_thread_mdt;
	pthread_t p_thread_dtg;
	//int status;
	//char *phonenum = NULL;
	int net_check_count = 300;

	DTG_LOGT("TACOC ETRACE MODEL!!");

	while(net_check_count-- > 0)
	{
		if(nettool_get_state() == DEFINES_MDS_OK)
			break;

		sleep(2);
		DTG_LOGI("taoc waiting for network enable");
	}
	
	init_configuration_data();

#ifdef SERVER_ABBR_DSKL //this feature >> MDT Packet create with DTG data.
	 //dmmgr_init("/system/mds/system/bin/dm.ini", "/system/mds/system/bin/PACKAGE"); no need, this code is in gtrack base.
	 dmmgr_send(eEVENT_PWR_ON, NULL, 0);
#endif

	if(power_get_ignition_status() == POWER_IGNITION_ON)
	{
		send_device_registration();
	}
	else
	{
		//check update
		check_update();
		send_device_de_registration();
	}

#ifdef SERVER_ABBR_DSKL //this feature >> MDT Packet create with DTG data.
	if (pthread_create(&p_thread_mdt, NULL, do_mdt_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}
#endif

	if (pthread_create(&p_thread_dtg, NULL, do_dtg_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

#if defined(BOARD_TL500S) || defined(BOARD_TL500K) || defined(BOARD_TL500L)
	#ifdef SERVER_ABBR_DSKL
	int	key_status, old_key_status;
    old_key_status = key_status = power_get_ignition_status();
	while(1)
	{
		key_status = power_get_ignition_status();
		if(key_status == POWER_IGNITION_OFF)
		{
			if(old_key_status == POWER_IGNITION_ON)
			{
				if (power_get_power_source() == POWER_SRC_DC)
					tacoc_ignition_off();
			}
		}
		old_key_status = key_status;
	

		DTG_LOGD("%s> main power status check...\n", __func__);
		sleep(5);
	}
	#endif
#else
	pthread_join(p_thread_dtg, (void **) status);
	DTG_LOGT("TACOC ETRACE MODEL Exit");
#endif
	return 0;
}

