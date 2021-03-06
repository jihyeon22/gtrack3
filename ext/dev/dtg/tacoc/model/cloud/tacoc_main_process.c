<<<<<<< HEAD
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <wrapper/dtg_atcmd.h>

#include <wrapper/dtg_log.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"

void dtg_alarm_callback(int signo)
{
	DTG_LOGD("<dtg_alarm_callback> : g_dtg_config.action_working[%d]\n", is_working_action());

	if(is_working_action())
		return;

	set_signal();
}

void *do_dtg_action_thread()
{
	int ret = -1;
	int result = -1;
	DTG_LOGD("do_dtg_action_thread ++\n");
	while(1)
	{
		lock_mutex();
		DTG_LOGD("action thread wait ++\n");
		wait_for_singleobject();
		DTG_LOGD("action thread received\n");

		ret = data_req_to_taco();
		if (!ret) {
			DTG_LOGD("Data Request Error\n");
		}

		alarm(get_interval());
		unlock_mutex();
	}
	DTG_LOGD("do_dtg_action_thread --\n");
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

	DTG_LOGT("TACOC CLOUD MODEL!!");
	
	init_configuration_data();
	
	if (pthread_create(&p_thread_action, NULL, do_dtg_action_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	while (1) {
		if ((phonenum = atcmd_get_phonenum()) == NULL) {
			DTG_LOGE("TACOC CLOUD GET PHONE NUMBER FAILURE");
			sleep(1);
		}
		else
			DTG_LOGD("TACOC CLOUD PHONE NUMBER : %s", phonenum);
			break;
	}
	
	send_device_registration();

	alarm( 1 );

	DTG_LOGD("set dtg alarm signal [%d]sec: ok\n", get_interval());
	pthread_join(p_thread_action, (void **) status);
	DTG_LOGD("DTG_APPLICATION Exit\n");
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
#include <assert.h>

#include <wrapper/dtg_atcmd.h>

#include <wrapper/dtg_log.h>
#include "dtg_type.h"
#include "dtg_data_manage.h"
#include "dtg_ini_utill.h"
#include "rpc_clnt_operation.h"

void dtg_alarm_callback(int signo)
{
	DTG_LOGD("<dtg_alarm_callback> : g_dtg_config.action_working[%d]\n", is_working_action());

	if(is_working_action())
		return;

	set_signal();
}

void *do_dtg_action_thread()
{
	int ret = -1;
	int result = -1;
	DTG_LOGD("do_dtg_action_thread ++\n");
	while(1)
	{
		lock_mutex();
		DTG_LOGD("action thread wait ++\n");
		wait_for_singleobject();
		DTG_LOGD("action thread received\n");

		ret = data_req_to_taco();
		if (!ret) {
			DTG_LOGD("Data Request Error\n");
		}

		alarm(get_interval());
		unlock_mutex();
	}
	DTG_LOGD("do_dtg_action_thread --\n");
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

	DTG_LOGT("TACOC CLOUD MODEL!!");
	
	init_configuration_data();
	
	if (pthread_create(&p_thread_action, NULL, do_dtg_action_thread, NULL) < 0) {
		fprintf(stderr, "cannot create p_thread_action thread\n");
		exit(1);
	}

	while (1) {
		if ((phonenum = atcmd_get_phonenum()) == NULL) {
			DTG_LOGE("TACOC CLOUD GET PHONE NUMBER FAILURE");
			sleep(1);
		}
		else
			DTG_LOGD("TACOC CLOUD PHONE NUMBER : %s", phonenum);
			break;
	}
	
	send_device_registration();

	alarm( 1 );

	DTG_LOGD("set dtg alarm signal [%d]sec: ok\n", get_interval());
	pthread_join(p_thread_action, (void **) status);
	DTG_LOGD("DTG_APPLICATION Exit\n");
	return 0;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
