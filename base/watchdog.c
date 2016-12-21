#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <base/watchdog.h>
#include <base/devel.h>
#include <base/error.h>
#include <base/sender.h>
#include <board/power.h>
#include <util/poweroff.h>
#include <util/tools.h>
#include <logd_rpc.h>

#include <callback.h>


// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE

void _deinit_essential_functions(void);

static char *strWdType[eWdLast] =
{
	"Net1",
#ifdef USE_NET_THREAD2
	"Net2",
#endif

#ifdef USE_BUTTON_THREAD
	"Pwr",
#endif

	"Main",
#ifdef USE_GPS_MODEL
	"Gps",
#endif
};

static time_t ktime_wd[eWdLast];
static unsigned int watchdog_time[eWdLast] = 
{
#if 1
	WATCHDOG_DEFAULT_SEC, //eWdNet1
#ifdef USE_NET_THREAD2
	WATCHDOG_DEFAULT_SEC, //eWdNet2
#endif
#ifdef USE_BUTTON_THREAD
	WATCHDOG_DEFAULT_SEC, //eWdPwr
#endif
	WATCHDOG_DEFAULT_SEC, //eWdMain
#ifdef USE_GPS_MODEL
	WATCHDOG_DEFAULT_SEC, //eWdGps
#endif
#else
	20,
	20,
	20,
	20,
	20
#endif
};
static int num_watchdog[eWdLast];
static int flag_wd_mutex = 0;

int wd_dbg[eWdLast];

int watchdog_set_cur_ktime(eWdType_t wd_type)
{
	time_t ktime_cur = tools_get_kerneltime();
	
	if(ktime_cur <= 0)
	{
		static int flag_send_log = 0;
				
		LOGE(eSVC_BASE, "%s> fail to get_ktime.\n", __FUNCTION__); 
		
		if(flag_send_log == 0)
		{
			devel_webdm_send_log("%s> fail to get_ktime. %d", __FUNCTION__, ktime_cur);
			flag_send_log = 1;
		}
		
		return -1;
	}

	//LOGI(eSVC_BASE, "%s> type[%d] %u\n", __FUNCTION__, wd_type, ktime_cur);

	
	ktime_wd[wd_type] = ktime_cur;
	
	return 0;
}

int watchdog_set_init_ktime(eWdType_t wd_type)
{
	ktime_wd[wd_type] = 0;
	
	return 0;
}

int watchdog_set_time(eWdType_t wd_type, unsigned int secs)
{
	watchdog_time[wd_type] = secs;

	LOGI(eSVC_BASE, "%s> pipe_time [%d]%u\n", __FUNCTION__, wd_type, watchdog_time[wd_type]);
	
	return 0;
}

