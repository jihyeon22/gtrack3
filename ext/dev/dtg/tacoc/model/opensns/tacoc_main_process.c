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
#include <taco_rpc.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"
#include "dtg_regist_process.h"
#include "server_packet.h"
#include "parsing.h"
#include <base/dmmgr.h>

extern void tacoc_ignition_off();

int g_dtg_no_rcv_count = 0;
int g_kt_fota_dtg_no_rcv_count = 0;
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)


int g_data_send_complete = 0;
void tacoc_ignition_off_process(unsigned int on_time)
{

}

#endif

void clear_key_flag();
void clear_power_flag();
extern int get_server_no_ack_count();
void *do_mdt_report_thread(void *pargs)
{
	unsigned int current_time = 0;
	unsigned int report_time = 0;
	int ret;
	int request_cnt;//get_mdt_create_period
	int remain_count;
	int check_count = 10;
	int key_on_report = 0;

	//report_time = current_time = get_modem_time_utc_sec();
	report_time = current_time = 0;

	while(1)
	{
		if(power_get_ignition_status() == POWER_IGNITION_OFF)
		{
			DTG_LOGI("mdt wait ign on......");
			report_time = current_time = 0;
			key_on_report = 0;
			sleep(10);
			continue;
		}

		if(get_innoca_dtg_setting_enable() > 0) {
			printf("=========================================================>dtg init setting start....\n");
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x20000001, 0, 2); //dtg header info clear
			printf("ret = [%d]\n", ret);
			set_innoca_dtg_setting_enable(0);
		}

		if(get_dtg_mdt_period_update_enable() > 0)
		{
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x20000002, 0, 2); //dtg mdt create period update
			printf("ret = [%d]\n", ret);
			set_dtg_mdt_period_update_enable(0); //clear
		}
		//

		current_time = get_modem_time_utc_sec();
		if(current_time - report_time >= get_mdt_report_period()) 
		{
			check_count = 10;
			while(1)
			{
				current_time = report_time = get_modem_time_utc_sec();

				if(power_get_ignition_status() == POWER_IGNITION_OFF)
					break;

				if(check_count <= 0)
					break;

				remain_count = tacom_unreaded_records_num();
				remain_count = ((remain_count & 0xFFFF0000) >> 16);
				DTG_LOGT("MDT Current Count = [%d]", remain_count);
				if(remain_count <= 0)
				{
					check_count += 1;
					sleep(5);
					continue;
				}
				else if(  key_on_report == 1 && (remain_count < (get_mdt_report_period() / get_mdt_create_period())))
				{
					//as collected data is a little, it will send at next time.
					break;
				}
				else
				{
					request_cnt = get_mdt_report_period() / get_mdt_create_period();
					request_cnt = (0x40000000 | (request_cnt & 0x0000FFFF));
					ret = data_req_to_taco_cmd(ACCUMAL_MDT_DATA, request_cnt, 0, 2); //mdt data
					if (ret < 0) {
						DTG_LOGE("mdt data read error");
						if(ret == -2222) {
							DTG_LOGE("mdt data server send error");
							sleep(3);
						}
						break; //add by jwrho 2015.01.17
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
						break;
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 2, 0, 1); //mdt data clear
						if (ret < 0) {
							DTG_LOGE("cannot clear mdt buffer.");
							break;
						}
						key_on_report = 1;
						sleep(3);
					}
				}
			} //end while
		} //if(current_time - report_time >= get_mdt_report_period())
		DTG_LOGI("mdt report time : [%d/%d]", current_time - report_time, get_mdt_report_period());
		sleep(6);
	} //end while

	return 0;
}
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

	unsigned int system_on_time;

	DTG_LOGD("%s: %s() --", __FILE__, __func__);


	system_on_time = current_time = report_time = get_modem_time_utc_sec();

	while(1) {
		current_time = get_modem_time_utc_sec();
		key_status = power_get_ignition_status();

		if(key_status == POWER_IGNITION_OFF)
		{
			tacoc_ignition_off_process(system_on_time);
			key_on_report_cnt = 0;
			key_on_report = 0;
			report_time = current_time;
			double_check_count = 0;
			sleep(10);
			continue;
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
				remain_count = (remain_count & 0x0000FFFF);
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
						//while sending data takes long time by any reason(network traffic), dtg data collect for that time.
						//so, if collected data is over 50 percent on get_dtg_report_period(), it just send this period time. NOT next period time.
						if( ( ( 100 * remain_count) / get_dtg_report_period() ) < 50)
							break;
					}
					send_cnt += 1;

					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_dtg_report_period(), 0, 2);
					DTG_LOGI("%s:%d> ACCUMAL_DATA [%d]", __func__, __LINE__, ret);
					if (ret < 0) {
						DTG_LOGE("dtg data read error");
						if(ret == -2222) {
							DTG_LOGE("dtg data server send error");
							sleep(3);
						}
						break; //add by jwrho 2015.01.17
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
						break;
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1); //dtg data clear
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

	} //end while
}

int main_process()
{
	pthread_t p_thread_mdt;
	pthread_t p_thread_dtg;
	time_t system_on_time;
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

/*
	while(1)
	{
		data_req_to_taco_cmd(ACCUMAL_DATA, 180 | 0x40000000, 0, 2); //0x400000b4
		sleep(2);
		data_req_to_taco_cmd(CLEAR_DATA, 2, 3, 1); //2 fact
		sleep(2);
	}
*/ //for test to have conversation with taco process.
	

	init_configuration_data();
	// dmmgr_init("/system/mds/system/bin/dm.ini", "/system/mds/system/bin/PACKAGE");

	// dmmgr_send(eEVENT_PWR_ON, NULL, 0);
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

	if (pthread_create(&p_thread_mdt, NULL, do_mdt_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	if (pthread_create(&p_thread_dtg, NULL, do_dtg_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	//system_on_time;

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
					set_dtg_key_status(eDTG_Key_Invalid);
					g_data_send_complete = 1;
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

