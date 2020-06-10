#include <stdio.h>
#include <unistd.h>
typedef unsigned long kernel_ulong_t;
#define BITS_PER_LONG 32
#include <linux/input.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <board/board_system.h>
#include <base/config.h>
#include <at/at_util.h>
#include <base/devel.h>
#include <base/button.h>
#ifdef USE_GPS_MODEL
#include <base/mileage.h>
#include <base/gpstool.h>
#endif
#include <base/sender.h>
#include <base/error.h>
#include <base/dmmgr.h>
#include <base/watchdog.h>
#include <board/gpio.h>
#include <board/led.h>
#include <board/power.h>
#include <util/tools.h>
#include <util/nettool.h>
#include <util/poweroff.h>
#include <include/defines.h>
#include <config.h>
#include <netcom.h>
#include <callback.h>
#include <logd_rpc.h>

#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
#include "kt_fota.h"
#include "kt_fota_config.h"
#endif

#define DEV_BTN_TIMEOUT_SEC			5
void _check_ign_onoff(void);
void _check_led_noti_off(void);
void _check_pwr_onoff(void);

//#define EMUL_GPS

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_COMMON


static int flag_run_thread_btn_pwr = 1;

static time_t btn_1_push_time, btn_2_push_time;
static int key_led_noti;
static time_t key_led_start, key_led_end;

static time_t time_ign_hold_time;
static int ign_on = POWER_IGNITION_OFF;

static time_t time_pwr_hold_time;
static int pwr_on = POWER_SRC_DC;

void _check_btn_pwr_event(const struct input_event ev_input)
{
	wd_dbg[eWdPwr] = 21;
	
	time_t cur_time = 0;
	if(ev_input.type == 0x01 && ev_input.code == 0x1d3 && ev_input.value == 0x01)
	{
		wd_dbg[eWdPwr] = 22;
		int hold_time;
		cur_time = tools_get_kerneltime();
		hold_time = (int)round(difftime(cur_time, btn_1_push_time));
		if(hold_time > BTN_HOLD_SECS)
		{
			wd_dbg[eWdPwr] = 23;
			LOGT(LOG_TARGET, " press button 1 on!! [%d]sec\r\n", hold_time);
			btn_1_push_time = tools_get_kerneltime();
			led_noti(eLedcmd_KEY_NOTI);
			key_led_start = tools_get_kerneltime();
			key_led_noti = 1;

			button1_callback();
			wd_dbg[eWdPwr] = 24;
#ifdef EMUL_GPS
			if(emul_gps_lat > 0 || emul_gps_lon > 0)
			{
				emul_gps_lat = 0;
				emul_gps_lon = 0;
			}
			else
			{
				emul_gps_lat = 0.07;
				emul_gps_lon = 0.07;
			}
#endif
		}
	}

	else if(ev_input.type == 0x01 && ev_input.code == 0x1d2 && ev_input.value == 0x01)
	{
		wd_dbg[eWdPwr] = 25;
		int hold_time;
		cur_time = tools_get_kerneltime();
		hold_time = (int)round(difftime(cur_time, btn_2_push_time));
		if(hold_time > BTN_HOLD_SECS)
		{
			wd_dbg[eWdPwr] = 26;
			LOGT(LOG_TARGET, " press button 2 on!! [%d]sec\r\n", hold_time);
			btn_2_push_time = tools_get_kerneltime();
			led_noti(eLedcmd_KEY_NOTI);
			key_led_start = tools_get_kerneltime();
			key_led_noti = 1;

			button2_callback();
			wd_dbg[eWdPwr] = 27;
#ifdef EMUL_GPS
			if(emul_gps_speed > 0)
			{
				emul_gps_speed = 0;
			}
			else
			{
				emul_gps_speed = 200;
			}
#endif
		}
	}
	else if(ev_input.type == 0x16)
	{
		wd_dbg[eWdPwr] = 28;
		if(ev_input.code == 0x08)
		{
			if(ev_input.value == 0x01)
			{
				wd_dbg[eWdPwr] = 29;
				LOGT(LOG_TARGET, "ignition line on!!!!!!\r\n");
				time_ign_hold_time = tools_get_kerneltime();
				//_check_ign_onoff();
			}
			else
			{
				LOGT(LOG_TARGET, "ignition line off!!!!!!\r\n");
				time_ign_hold_time = tools_get_kerneltime();
				//_check_ign_onoff();
			}
		}
		else if(ev_input.code == 0x09)
		{
			if(ev_input.value == 0x00)
			{
				wd_dbg[eWdPwr] = 30;
				LOGT(LOG_TARGET, "dc mode!!!!!!\r\n");
				time_pwr_hold_time = tools_get_kerneltime();
				//_check_pwr_onoff();
			}
			else
			{
				LOGT(LOG_TARGET, "battery mode!!!!!!\r\n");
				time_pwr_hold_time = tools_get_kerneltime();
				//_check_pwr_onoff();
			}			
		}
	}
}

