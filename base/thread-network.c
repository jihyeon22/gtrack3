#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <time.h>

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

#include <base/thread.h>
#include <at/at_util.h>
#include <mdsapi/mds_api.h>
#include <board/modem-time.h>
#include <util/poweroff.h>
#include <callback.h>

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#include <kt_fota.h>
#include <kt_fota_config.h>
#endif

#if defined (BOARD_TL500S) && defined (USE_TL500S_FOTA)
#include <tl500s_fota_proc.h>
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
static int max_network_fail_reset_cnt = DEFAULT_NETWORK_FAIL_RESET_CNT_1;
static int saved_network_fail_cnt = 0;
static int first_network_conn_flag = 0;

// network chk api..
#define CHK_REGI_RET_OK     1
#define CHK_REGI_RET_NOK    -1
static int chk_network_regi(int chk_interval);
static int get_net_conn_fail_cnt();
static int set_net_conn_fail_cnt(int cnt);

void *thread_network(void *args)
{
	int ready = 0;
	int network_fail_cnt = 0;
	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if(!flag_run_thread_network)
	{
		return NULL;
	}

//#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#if defined (KT_FOTA_ENABLE)
	load_ini_kt_fota_svc_info();
#endif

    saved_network_fail_cnt = get_net_conn_fail_cnt();

	while(flag_run_thread_network)
	{
		if ( ( nettool_get_state() == DEFINES_MDS_OK ) && ( chk_network_regi(1) == CHK_REGI_RET_OK ) )
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
#if defined (BOARD_TL500S) && defined (USE_TL500S_FOTA)
			tl500s_fota_proc();
#endif
            if ( saved_network_fail_cnt > 0 )
            {
                 devel_webdm_send_log("network fail reboot cnt : %d ", saved_network_fail_cnt);
                 set_net_conn_fail_cnt(0);
                 saved_network_fail_cnt = 0;
            }
            
#if defined (RDATE_TIME_SYNC_ENABLE)
            if ( mds_api_set_network_time(KOR_TIME_ZONE_OFFSET) == DEFINES_MDS_API_NOK ) 
                devel_webdm_send_log("rdate time sync fail ");
#endif
			network_on_callback();
			break;
		}
		else
		{
            network_fail_cnt++;

            if ( max_network_fail_reset_cnt > 0 ) 
            {
                LOGE(LOG_TARGET, "NETWORK CHK FAIL [%d][%d] -> SAVED FAIL CNT [%d]\n",network_fail_cnt, max_network_fail_reset_cnt, saved_network_fail_cnt);
                // 8번까지는 네트워크 안되면 빠르게 재부팅한다.
                if ( saved_network_fail_cnt < 8 )
                {
                    LOGE(LOG_TARGET, "   =>  network fail case 1 : SAVED FAIL CNT [%d]\n",saved_network_fail_cnt);
                    max_network_fail_reset_cnt = DEFAULT_NETWORK_FAIL_RESET_CNT_2;
                }

// No use network senario need not reset.
#if defined (KT_FOTA_TEST_SVR) || defined (NO_USE_NETWORK)
                // do nothing
                LOGE(LOG_TARGET, "NO_USE_NETWORK DO NOTHING.. \r\n");
#else
                if ( network_fail_cnt > max_network_fail_reset_cnt )
                {
                    LOGE(LOG_TARGET, "NET FAIL DO RESET!!!!.. \r\n");
                    saved_network_fail_cnt += 1;
                    set_net_conn_fail_cnt(saved_network_fail_cnt);
                    network_fail_emergency_reset_callback();
                    poweroff(NULL,0);
                }
#endif

            }

		}
		LOGT(LOG_TARGET, "Waiting for available network.\n");
		sleep(1);
	}

    warn_timeout_prev_send_time = tools_get_kerneltime();
    
    // first network connect
    first_network_conn_flag = 1;

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

#if defined (KT_FOTA_TEST_SVR) || defined (NO_USE_NETWORK)
        // donoting..
        LOGE(LOG_TARGET, "NO_USE_NETWORK DO NOTHING.. \r\n");
#else
        chk_runtime_network_chk();
#endif

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


void set_max_network_fail_reset_cnt(int cnt)
{
	max_network_fail_reset_cnt = cnt;
}


int get_net_conn_fail_cnt()
{
    char read_buff[128] = {0,};
    int read_cnt = 0;

    int ret_val = 0;

    read_cnt = mds_api_read_data(NET_CONN_FAIL_CNT_PATH, (void*)read_buff, sizeof(read_buff));
    if ( read_cnt > 0 )
        ret_val = atoi(read_buff);

    LOGT(eSVC_COMMON,"get regi fail cnt [%d]\r\n", ret_val);

    return ret_val;
}

