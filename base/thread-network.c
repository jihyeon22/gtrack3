#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <base/config.h>
#include <at/at_util.h>
#include <base/sender.h>
#include <base/devel.h>
#include <base/error.h>
#include <base/dmmgr.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#endif
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/pipe.h>
#include <util/nettool.h>
#include <include/defines.h>
#include <logd_rpc.h>
#include <callback.h>

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#include <kt_fota.h>
#include <kt_fota_config.h>
#endif

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#define PIPE1_DEV_NET_TIMEOUT_SEC		10
#else
#define PIPE1_DEV_NET_TIMEOUT_SEC		30
#endif
#define PIPE2_DEV_NET_TIMEOUT_SEC		180
#define WARN_PIPE_ERROR_TIME			(1*3600) //1 hour

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK


static int flag_run_thread_network = 1;
static int flag_run_thread_network2 = 1;
static int warn_timeout_sec_thread_network = WARN_PIPE_ERROR_TIME;
static time_t warn_timeout_prev_send_time = 0;


void *thread_network(void *args)
{
	int ready = 0;

	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if(!flag_run_thread_network)
	{
		return NULL;
	}

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
	load_ini_kt_fota_svc_info();
#endif

	while(flag_run_thread_network)
	{
		if(nettool_get_state() == DEFINES_MDS_OK)
		{
			// 바로 시도하니까 네트워크가 안된다.
			sleep(2);
			configurationBase_t *conf_base = get_config_base();
#ifdef USE_GPS_MODEL
			gps_start_utc_adjust();
#endif
			// REQ : First run MDS FOTA and second run KT FOTA..
			dmmgr_send_incomplete_event();
			if(dmmgr_sended_key_on() == 0)
			{
				dmmgr_send(eEVENT_UPDATE, NULL, 0);
			}

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
			kt_fota_init();
#endif

			if(conf_base->common.bootstrap==1)
			{
				save_config("common:bootstrap", "0");
			}
			
			// at_channel_recovery();
			
			network_on_callback();
			break;
		}
		LOGT(LOG_TARGET, "Waiting for available network.\n");
		sleep(1);
	}

	warn_timeout_prev_send_time = tools_get_kerneltime();

	// btn thread 와 sync 로 인해 못보냈다면 다시 보내도록.

	while(flag_run_thread_network) {
		int nfds = 0;
		fd_set readfds;

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
		if(kt_fota_check_cycle() == 1)
		{
			kt_fota_send();
		}
#endif

		watchdog_set_cur_ktime(eWdNet1);
		watchdog_process();

		wd_dbg[eWdNet1] = 1;
		dmmgr_alive_send();

		wd_dbg[eWdNet1] = 2;
		sender_set_network_fds(ePIPE_1, &nfds, &readfds);

		wd_dbg[eWdNet1] = 3;		
		ready = pipe_wait_read_signal_sec(nfds, &readfds, PIPE1_DEV_NET_TIMEOUT_SEC);
		LOGT(LOG_TARGET, "ePIPE_1 pipe select : [%d] num : [%d]\n", ready, sender_get_num_remaindata(ePIPE_1));
		wd_dbg[eWdNet1] = 4;

		if(ready < 0) {
			wd_dbg[eWdNet1] = 5;
			if(errno != EINTR)
			{
				LOGE(LOG_TARGET, "[network Thread] ePIPE_1 : select error [%d] : %s\n", errno, strerror(errno));
				error_critical(eERROR_EXIT, "thread_net select Error : ePIPE_1");
			}
			else 
			{
				error_critical(eERROR_LOG, "thread_net select ePIPE_1 : select error EINTR");
			}
			continue;
		}
		
		if(ready > 0)
		{
			wd_dbg[eWdNet1] = 6;
			warn_timeout_prev_send_time = tools_get_kerneltime();
			
			if(sender_network_process(ePIPE_1, &readfds) < 0)
			{
				continue;
			}
		}
		else
		{
			wd_dbg[eWdNet1] = 7;
			time_t cur_ktime = 0;
			gNetworkTriger_empty = 1;

			if(warn_timeout_sec_thread_network == 0)
			{
				continue;
			}

			cur_ktime = tools_get_kerneltime();
			if(tools_get_kerneltime() == 0)
			{
				continue;
			}
			
			if(cur_ktime - warn_timeout_prev_send_time >= warn_timeout_sec_thread_network)
			{
				devel_webdm_send_log("Warn : No network for %u sec. ", warn_timeout_sec_thread_network);
				warn_timeout_prev_send_time = cur_ktime;
			}
		}
		wd_dbg[eWdNet1] = 8;
	}

	return NULL;
}

#ifdef USE_NET_THREAD2
void *thread_network2(void *args)
{
	int ready = 0;

	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if(!flag_run_thread_network2)
	{
		return NULL;
	}

	while(flag_run_thread_network2)
	{
		if(nettool_get_state() == DEFINES_MDS_OK)
		{
			break;
		}
		LOGT(LOG_TARGET, "Waiting for available network2.\n");
		sleep(1);
	}

	while(flag_run_thread_network2) {
		int nfds = 0;
		fd_set readfds;

		watchdog_set_cur_ktime(eWdNet2);
		watchdog_process();
		
		sender_set_network_fds(ePIPE_2, &nfds, &readfds);
		
		ready = pipe_wait_read_signal_sec(nfds, &readfds, PIPE2_DEV_NET_TIMEOUT_SEC);
		LOGT(LOG_TARGET, "ePIPE_2 pipe select : [%d] num : [%d]\n", ready, sender_get_num_remaindata(ePIPE_2));

		if(ready < 0) {
			if(errno != EINTR)
			{
				LOGE(LOG_TARGET, "[network Thread] ePIPE_2 : select error [%d] : %s\n", errno, strerror(errno));
				error_critical(eERROR_EXIT, "thread_net select Error : ePIPE_2");
			}
			continue;
		}
		
		if(ready > 0)
		{
			if(sender_network_process(ePIPE_2, &readfds) < 0)
			{
				continue;
			}
		}
		else
		{
			gNetworkTriger2_empty = 1;
		}
	}

	return NULL;
}
#endif

void exit_thread_network(void)
{
	flag_run_thread_network = 0;
}

#ifdef USE_NET_THREAD2
void exit_thread_network2(void)
{
	flag_run_thread_network2 = 0;
}
#endif

void thread_network_set_warn_timeout(int time)
{
	if(time == 0)
	{
		warn_timeout_sec_thread_network = 0;
	}
	else if(time < 60)
	{
		warn_timeout_sec_thread_network = 60;
	}
	else
	{
		warn_timeout_sec_thread_network = time;
	}
}