void _check_led_noti_off(void)
{
	wd_dbg[eWdPwr] = 31;
	if(key_led_noti) {
		key_led_end = tools_get_kerneltime();
		if(difftime(key_led_end, key_led_start) >= BTN_LED_BLINK_SECS) {
			key_led_noti = 0;
			led_noti(eLedcmd_KEY_NOTI_OFF);
		}
	}
}

void _check_ign_onoff(void)
{
	configurationBase_t *conf = get_config_base();
	int ign_hold_sec = conf->common.time_hold_ign;

	if(ign_on == POWER_IGNITION_ON)
	{
		wd_dbg[eWdPwr] = 32;
		if(tools_get_kerneltime() - time_ign_hold_time < ign_hold_sec)
		{
			wd_dbg[eWdPwr] = 33;
//jwrho 2015.02.23 ++
			if(power_get_ignition_status() == POWER_IGNITION_ON)
				time_ign_hold_time = tools_get_kerneltime();
//jwrho 2015.02.23 --
			LOGT(LOG_TARGET, "time_ign_off..[%d] sec Skip\n", ign_hold_sec);
			return;
		}

		wd_dbg[eWdPwr] = 34;
		if(power_get_ignition_status() == POWER_IGNITION_OFF)
		{
			wd_dbg[eWdPwr] = 35;
			devel_log_poweroff(__FUNCTION__, sizeof(__FUNCTION__));

			wd_dbg[eWdPwr] = 36;
			LOGT(LOG_TARGET, "Run ignition-off process\n");
#ifndef SERVER_ABBR_DSKL
			dmmgr_send(eEVENT_KEY_OFF, NULL, 0);
#endif

			wd_dbg[eWdPwr] = 37;
#ifdef USE_GPS_MODEL
			mileage_write();
			gps_valid_data_write();
#endif

			wd_dbg[eWdPwr] = 38;
			ignition_off_callback();
			wd_dbg[eWdPwr] = 39;

			time_ign_hold_time = tools_get_kerneltime();
			ign_on = POWER_IGNITION_OFF;
			
		}
//jwrho 2015.02.23 ++
		else
		{
			wd_dbg[eWdPwr] = 40;
			time_ign_hold_time = tools_get_kerneltime();
		}
//jwrho 2015.02.23 --
	}
	else
	{
		wd_dbg[eWdPwr] = 41;
		if(tools_get_kerneltime() - time_ign_hold_time < ign_hold_sec)
		{
			wd_dbg[eWdPwr] = 42;
//jwrho 2015.02.23 ++
			if(power_get_ignition_status() == POWER_IGNITION_OFF)
				time_ign_hold_time = tools_get_kerneltime();
//jwrho 2015.02.23 --
			LOGT(LOG_TARGET, "time_ign_on..[%d] sec Skip\n", ign_hold_sec);
			return;
		}

		wd_dbg[eWdPwr] = 43;
		if(power_get_ignition_status() == POWER_IGNITION_ON)
		{
			wd_dbg[eWdPwr] = 44;
			LOGT(LOG_TARGET, "Run ignition-on process\n");
			nettool_init_hostbyname_func();
			wd_dbg[eWdPwr] = 45;

#ifndef SERVER_ABBR_DSKL
			dmmgr_send(eEVENT_KEY_ON, NULL, 0);
#endif

			wd_dbg[eWdPwr] = 46;
#ifdef USE_GPS_MODEL
			gps_restart_check_distance();
#endif
			wd_dbg[eWdPwr] = 47;
			ignition_on_callback();
			wd_dbg[eWdPwr] = 48;

			time_ign_hold_time = tools_get_kerneltime();
			ign_on = POWER_IGNITION_ON;
		}
//jwrho 2015.02.23 ++
		else
		{
			wd_dbg[eWdPwr] = 49;
			time_ign_hold_time = tools_get_kerneltime();
		}
//jwrho 2015.02.23 --
		wd_dbg[eWdPwr] = 50;
	}
}

