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
		alarm(get_dtg_report_period());
		return;
	}

	set_signal();
}

void *do_ctrl_report_thread()
{
	int i, d_time, ret = 0;
	time_t time_ent, time_out;
	ctrl_pkt_t *mdt_buf;
	int ctrl_buf_size;
	int ctrl_buf_cnt;
	int tmp_index;

	while(1) {
		d_time = 0;
		i = 0;
		time(&time_ent);
		ret = data_req_to_taco_cmd(CURRENT_DATA, 2, sizeof(ctrl_pkt_t), 1);
		time(&time_out);
		d_time = difftime(time_out, time_ent);
		if ((get_ctrl_create_period() - (int)d_time) > 0)
			sleep(get_ctrl_create_period() - (int)d_time);
		else
			sleep(get_ctrl_create_period());
	}
}

extern int sent_group;
void *do_dtg_action_thread()
{
	int d_time, ret = 0;
	time_t time_ent, time_out;
	char dtg_buf[134*1024] = {0,};
	int backup_data_bit = 0;
	DTG_LOGD("%s: %s() ++", __FILE__, __func__);
	while(1)
	{
		DTG_LOGD("action thread wait ++");
		
		lock_mutex();
		wait_for_singleobject();
		time(&time_ent);
		DTG_LOGD("action thread received");
		ret = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);

		if (ret > 0) {
			ret = data_req_to_taco_cmd(ACCUMAL_DATA, 
				backup_data_bit | get_dtg_report_period(), 134*1024, 2);

			if (ret < 0) {
				DTG_LOGE("dtg data read error.");
			} else if (ret == 101){
				DTG_LOGD("not enough data records.");
			} else {
				ret = data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
				if (ret < 0) {
					backup_data_bit = 0x10000000;
					DTG_LOGE("cannot clear dtg buffer.");
				} else {
					backup_data_bit = 0x0;
					sent_group = 0;
				}
			}
			time(&time_out);
			d_time = difftime(time_out, time_ent);
			if ((get_dtg_report_period() - (int)d_time) > 0)
				alarm(get_dtg_report_period() - (int)d_time);
			else
				alarm(1);
		} else {
			DTG_LOGE("dtg unread record call error.");
			alarm(1);
		}
		unlock_mutex();
	}
	DTG_LOGD("%s: %s() --", __FILE__, __func__);
}

int main_process()
{
	pthread_t p_thread_mdt;
	pthread_t p_thread_dtg;
	int status;
	struct sigaction act;
	char *phonenum = NULL;

	act.sa_handler = dtg_alarm_callback;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, 0);

	DTG_LOGT("TACOC DAISHIN MODEL!!");
	
	init_configuration_data();

	send_device_registration();

	if (pthread_create(&p_thread_mdt, NULL, do_ctrl_report_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}
	sleep(5);

	if (pthread_create(&p_thread_dtg, NULL, do_dtg_action_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	alarm(1);

	pthread_join(p_thread_dtg, (void **) status);
	DTG_LOGT("TACOC TRIPOS MODEL Exit");
	return 0;
}

