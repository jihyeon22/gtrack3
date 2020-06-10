<<<<<<< HEAD
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <wrapper/dtg_atcmd.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"

void dtg_alarm_callback(int signo)
{
	DTG_LOGD("<dtg_alarm_callback> : g_dtg_config.action_working[%d]", is_working_action());

	if(is_working_action()) {
		alarm(get_create_period());
		return;
	}

	set_signal();
}

void *do_dtg_action_thread()
{
	int d_time, ret;
	time_t time_ent, time_out;
	int fail_cnt = 0;
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);
	while(1)
	{
		DTG_LOGD("action thread wait ++");
		
		lock_mutex();
		wait_for_singleobject();
		time(&time_ent);
		DTG_LOGD("action thread received");

		update_total_create_period();
		if(get_total_create_period() < get_trans_period())
		{
			save_report_msg();
		}
		else
		{
			ret = data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(msg_mdt_t), 1);

			if (ret == 0) {
				fail_cnt = 0;
				if(get_serial_port_mode() == UT_DEFAULT_MODE)
				{
					DTG_LOGD("++++++ TRIPOS STEP_3\n");
					save_report_msg();
					send_saved_report_msg();
					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_create_period(), 0, 2);
					if (ret < 0) {
						DTG_LOGE("dtg data read error.");
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
						if (ret < 0)
							DTG_LOGE("cannot clear dtg buffer.");
					}
					if(ret != 3)
						mode_change_to_ut1_2_mode();
				}
				else
				{
					DTG_LOGD("++++++ TRIPOS STEP_4\n");
					send_saved_report_msg();
					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_create_period(), 0, 2);
					if (ret < 0) {
						DTG_LOGE("dtg data read error.");
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
						if (ret < 0)
							DTG_LOGE("cannot clear dtg buffer.");
					}
					if(ret == 3)
						mode_change_to_ut_default_mode();
				}
				reset_total_create_period();
			} else {
				if (fail_cnt > 3){
					g_connect_to_server();
					ret = send_zero_data_msg();
					g_disconnect_to_server();
					if(ret < 0) {
						DTG_LOGE("Message Trans Error - send_zero_data_msg");
					}
					fail_cnt = 0;
				} else {
					fail_cnt++;
				}
			}
		}
		DTG_LOGD("++++++ TRIPOS STEP_6(%d)", get_create_period());
		time(&time_out);
		d_time = difftime(time_out, time_ent);
		if ((get_create_period() - (int)d_time) > 0)
			alarm(get_create_period() - (int)d_time);
		else
			alarm(get_create_period());
		unlock_mutex();
	}
	DTG_LOGD("%s: %s() --", __FILE__, __func__);
}

int main_process()
{
	pthread_t p_thread_action;
	int status;
	struct sigaction act;
	char *phonenum = NULL;

	act.sa_handler = dtg_alarm_callback;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, 0);

	DTG_LOGT("TACOC TRIPOS MODEL!!");
	
	init_configuration_data();

	if (pthread_create(&p_thread_action, NULL, do_dtg_action_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	while (1) {
		if ((phonenum = atcmd_get_phonenum()) == NULL) {
			DTG_LOGE("TACOC TRIPOS GET PHONE NUMBER FAILURE");
			sleep(1);
		}
		else
			DTG_LOGD("TACOC TRIPOS PHONE NUMBER : %s", phonenum);
			break;
	}

	set_dev_phone_number(phonenum);
	send_device_registration();

	alarm(1);

	//pthread_join(p_thread_action, (void **) status);
	//DTG_LOGT("TACOC TRIPOS MODEL Exit");
	return 0;
}

=======
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <wrapper/dtg_atcmd.h>

#include <wrapper/dtg_log.h>
// #include <taco_rpc.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"

void dtg_alarm_callback(int signo)
{
	DTG_LOGD("<dtg_alarm_callback> : g_dtg_config.action_working[%d]", is_working_action());

	if(is_working_action()) {
		alarm(get_create_period());
		return;
	}

	set_signal();
}

void *do_dtg_action_thread()
{
	int d_time, ret;
	time_t time_ent, time_out;
	int fail_cnt = 0;
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);
	while(1)
	{
		DTG_LOGD("action thread wait ++");
		
		lock_mutex();
		wait_for_singleobject();
		time(&time_ent);
		DTG_LOGD("action thread received");

		update_total_create_period();
		if(get_total_create_period() < get_trans_period())
		{
			save_report_msg();
		}
		else
		{
			ret = data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(msg_mdt_t), 1);

			if (ret == 0) {
				fail_cnt = 0;
				if(get_serial_port_mode() == UT_DEFAULT_MODE)
				{
					DTG_LOGD("++++++ TRIPOS STEP_3\n");
					save_report_msg();
					send_saved_report_msg();
					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_create_period(), 0, 2);
					if (ret < 0) {
						DTG_LOGE("dtg data read error.");
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
						if (ret < 0)
							DTG_LOGE("cannot clear dtg buffer.");
					}
					if(ret != 3)
						mode_change_to_ut1_2_mode();
				}
				else
				{
					DTG_LOGD("++++++ TRIPOS STEP_4\n");
					send_saved_report_msg();
					ret = data_req_to_taco_cmd(ACCUMAL_DATA, get_create_period(), 0, 2);
					if (ret < 0) {
						DTG_LOGE("dtg data read error.");
					} else if (ret == 101){
						DTG_LOGD("not enough data records.");
					} else {
						ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
						if (ret < 0)
							DTG_LOGE("cannot clear dtg buffer.");
					}
					if(ret == 3)
						mode_change_to_ut_default_mode();
				}
				reset_total_create_period();
			} else {
				if (fail_cnt > 3){
					g_connect_to_server();
					ret = send_zero_data_msg();
					g_disconnect_to_server();
					if(ret < 0) {
						DTG_LOGE("Message Trans Error - send_zero_data_msg");
					}
					fail_cnt = 0;
				} else {
					fail_cnt++;
				}
			}
		}
		DTG_LOGD("++++++ TRIPOS STEP_6(%d)", get_create_period());
		time(&time_out);
		d_time = difftime(time_out, time_ent);
		if ((get_create_period() - (int)d_time) > 0)
			alarm(get_create_period() - (int)d_time);
		else
			alarm(get_create_period());
		unlock_mutex();
	}
	DTG_LOGD("%s: %s() --", __FILE__, __func__);
}

int main_process()
{
	pthread_t p_thread_action;
	int status;
	struct sigaction act;
	char *phonenum = NULL;

	act.sa_handler = dtg_alarm_callback;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, 0);

	DTG_LOGT("TACOC TRIPOS MODEL!!");
	
	init_configuration_data();

	if (pthread_create(&p_thread_action, NULL, do_dtg_action_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	while (1) {
		if ((phonenum = atcmd_get_phonenum()) == NULL) {
			DTG_LOGE("TACOC TRIPOS GET PHONE NUMBER FAILURE");
			sleep(1);
		}
		else
			DTG_LOGD("TACOC TRIPOS PHONE NUMBER : %s", phonenum);
			break;
	}

	set_dev_phone_number(phonenum);
	send_device_registration();

	alarm(1);

	//pthread_join(p_thread_action, (void **) status);
	//DTG_LOGT("TACOC TRIPOS MODEL Exit");
	return 0;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