void _check_pwr_onoff(void)
{
	configurationBase_t *conf = get_config_base();
	int pwr_hold_sec = conf->common.time_hold_power;

	wd_dbg[eWdPwr] = 51;
	if(pwr_on == POWER_SRC_DC)
	{
		wd_dbg[eWdPwr] = 52;
		if(tools_get_kerneltime() - time_pwr_hold_time < pwr_hold_sec)
		{
			wd_dbg[eWdPwr] = 53;
//jwrho 2015.02.23 ++
			if(power_get_power_source() == POWER_SRC_DC)
				time_pwr_hold_time = tools_get_kerneltime();
//jwrho 2015.02.23 --
			LOGT(LOG_TARGET, "time_pwr_off..[%d] sec Skip\n", pwr_hold_sec);
			return;
		}

		wd_dbg[eWdPwr] = 54;
		if(power_get_power_source() == POWER_SRC_BATTERY)
		{
			wd_dbg[eWdPwr] = 55;
			devel_log_poweroff(__FUNCTION__, sizeof(__FUNCTION__));

			LOGT(LOG_TARGET, "Run power-off process\n");
#if defined (BOARD_TL500K) && defined (KT_FOTA_ENABLE)
			kt_fota_deinit();
#endif
			wd_dbg[eWdPwr] = 56;

#ifndef SERVER_ABBR_DSKL
			dmmgr_send(eEVENT_PWR_OFF, NULL, 0);
#endif
			wd_dbg[eWdPwr] = 57;
			// critical data backup. for simulation
			crit_backup_simul();
			
			power_off_callback();
			wd_dbg[eWdPwr] = 58;

			time_pwr_hold_time = tools_get_kerneltime();
			pwr_on = POWER_SRC_BATTERY;
		}
//jwrho 2015.02.23 ++
		else
		{
			wd_dbg[eWdPwr] = 59;
			time_pwr_hold_time = tools_get_kerneltime();
		}
		wd_dbg[eWdPwr] = 60;
//jwrho 2015.02.23 --
	}
	else
	{
		wd_dbg[eWdPwr] = 61;
		if(tools_get_kerneltime() - time_pwr_hold_time < pwr_hold_sec)
		{
			wd_dbg[eWdPwr] = 67;
//jwrho 2015.02.23 ++
			if(power_get_power_source() == POWER_SRC_BATTERY)
				time_pwr_hold_time = tools_get_kerneltime();
//jwrho 2015.02.23 --
			LOGT(LOG_TARGET, "time_pwr_on..[%d] sec Skip\n", pwr_hold_sec);
			return;
		}

		wd_dbg[eWdPwr] = 68;
		if(power_get_power_source() == POWER_SRC_DC)
		{
			wd_dbg[eWdPwr] = 69;
			LOGT(LOG_TARGET, "Run power-on process\n");

#ifndef SERVER_ABBR_DSKL
			dmmgr_send(eEVENT_PWR_ON, NULL, 0);
#endif

			wd_dbg[eWdPwr] = 70;
			power_on_callback();
			wd_dbg[eWdPwr] = 71;

			time_pwr_hold_time = tools_get_kerneltime();
			pwr_on = POWER_SRC_DC;
		}
//jwrho 2015.02.23 ++
		else
		{
			wd_dbg[eWdPwr] = 72;
			time_pwr_hold_time = tools_get_kerneltime();
		}
//jwrho 2015.02.23 --
		wd_dbg[eWdPwr] = 73;
	}
}

