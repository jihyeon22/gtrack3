#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/mileage.h>
#include <base/devel.h>
#include <base/sender.h>
#include <base/watchdog.h>
#include <board/battery.h>
#include <board/modem-time.h>
#include <base/thread.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <util/stackdump.h>
#include "logd/logd_rpc.h"

#include <netcom.h>
#include <callback.h>
#include <report.h>
#include <config.h>
#include <data-list.h>

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static int _process_poweroff(char *log);

static int gps_first_fix = 0;
static int have_sended_pwr_on_msg = 0;
static int key_on = -1;
time_t ktime_reported = 0;
static time_t prev_gps_active_time = 0;

static int flag_run_thread_main = 1;

static char *_get_ps(void);
static void _check_mem(void);

void abort_callback(void)
{
	FILE *log_fd;
	char *outdata = NULL;
	
	outdata = _get_ps();
	if(outdata != NULL)
	{
		log_fd = fopen(CALLSTACK_LOG_PATH, "a");

		if(log_fd != NULL) {
			fprintf(log_fd, "%s", outdata);
			fprintf(log_fd, "\n\n");
			fclose(log_fd);
		}
	}	
}

void init_model_callback(void)
{
	thread_network_set_warn_timeout(0);
	stackdump_abort_callback = abort_callback;
	setting_network_param();
}

void network_on_callback(void)
{
	char smsmsg[100] = {0,};
	sprintf(smsmsg, "start %u %u %d\n", mileage_get_m(), (unsigned int)tools_get_kerneltime(), key_on);
	devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	devel_webdm_send_log(smsmsg);
}

void button1_callback(void)
{
	LOGT(LOG_TARGET, "button1 callback\n");
#if 1
	//if(gps_first_fix == 1)
	{
		int powerType = 0;
		
		if(key_on == 1)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
		sender_add_data_to_buffer(REPORT_ARRIVAL_EVENT, &powerType, ePIPE_2);
	}
#endif	
	
}

void button2_callback(void)
{
	LOGT(LOG_TARGET, "button2 callback\n");
#if 1
	//if(gps_first_fix == 1)
	{
		int powerType = 0;
		
		if(key_on == 1)
		{
			powerType = 0;
		}
		else
		{
			powerType = 1;
		}
		
		sender_add_data_to_buffer(REPORT_WAITING_EVENT, &powerType, ePIPE_2);
	}
#endif
}

void ignition_on_callback(void)
{
}

void ignition_off_callback(void)
{
}

void power_on_callback(void)
{
	if(gps_first_fix == 1)
	{
		int powerType = 0;
		configurationModel_t *conf = get_config_model();
		
		have_sended_pwr_on_msg = 1;
		thread_network_set_warn_timeout(conf->model.report_interval_keyon * 2);
		sender_add_data_to_buffer(REPORT_TURNON_EVENT, &powerType, ePIPE_1);
		sender_add_data_to_buffer(REPORT_POWER_EVENT, &powerType, ePIPE_1);
	}
	
	key_on = 1;
}

void power_off_callback(void)
{
	thread_network_set_warn_timeout(41*60); //40 minutes

	if(have_sended_pwr_on_msg == 1)
	{
		int powerType = 1;
		
		sender_add_data_to_buffer(REPORT_TURNOFF_EVENT, &powerType, ePIPE_1);
		sender_add_data_to_buffer(REPORT_POWER_EVENT, &powerType, ePIPE_1);
		have_sended_pwr_on_msg = 0;
	}
	
	key_on = 0;
}

