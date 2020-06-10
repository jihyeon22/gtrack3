#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <board/gpio.h>
#include "led.h"
#include <logd_rpc.h>

#include <mdsapi/mds_api.h>

#include "board/board_system.h"

static const char *led_name[] = {
	LED_NAME_PWR,
	LED_NAME_WCDMA,
	LED_NAME_GPS
};

static const char *led_color[] = {
	LED_COLOR_RED,
	LED_COLOR_GREEN,
	LED_COLOR_YELLO,
	LED_COLOR_BLUE,
	LED_COLOR_MAGENTA,
	LED_COLOR_CYAN,
	LED_COLOR_WHITE
};

ledStatus_t gps_led =
{
	.status_lock = PTHREAD_MUTEX_INITIALIZER,
	.back_status = eLedcmd_SYSTEM_INIT,
	.status = eLedcmd_SYSTEM_INIT
};

void led_on(const ledName_t led, const ledColor_t color)
{
	mds_api_led_on(led_name[led], led_color[color]);
}

void led_off(const ledName_t led)
{
	mds_api_led_off(led_name[led]);
}

void led_blink(const ledName_t led, const ledColor_t color, const int delay_on, const int delay_off)
{
	mds_api_led_blink(led_name[led], led_color[color],delay_on,delay_off);
}

int led_noti(const ledCmd_t cmd)
{
	pthread_mutex_lock(&gps_led.status_lock);

	switch(cmd)
	{
		case eLedcmd_SYSTEM_INIT:
		case eLedcmd_GPS_GOOD:
		case eLedcmd_GPS_BAD:
		case eLedcmd_GPS_SEARCH:
		case eLedcmd_ERR_NOTI:
			if(gps_led.status == eLedcmd_KEY_NOTI)
			{
				gps_led.back_status = cmd;
			}
			else
			{
				gps_led.status = cmd;
			}
			break;
		case eLedcmd_KEY_NOTI:
			if(gps_led.status != eLedcmd_KEY_NOTI)
			{
				gps_led.back_status = gps_led.status;
				gps_led.status = eLedcmd_KEY_NOTI;
			}
			break;
		case eLedcmd_KEY_NOTI_OFF:
			gps_led.status = gps_led.back_status;
			break;
		case eLedcmd_INIT_NOTI:
			gps_led.status = eLedcmd_INIT_NOTI;
			break;
		default:
			;
	}

	switch(gps_led.status)
	{
		case eLedcmd_SYSTEM_INIT:
			led_blink(eGPS_LED, eCOLOR_RED, 250, 250);
			break;
		case eLedcmd_GPS_GOOD:
			led_on(eGPS_LED, eCOLOR_GREEN);
			break;
		case eLedcmd_GPS_BAD:
			led_on(eGPS_LED, eCOLOR_RED);
			break;
		case eLedcmd_GPS_SEARCH:
			led_blink(eGPS_LED, eCOLOR_RED, 1000, 1000);
			break;
		case eLedcmd_KEY_NOTI:
			led_blink(eGPS_LED, eCOLOR_YELLOW, 250, 250);
			break;
		case eLedcmd_ERR_NOTI:
			led_blink(eGPS_LED, eCOLOR_RED, 1000, 1000);
			led_blink(eWCDMA_LED, eCOLOR_RED, 1000, 1000);
			led_blink(ePOWER_LED, eCOLOR_RED, 1000, 1000);
			break;
		case eLedcmd_INIT_NOTI:
			led_blink(eGPS_LED, eCOLOR_GREEN, 500, 500);
			led_blink(eWCDMA_LED, eCOLOR_GREEN, 500, 500);
			led_blink(ePOWER_LED, eCOLOR_GREEN, 500, 500);
			break;
		case eLedcmd_PWR_ON_NOTI:
			led_on(ePOWER_LED, eCOLOR_GREEN);
			break;
		case eLedcmd_PWR_OFF_NOTI:
			led_blink(ePOWER_LED, eCOLOR_RED, 1000, 1000);
			break;
		default:
			;
	}

	pthread_mutex_unlock(&gps_led.status_lock);
	return 0;
}

void led_set_all(const ledColor_t color, const int on)
{
	if(on)
	{
		mds_api_led_on(led_name[ePOWER_LED], led_color[color]);
		mds_api_led_on(led_name[eWCDMA_LED], led_color[color]);
		mds_api_led_on(led_name[eGPS_LED], led_color[color]);
	}
	else
	{
		mds_api_led_off(led_name[ePOWER_LED]);
		mds_api_led_off(led_name[eWCDMA_LED]);
		mds_api_led_off(led_name[eGPS_LED]);
	}
}
