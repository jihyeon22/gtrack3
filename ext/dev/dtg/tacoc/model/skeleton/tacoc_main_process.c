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
#include <util/nettool.h>

#include <standard_protocol.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include <base/dmmgr.h>

extern void tacoc_ignition_off();

int g_dtg_no_rcv_count = 0;
int g_key_on_report = 0;

void tacoc_ignition_off_process()
{

}

extern int get_server_no_ack_count();
void *do_dtg_report_thread()
{
	int current_time = 0;
	int key_on_report_cnt = 0;
	int key_status;
	int remain_count = 0;
	int send_cnt = 0;
	int double_check_count = 0;
	int report_time = 0;
	int ret;
	int overhead_time = 0;

	DTG_LOGD("%s: %s() --", __FILE__, __func__);
	while(1) {
		current_time = get_modem_time_utc_sec();
		key_status = power_get_ignition_status();

		if(key_status == POWER_IGNITION_OFF)
		{
			key_on_report_cnt = 0;
			g_key_on_report = 0;
			report_time = current_time;
			double_check_count = 0;
			sleep(10);
			continue;
		}
		else
		{
			if(g_key_on_report == 0) {
				if(key_on_report_cnt > 10) {
					g_key_on_report = 1;
				}

				if(data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1) < 0) {
					key_on_report_cnt += 1;
					sleep(10);
					continue;
				}
				g_key_on_report = 1;
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
					break;
				}
				else
				{
					if(send_cnt > 0) {
						//if(remain_count < get_dtg_report_period())
						//	break;
						if( ( ( 100 * remain_count) / get_dtg_report_period() ) < 50)
							break;
					}
					send_cnt += 1;
					if(send_cnt > 1)
						data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t), 1); //send latest one data into server 

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
	//pthread_t p_thread_mdt;
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
	// dmmgr_init("/system/mds/system/bin/dm.ini", "/system/mds/system/bin/PACKAGE");

	// dmmgr_send(eEVENT_PWR_ON, NULL, 0);
	if(power_get_ignition_status() == POWER_IGNITION_ON)
		send_device_registration();
	else
		send_device_de_registration();

	if (pthread_create(&p_thread_dtg, NULL, do_dtg_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
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
#else
	pthread_join(p_thread_dtg, (void **) status);
	DTG_LOGT("TACOC ETRACE MODEL Exit");
#endif
	return 0;
}