void gps_parse_one_context_callback(void)
{
	configurationModel_t *conf = get_config_model();
	gpsData_t gpsdata;
	gps_get_curr_data(&gpsdata);

	static int cycle = 0;
	static unsigned int report_cycle = 0;

	if(gpsdata.active == 1 && gpsdata.speed > 0)
	{
		prev_gps_active_time = tools_get_kerneltime();
	}

	if(gps_first_fix == 0)
	{
		// When receive valid gps for the first time.
		if(gpsdata.active == 1)
		{
			thread_network_set_warn_timeout(conf->model.report_interval_keyon * 2);
			mileage_process(&gpsdata);
			if(key_on == 1)
			{
				int powerType = 0;
				sender_add_data_to_buffer(REPORT_TURNON_EVENT, &powerType, ePIPE_1);
				sender_add_data_to_buffer(REPORT_POWER_EVENT, &powerType, ePIPE_1);
				have_sended_pwr_on_msg = 1;
			}
			gps_first_fix = 1;
		}
	}
	else
	{
		if(key_on <= 0){
			if( report_cycle > 0)
			{
				report_cycle = 0;
				ktime_reported = tools_get_kerneltime();
				
				if(sender_add_data_to_buffer(REPORT_PERIOD_EVENT, NULL, ePIPE_1) < 0)
				{
					LOGE(LOG_TARGET, "make PERIOD PACKET fail");
				}
			}
			
			ktime_reported = 0;
			return;
		}

		mileage_process(&gpsdata);
		
		LOGT(LOG_TARGET, "%s : Send interval [%d]/[%d]\n", __FUNCTION__, cycle, conf->model.collect_interval_keyon);

		if(++cycle >= conf->model.collect_interval_keyon)
		{
			periodData_t *data;
			unsigned char *encbuf;
			unsigned short enclen;

			cycle = 0;

			if(make_partial_period_packet(&encbuf, &enclen, &gpsdata) >= 0)
			{
				data = malloc(sizeof(periodData_t));
				if(data == NULL)
				{
					LOGE(LOG_TARGET, "malloc fail\n");
					free(encbuf);
					return;
				}
				data->encbuf = encbuf;
				data->enclen = enclen;

				if(list_add(&packet_list, data) < 0)
				{
					LOGE(LOG_TARGET, "%s : list add fail\n", __FUNCTION__);
					free(encbuf);
					free(data);
				}
			}
			report_cycle++;
		}

		if(conf->model.collect_interval_keyon <= 0)
		{
			return;
		}
		
		if(report_cycle >= conf->model.report_interval_keyon/conf->model.collect_interval_keyon)
		{
			report_cycle = 0;
			ktime_reported = tools_get_kerneltime();
			
			if(sender_add_data_to_buffer(REPORT_PERIOD_EVENT, NULL, ePIPE_1) < 0)
			{
				LOGE(LOG_TARGET, "make PERIOD PACKET fail");
			}
		}
	}
}

#define MAIN_LOOP_CYCLE_SECS 3
#define POWEROFF_STANDBY_SECS 7200
void main_loop_callback(void)
{
	time_t ktime_now = 0;
	time_t ktime_key_off = 0;
	time_t system_on_time = tools_get_kerneltime();

	if(!flag_run_thread_main)
	{
		return;
	}

	while(flag_run_thread_main)
	{
		configurationModel_t * conf = get_config_model();
		int batt = 0;
		
		_check_mem();
		watchdog_process();
		watchdog_set_cur_ktime(eWdMain);
		
		ktime_now = tools_get_kerneltime();
		
		if(key_on == 0)
		{
			if(ktime_key_off == 0)
			{
				ktime_key_off = ktime_now;
			}

			batt = battery_get_battlevel_internal();

			LOGI(LOG_TARGET, "Check power-on time %d > %d, batt %d\n", ktime_now - ktime_key_off, POWEROFF_STANDBY_SECS, batt);
			
			if((ktime_now - ktime_key_off > POWEROFF_STANDBY_SECS) ||
				(batt > 0 && batt < 3700))
			{
				LOGT(LOG_TARGET, "batt internal %d mv\n", batt);
				_process_poweroff("main_loop_callback:poweroff");
			}

			sleep(MAIN_LOOP_CYCLE_SECS);
			continue;
		}
		
		ktime_key_off = 0;

		if(key_on == 0 || ktime_now - prev_gps_active_time >= 1800)
		{
			if(ktime_now - system_on_time > (24 * 3600) ) 
			{
				devel_webdm_send_log("regular poweroff");
				_process_poweroff("main_loop_callback:regular poweroff");
			}
			sleep(MAIN_LOOP_CYCLE_SECS);
			continue;
		}
	
		if(gps_first_fix == 0)
		{
			sleep(MAIN_LOOP_CYCLE_SECS);
			continue;
		}

		if(ktime_reported > 0 && ktime_now - ktime_reported >= (conf->model.report_interval_keyon *5) )
		{
			static int flag_send_log_no_report = 0;
			
			if(flag_send_log_no_report == 0)
			{
				devel_webdm_send_log("Warn : No report packet. (Key Status:%d) ", key_on);
				flag_send_log_no_report = 1;
			}
		}

		sleep(MAIN_LOOP_CYCLE_SECS);
	}
}

