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


#include <wrapper/dtg_log.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>

#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include <base/dmmgr.h>

#include <tacom/tacom_inc.h>

#include <board/power.h>

#include "vehicle_msg.h"

extern void tacoc_ignition_off();

int g_dtg_no_rcv_count = 0;
int g_kt_fota_dtg_no_rcv_count = 0;


void tacoc_ignition_off_process()
{

}



#if defined(BOARD_TL500K)
	extern int get_server_no_ack_count();
#endif

extern int sent_group;
void *do_dtg_report_thread()
{
	int d_time = 0, ret = 0;
	time_t time_ent, time_out;
	int key_status;
	int net_fail_cnt = 0;
	int remain_count = 0;
	int send_cnt = 0;
	char log_buf[128];

	int report_time = 0;
	int current_time = 0;
	int mdt_create_time = 0;
	int mdt_current_time = 0;
	int double_check_count = 0;

	DTG_LOGD("%s: %s() ++", __FILE__, __func__);
/*
while(1) {
	DTG_LOGI("DM Send Msg....#3+++++");
	// dmmgr_send(eEVENT_PWR_ON, NULL, 0);
	DTG_LOGI("DM Send Msg....#3-----");
	sleep(5);
}
*/

	while(1)
	{
		DTG_LOGD("action thread wait ++");

		key_status = power_get_ignition_status();
		if(key_status == POWER_IGNITION_OFF)
			tacoc_ignition_off_process();

		
		current_time = get_modem_time_utc_sec();
		DTG_LOGI("DTG Report Period -----> [%d/%d]\n", (current_time - report_time), get_dtg_report_period());

		if(current_time - report_time >= get_dtg_report_period()) {
			report_time = current_time;
			double_check_count = 0;
		}
		else {
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

		ret = 0;
		remain_count = 0;

		mdt_create_time = 0;
		mdt_current_time = current_time;

		send_cnt = 0;
		while(1)
		{
			DTG_LOGD("action thread received");
			mdt_current_time = get_modem_time_utc_sec();
			d_time = get_modem_time_utc_sec();
			if(send_cnt > 10)
			{
				DTG_LOGE("send count 10 times over.");
				break;
			}
			send_cnt += 1;
			remain_count = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);
			DTG_LOGI("%s:%d> REMAIN_DATA [%d]", __func__, __LINE__, remain_count);
			if (remain_count > 0) 
			{
				g_dtg_no_rcv_count = 0;
				g_kt_fota_dtg_no_rcv_count = 0;
				ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_dtg_report_period(), 0, 2);
				DTG_LOGI("%s:%d> ACCUMAL_DATA [%d]", __func__, __LINE__, ret); //if success, then return 0
				if (ret < 0) {
					DTG_LOGE("dtg data read error");
					if(ret == -2222) {
						DTG_LOGE("dtg data rserver send error");
						sleep(3);
						//break; //remove by jwrho 2015.01.17
					}
					break; //add by jwrho 2015.01.17
				} else if (ret == 101){
					DTG_LOGD("not enough data records.");
					data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
					break;
				} else {
					ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
					if (ret < 0) {
						DTG_LOGE("cannot clear dtg buffer.");
						break;
					}
					sleep(3);
				}

				remain_count = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);
				DTG_LOGI("%s:%d> REMAIN_DATA [%d]", __func__, __LINE__, remain_count);
				if(remain_count < (get_dtg_report_period()))
				{
					break;
				}
				else
				{
					DTG_LOGI("#1 mdt create [%d/%d]\n", mdt_current_time-mdt_create_time, get_dtg_report_period());
					if(mdt_current_time - mdt_create_time >= get_dtg_report_period()) {
						mdt_create_time = mdt_current_time;
						data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(mdt_pck_t), 1); //mdt data create
					}
				}
			}
			else
			{
				DTG_LOGE("dtg unread record call error remain_count[%d]. no_rcv_cnt[%d]", remain_count, g_dtg_no_rcv_count);
				if(g_dtg_no_rcv_count == 5) 
				{
					sprintf(log_buf, "DTG NO DATA!!! key[%d], remain_cnt[%d]", power_get_ignition_status(), remain_count);
					dmmgr_send(eEVENT_WARNING, log_buf, 0);
					g_dtg_no_rcv_count = 0;
				}
				else
				{
					g_dtg_no_rcv_count += 1;
					if(g_kt_fota_dtg_no_rcv_count < 2)
						g_kt_fota_dtg_no_rcv_count += 1;
				}

				break;
			}
		} //end while

#if defined(BOARD_NEO_W200K)
/*
		LOGT("server no ack cnt = [%d]", get_server_no_ack_count());
		if(get_server_no_ack_count() >= 3)
		{
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save
			//device reset
			while(1) {
				LOGE("Server No ACK....Device Reset...");
				system("poweroff");
				sleep(1);
			}
		}
*/
#endif

	DTG_LOGI("#2 mdt create [%d/%d]\n", mdt_current_time-mdt_create_time, get_dtg_report_period());
	if(mdt_current_time - mdt_create_time >= get_dtg_report_period()) 
		ret = data_req_to_taco_cmd(CURRENT_DATA, 3, sizeof(mdt_pck_t), 1); //mdt data create


		current_time = get_modem_time_utc_sec();
		DTG_LOGE("d_time = [%d]\n", current_time - d_time);
		//report_time = (get_modem_time_utc_sec());
		report_time = (get_modem_time_utc_sec()-(current_time-d_time));
		
	}
	DTG_LOGD("%s: %s() --", __FILE__, __func__);
}

int main_process()
{
	DTG_LOGT("TACOC HANURI-TIEN MODEL!!");
	
	pthread_t p_thread_dtg;

	fprintf(stderr, "check_point============== %s : %d\n", __func__, __LINE__);
	init_configuration_data();	
	fprintf(stderr, "check_point============== %s : %d\n", __func__, __LINE__);

#ifdef BOARD_NEO_W200K
/*
	#if ! (defined(DEVICE_MODEL_LOOP) || defined(DEVICE_MODEL_LOOP2) || defined(DEVICE_MODEL_CHOYOUNG) )
		send_fota_req_packet(ePOLLING_MODE, eHTTP, 30);
		
		if(get_ignition_source() == POWER_IGNITION_ON)
			send_device_on_packet(30);
		else
			send_device_off_packet(30);

		if(get_kt_fota_qry_report() <= 0)
			send_qty_packet(ePOLLING_MODE, eDEVICE_MODEM_QTY_MODE, 30);
	#endif
*/
#endif
	if (pthread_create(&p_thread_dtg, NULL, do_dtg_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

#if 0   

	alarm(1);

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
	

		DTG_LOGI("%s> main power status check...\n", __func__);
		sleep(5);
	}
#endif
	pthread_join(p_thread_dtg, NULL);
	DTG_LOGT("TACOC TRIPOS MODEL Exit");

	return 0;
}