void *thread_btn_pwr(void *args)
{
	int fd_evt = 0;
//	int fd_cnt = 0;
	int rc;
	fd_set  readSet;
	struct  timeval tv;

	struct input_event ev_input;
	configurationBase_t *conf = get_config_base();

	LOGI(LOG_TARGET, "PID %s : %d\n", __FUNCTION__, getpid());

	if(!flag_run_thread_btn_pwr)
	{
		return NULL;
	}

	if(conf->common.first_pwr_status_on == 1)
	{
		time_pwr_hold_time = tools_get_kerneltime();
		pwr_on = POWER_SRC_DC;

#ifndef SERVER_ABBR_DSKL
		dmmgr_send(eEVENT_PWR_ON, NULL, 0);
#endif
		power_on_callback();
	}
	else
	{
		if(power_get_power_source() == POWER_SRC_BATTERY)
		{
			
			time_pwr_hold_time = tools_get_kerneltime();
			pwr_on = POWER_SRC_BATTERY;

#ifndef SERVER_ABBR_DSKL
			dmmgr_send(eEVENT_PWR_OFF, NULL, 0);
#endif
			power_off_callback();
		}
		else
		{
			
			time_pwr_hold_time = tools_get_kerneltime();
			pwr_on = POWER_SRC_DC;
#ifndef SERVER_ABBR_DSKL
			dmmgr_send(eEVENT_PWR_ON, NULL, 0);
#endif
			power_on_callback();
		}
	}

	if(power_get_ignition_status() == POWER_IGNITION_OFF)
	{
		
		time_ign_hold_time = tools_get_kerneltime();
		ign_on = POWER_IGNITION_OFF;
#ifndef SERVER_ABBR_DSKL
		dmmgr_send(eEVENT_KEY_OFF, NULL, 0);
#endif
		ignition_off_callback();
	}
	else
	{
		
		time_ign_hold_time = tools_get_kerneltime();
		ign_on = POWER_IGNITION_ON;
#ifndef SERVER_ABBR_DSKL
		dmmgr_send(eEVENT_KEY_ON, NULL, 0);
#endif
		ignition_on_callback();
	}
	btn_1_push_time = btn_2_push_time = tools_get_kerneltime();

	fd_evt = open(EVENT0_DEV_NAME, O_RDONLY | O_NONBLOCK);
	if(fd_evt < 0) {
		LOGE(LOG_TARGET, "%s(): open() error\n", __func__);
		error_critical(eERROR_REBOOT, "thread_btn_pwr open Error");
	}

	int val;

	val = fcntl(fd_evt, F_GETFD, 0);
	val |= FD_CLOEXEC;
	fcntl(fd_evt, F_SETFD, val);


	while(flag_run_thread_btn_pwr)
	{
		wd_dbg[eWdPwr] = 1;

		// 처리하지못한 dm event msg 들을 보내도록한다.
		dmmgr_send_incomplete_event();

		watchdog_set_cur_ktime(eWdPwr);
		watchdog_process();
		
		FD_ZERO(&readSet);
		FD_SET(fd_evt, &readSet);

		tv.tv_sec = DEV_BTN_TIMEOUT_SEC;
		tv.tv_usec = 0;
		
		wd_dbg[eWdPwr] = 2;
		rc = select(fd_evt + 1 , &readSet, NULL, NULL, &tv);

		wd_dbg[eWdPwr] = 3;
		if(rc < 0)
		{
			wd_dbg[eWdPwr] = 4;
			if(errno != EINTR)
			{
				wd_dbg[eWdPwr] = 5;
				LOGE(LOG_TARGET, "%s(): select error\n", __FUNCTION__);
				error_critical(eERROR_EXIT, "btn_pwr select Error");
			}
			continue;
		}

		wd_dbg[eWdPwr] = 6;
		if(rc > 0)
		{
			int chk_fd = 0;

			wd_dbg[eWdPwr] = 7;
			if( FD_ISSET(fd_evt, &readSet) )
			{
				wd_dbg[eWdPwr] = 8;
				if(read(fd_evt, &ev_input, sizeof(ev_input)) == sizeof(ev_input))
				{
					wd_dbg[eWdPwr] = 9;
					LOGI(LOG_TARGET, "type: 0x%x, code: 0x%x, value: 0x%x\n", ev_input.type, ev_input.code, ev_input.value);
					_check_btn_pwr_event(ev_input);
				}
				
				chk_fd = 1;

			}
			wd_dbg[eWdPwr] = 10;

			if (chk_fd == 0)
			{
				wd_dbg[eWdPwr] = 14;
				continue;
			}

		}
		wd_dbg[eWdPwr] = 15;
		_check_led_noti_off();
		wd_dbg[eWdPwr] = 16;
		_check_ign_onoff();
		wd_dbg[eWdPwr] = 17;
		_check_pwr_onoff();
		wd_dbg[eWdPwr] = 18;

		
	}

	wd_dbg[eWdPwr] = 19;
	close(fd_evt);
	wd_dbg[eWdPwr] = 20;


	return NULL;
}

void exit_thread_btn_pwr(void)
{
	flag_run_thread_btn_pwr = 0;
}