void terminate_app_callback(void)
{
}

void exit_main_loop(void)
{
	flag_run_thread_main = 0;
}

static int _process_poweroff(char *log)
{
	char smsmsg[140] = {0,};

	gps_valid_data_write();

	snprintf(smsmsg, sizeof(smsmsg)-1, "Accumulate distance :  %um at the end. IntVatt : %umV\n",
		mileage_get_m(), battery_get_battlevel_internal());

	devel_send_sms_noti(smsmsg, strlen(smsmsg), 3);
	devel_webdm_send_log(smsmsg);
	
	sender_wait_empty_network(WAIT_PIPE_CLEAN_SECS);
	poweroff(log, strlen(log));
	return 0;
}

static char *_get_mem(void)
{
	static char buff[1000] = {0};
	FILE *fp;

	fp = fopen("/proc/meminfo", "r");
	if(fp == NULL)
	{
		printf("fopen fail\n");
		return NULL;
	}

	fread(buff, sizeof(buff), 1, fp);

	fclose(fp);
	
	return buff;
}

static char *_get_ps(void)
{
	DIR* dir;
	struct dirent* ent;
	char buf[512];
	FILE *fp = NULL;
	long  pid;
	long msize;
	char pname[100] = {0,};
	char state;
	static char buff[3000] = {0};
	char *pbuff = NULL;
	int length = 0;
	
	if(!(dir = opendir("/proc"))) {
		perror("can't open /proc");
		return NULL;
	}
	
	pbuff = buff;
	while((ent = readdir(dir)) != NULL) {
		long lpid = atol(ent->d_name);
		if(lpid <= 0) {
			continue;
		}
		snprintf(buf, sizeof(buf), "/proc/%ld/stat", lpid);
		fp = fopen(buf, "r");
		if(fp) {
			if((fscanf(fp, "%ld (%[^)]) %c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %ld", &pid, pname, &state, &msize)) != 4) {
				fclose(fp);
				closedir(dir);
				return NULL;
			}
			
			
			length = sprintf(pbuff, "%ld %s %c %ld\n", pid, pname, state, msize/1024);
			pbuff += length;

			if(buff - pbuff >= 2800)
			{
				fclose(fp);
				break;
			}
			fclose(fp);
		}
	}
	closedir(dir);
	
	return buff;
}

static void _check_mem(void)
{
	static time_t ktime_chk_mem = 0;
	int remain_mem = 0;
	
	if(ktime_chk_mem == 0)
	{
		ktime_chk_mem = tools_get_kerneltime();
	}
	else if(tools_get_kerneltime() - ktime_chk_mem < 60)
	{
		return;
	}

	remain_mem = tools_get_available_memory();
	if(remain_mem >= 0 && remain_mem < 5000) //5000KB(5MB) 
	{
		char *outdata = NULL;
		
		outdata = _get_mem();
		if(outdata != NULL)
			devel_webdm_send_log("%s", outdata);
		outdata = _get_ps();
		if(outdata != NULL)
			devel_webdm_send_log("%s", outdata);
		devel_webdm_send_log("Warn : Lack of Available memory. %d", remain_mem);
		_process_poweroff("_check_mem:leak mem");
	}
}

void network_fail_emergency_reset_callback()
{
    gps_valid_data_write();
    mileage_write();
    poweroff("netfail", strlen("netfail"));
}

void gps_ant_ok_callback(void)
{

}

void gps_ant_nok_callback(void)
{
    
}