void watchdog_process(void)
{
	int i;
	static time_t ktime_prev_run = 0;
	static time_t ktime_prev_log = 0;

	time_t ktime_cur = tools_get_kerneltime();

	if(ktime_cur <= 0)
	{
		LOGE(eSVC_BASE, "%s> fail to get_ktime. %d\n", __FUNCTION__, ktime_cur);
	
		return;
	}

	if(ktime_cur - ktime_prev_run <= 3)
	{
		return;
	}
	ktime_prev_run = ktime_cur;

	for(i=0; i<eWdLast; i++)
	{
		if(ktime_wd[i] == 0)
		{
			LOGT(eSVC_BASE, "%s> Watchdog[%d] is not started yet.\n", __FUNCTION__, i);
		
			continue;
		}

		if(ktime_cur - ktime_wd[i] >= watchdog_time[i])
		{
			num_watchdog[i]++;
		}
		else
		{
			num_watchdog[i] = 0;
		}

		if(num_watchdog[i] < 3)
		{
			continue;
		}

		if(flag_wd_mutex == 0)
		{
			char str_log[512] = {0};
			char *pTmp = str_log;
			int remain = sizeof(str_log) - 1;
			int i;
		
			flag_wd_mutex = 1;
			
			LOGI(eSVC_BASE, "%s> run watchdog.\n", __FUNCTION__);
		
			terminate_app_callback();
			_deinit_essential_functions();

			pTmp += snprintf(pTmp, remain, "%s> ", __FUNCTION__);
			remain -= pTmp - str_log;

			for(i=0; i<eWdLast; i++)
			{
				if(remain < 0)
				{
					remain = 0;
					continue;
				}

				pTmp += snprintf(pTmp, remain, "%s:%ld %d[%d][%d],",
					strWdType[i], !ktime_wd[i]?0:ktime_cur - ktime_wd[i], watchdog_time[i], num_watchdog[i], wd_dbg[i]);
				remain -= pTmp - str_log;
			}

			if(remain > 0)
			{
#ifdef USE_NET_THREAD2
				pTmp += snprintf(pTmp, remain, "P:%d %d/%d %d", sender_get_num_remaindata(ePIPE_1), sender_get_size_remaindata(ePIPE_1),
					sender_get_num_remaindata(ePIPE_2), sender_get_size_remaindata(ePIPE_2));
#else
				pTmp += snprintf(pTmp, remain, "P:%d %d", sender_get_num_remaindata(ePIPE_1), sender_get_size_remaindata(ePIPE_1));
#endif
				remain -= pTmp - str_log; //no use
			}

			error_critical(eERROR_LOG, "%s", str_log);
			devel_webdm_send_log("%s", str_log);

/*
			error_critical(eERROR_LOG, "%s> %d %d %d %d, %d %d %d, %d/%d",
				__FUNCTION__, !ktime_wd[eWdNet1]?0:ktime_cur - ktime_wd[eWdNet1], watchdog_time[eWdNet1], num_watchdog[eWdNet1],sender_get_num_remaindata(ePIPE_1),
				!ktime_wd[eWdPwr]?0:ktime_cur - ktime_wd[eWdPwr], watchdog_time[eWdPwr], num_watchdog[eWdPwr],
				wd_dbg[eWdNet1], wd_dbg[eWdPwr]);
			devel_webdm_send_log("%s> %d %d %d %d, %d %d %d, %d/%d",
				__FUNCTION__, !ktime_wd[eWdNet1]?0:ktime_cur - ktime_wd[eWdNet1], watchdog_time[eWdNet1], num_watchdog[eWdNet1],sender_get_num_remaindata(ePIPE_1),
				!ktime_wd[eWdPwr]?0:ktime_cur - ktime_wd[eWdPwr], watchdog_time[eWdPwr], num_watchdog[eWdPwr],
				wd_dbg[eWdNet1], wd_dbg[eWdPwr]);
*/
			poweroff(__FUNCTION__, sizeof(__FUNCTION__));
		}
	}

	if(ktime_prev_log == 0 || ktime_cur - ktime_prev_log > 10)
	{
		ktime_prev_log = ktime_cur;
		/*
		LOGI(eSVC_BASE, "%s> check. net1:%d>%u %d,net2:%d>%u %d,pwr:%d>%d %d,main:%d>%d %d,Gps:%d>%d %d\n",
			__FUNCTION__,
			!ktime_wd[eWdNet1]?0:ktime_cur - ktime_wd[eWdNet1], watchdog_time[eWdNet1],num_watchdog[eWdNet1],
			!ktime_wd[eWdNet2]?0:ktime_cur - ktime_wd[eWdNet2], watchdog_time[eWdNet2],num_watchdog[eWdNet2],
			!ktime_wd[eWdPwr]?0:ktime_cur - ktime_wd[eWdPwr], watchdog_time[eWdPwr],num_watchdog[eWdPwr],
			!ktime_wd[eWdMain]?0:ktime_cur - ktime_wd[eWdMain], watchdog_time[eWdMain],num_watchdog[eWdMain],
			!ktime_wd[eWdGps]?0:ktime_cur - ktime_wd[eWdGps], watchdog_time[eWdGps],num_watchdog[eWdGps]
		);
		*/
		LOGI(eSVC_BASE, "%s> check net1:%d>%u %d\n", __FUNCTION__, !ktime_wd[eWdNet1]?0:ktime_cur - ktime_wd[eWdNet1], watchdog_time[eWdNet1],num_watchdog[eWdNet1]);
#ifdef USE_NET_THREAD2
		LOGI(eSVC_BASE, "%s> check net2:%d>%u %d\n", __FUNCTION__, !ktime_wd[eWdNet2]?0:ktime_cur - ktime_wd[eWdNet2], watchdog_time[eWdNet2],num_watchdog[eWdNet2]);
#endif
#ifdef USE_BUTTON_THREAD
		LOGI(eSVC_BASE, "%s> check pwr:%d>%u %d\n", __FUNCTION__,  !ktime_wd[eWdPwr]?0:ktime_cur - ktime_wd[eWdPwr], watchdog_time[eWdPwr],num_watchdog[eWdPwr]);
#endif
		LOGI(eSVC_BASE, "%s> check main:%d>%u %d\n", __FUNCTION__, !ktime_wd[eWdMain]?0:ktime_cur - ktime_wd[eWdMain], watchdog_time[eWdMain],num_watchdog[eWdMain]); 
#ifdef USE_GPS_MODEL
		LOGI(eSVC_BASE, "%s> check Gps:%d>%u %d\n", __FUNCTION__,  !ktime_wd[eWdGps]?0:ktime_cur - ktime_wd[eWdGps], watchdog_time[eWdGps],num_watchdog[eWdGps]);
#endif
	}
}


void get_watchdog_status()
{
	char str_log[1024];
	char tmp[128];
	memset(str_log, 0x00, sizeof(str_log));

	time_t ktime_cur = tools_get_kerneltime();
	if(ktime_cur <= 0)
	{
		LOGE(eSVC_BASE, "%s> fail to get_ktime. %d\n", __FUNCTION__, ktime_cur);
		return;
	}

	sprintf(tmp, "[net1:%d, %u, %d] ", !ktime_wd[eWdNet1]?0:ktime_cur - ktime_wd[eWdNet1], watchdog_time[eWdNet1],num_watchdog[eWdNet1]);
	strcpy(str_log, tmp);
#ifdef USE_NET_THREAD2
	sprintf(tmp, "[net2:%d, %u, %d] ", !ktime_wd[eWdNet2]?0:ktime_cur - ktime_wd[eWdNet2], watchdog_time[eWdNet2],num_watchdog[eWdNet2]);
	strcat(str_log, tmp);
#endif
#ifdef USE_BUTTON_THREAD
	sprintf(tmp, "[pwr:%d, %u, %d] ", !ktime_wd[eWdPwr]?0:ktime_cur - ktime_wd[eWdPwr], watchdog_time[eWdPwr],num_watchdog[eWdPwr]);
	strcat(str_log, tmp);
#endif
	sprintf(tmp, "[main:%d, %u, %d] ", !ktime_wd[eWdMain]?0:ktime_cur - ktime_wd[eWdMain], watchdog_time[eWdMain],num_watchdog[eWdMain]); 
	strcat(str_log, tmp);
#ifdef USE_GPS_MODEL
	sprintf(tmp, "[Gps:%d, %u, %d] ", !ktime_wd[eWdGps]?0:ktime_cur - ktime_wd[eWdGps], watchdog_time[eWdGps],num_watchdog[eWdGps]);
	strcat(str_log, tmp);
#endif
	devel_webdm_send_log("%s", str_log);
}