int set_net_conn_fail_cnt(int cnt)
{
    char write_buff[128] = {0,};
    int write_cnt = 0;

    write_cnt = sprintf(write_buff,"%d", cnt);

    mds_api_write_data(NET_CONN_FAIL_CNT_PATH, (void*)write_buff, write_cnt, 0);

    LOGT(eSVC_COMMON,"set regi fail cnt [%d]\r\n", cnt);

    return cnt;
}

int chk_network_regi(int chk_interval)
{
    AT_RET_NET_STAT netstat = 0;
    int ret_val = CHK_REGI_RET_NOK;
    static int run_cnt = 0;

    // 너무 자주하면, 문제생길까 싶다.
    if (( run_cnt++ % chk_interval ) != 0)
    {
        LOGT(eSVC_COMMON,"wait_regi_init() : skip [%d][%d]\r\n", run_cnt, chk_interval);
        return CHK_REGI_RET_OK;
    }

    if ( at_get_netstat(&netstat) != AT_RET_SUCCESS )
    {
        LOGE(eSVC_COMMON, "wait_regi_init() : at_get_netstat err \n");
        ret_val = CHK_REGI_RET_NOK;
    }

    if ( (netstat == AT_RET_NET_REGI_SUCCESS) || (netstat == AT_RET_NET_REGI_FAIL_ROAMING) )
    {
        LOGT(eSVC_COMMON,"wait_regi_init() : netstat is OK\r\n");
        ret_val = CHK_REGI_RET_OK;
    }
    else
    {
        LOGE(eSVC_COMMON,"wait_regi_init() : netstat is not OK\r\n");
        ret_val = CHK_REGI_RET_NOK;
    }

    return ret_val;

}

int chk_runtime_network_chk()
{
    static int network_fail_cnt = 0;

    static time_t last_cycle = 0;
    time_t cur_time = 0;

	struct timeval tv;
	struct tm ttm;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);
    cur_time = mktime(&ttm);

    int fail_reset_count = DEFAULT_NETWORK_FAIL_RESET_CNT_3;

    // 런타임 체크루틴이기 때문에... 네트워크 접속할때까지 ...기다림
    if ( first_network_conn_flag == 0 )
    {
        network_fail_cnt = 0;
        LOGE(LOG_TARGET, "RUNTIME NETCHK : WAIT FOR FIRST NETWORK CONN \n");
        return 0;
    }

    // 30 sec interval network chk
    // 네트워크 쓰레드의 경우 불리는 주기가 불규칙, 때문에 시간계산하여 30초마다 한번씩 불리도록
    {
        if(cur_time == 0)
        {
            return 0;
        }

        if(last_cycle == 0)
        {
            last_cycle = cur_time;
            return 0;
        }

        if ( cur_time < last_cycle )
        {
            last_cycle = cur_time;
            return 0;
        }

        // 네트워크 timeout 값이 30 이기 때문에 25로 넉넉히 설정
        if( (cur_time - last_cycle) < 30 )
        {
            return 0;
        }

        last_cycle = cur_time;
    }

    // network chk...
    if ( ( nettool_get_state() == DEFINES_MDS_OK ) && ( chk_network_regi(1) == 1 ) )
    {
        network_fail_cnt = 0;
        LOGI(LOG_TARGET, "RUNTIME NETCHK : NETWORK CONN OK\n");

    // 모뎀이 이상한 시간을 세팅하는경우가 있때에 대비
#if defined (RDATE_TIME_SYNC_ENABLE)
        if ( mds_api_set_network_time(KOR_TIME_ZONE_OFFSET) == DEFINES_MDS_API_NOK ) 
            devel_webdm_send_log("rdate time sync fail ");
#endif
        return 0;
    }

    // network stat invalid
    network_fail_cnt++;
    LOGE(LOG_TARGET, "RUNTIME NETCHK : NETWORK CONN NOK\n");

    if ( max_network_fail_reset_cnt > 0 ) 
    {
        LOGE(LOG_TARGET, "RUNTIME NETCHK : [%d][%d] -> SAVED FAIL CNT [%d]\n",network_fail_cnt, fail_reset_count, saved_network_fail_cnt);

        if ( network_fail_cnt >= fail_reset_count )
        {
            saved_network_fail_cnt += 1;
            set_net_conn_fail_cnt(saved_network_fail_cnt);
            network_fail_emergency_reset_callback();
            poweroff(NULL,0);
        }
    }

    return 0;
}