#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <board/gpio.h>
#include "led.h"
#include <logd_rpc.h>

static void _led_set_trigger(const char *name, char *trigger);
static void _led_trigger_timer(const char *name, char *delay_on, char *delay_off);
static void _led_set_color(const char *name, const char *color);
static void _led_set_brightness(const char *name, unsigned char value);
static void _led_set_control(const char *name, unsigned char value);

static const char *led_name[] = {
	"power",
	"wcdma",
	"gps"
};

static const char *led_color[] = {
	"red",
	"green",
	"yellow",
	"blue",
	"magenta",
	"cyan",
	"white"
};

ledStatus_t gps_led =
{
	.status_lock = PTHREAD_MUTEX_INITIALIZER,
	.back_status = eLedcmd_SYSTEM_INIT,
	.status = eLedcmd_SYSTEM_INIT
};

void led_on(const ledName_t led, const ledColor_t color)
{
	_led_set_trigger(led_name[led], "none");
	_led_set_color(led_name[led], led_color[color]);
	_led_set_brightness(led_name[led], 255);
}

void led_off(const ledName_t led)
{
	_led_set_color(led_name[led], "none");
	_led_set_brightness(led_name[led], 0);
}

void led_blink(const ledName_t led, const ledColor_t color, const int delay_on, const int delay_off)
{
	char on_time[10] = {0}, off_time[10] = {0};

	sprintf(on_time, "%d", delay_on);
	sprintf(off_time, "%d", delay_off);

	_led_set_color(led_name[led], led_color[color]);
	_led_trigger_timer(led_name[led], on_time, off_time);
	_led_set_brightness(led_name[led], 255);
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
			_led_set_control("wcdma", '1');
			
			led_blink(eGPS_LED, eCOLOR_RED, 1000, 1000);
			led_blink(eWCDMA_LED, eCOLOR_RED, 1000, 1000);
			led_blink(ePOWER_LED, eCOLOR_RED, 1000, 1000);
			break;
		case eLedcmd_INIT_NOTI:
			_led_set_control("wcdma", '1');
			
			led_blink(eGPS_LED, eCOLOR_GREEN, 500, 500);
			led_blink(eWCDMA_LED, eCOLOR_GREEN, 500, 500);
			led_blink(ePOWER_LED, eCOLOR_GREEN, 500, 500);
			break;
		case eLedcmd_PWR_ON_NOTI:
			_led_set_control("power", '1');
			led_on(ePOWER_LED, eCOLOR_GREEN);
			break;
		case eLedcmd_PWR_OFF_NOTI:
			_led_set_control("power", '1');
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
	_led_set_control("wcdma", '1');

	if(on)
	{
		led_on(ePOWER_LED, color);
		led_on(eWCDMA_LED, color);
		led_on(eGPS_LED, color);
	}
	else
	{
		led_off(ePOWER_LED);
		led_off(eWCDMA_LED);
		led_off(eGPS_LED);
	}
}

static void _led_set_trigger(const char *name, char *trigger)
{
	int fd;
	char buf[64] = {0};

	sprintf(buf, "/sys/class/leds/neow200::%s/trigger", name);
	fd = open(buf, O_RDWR);
	if(fd < 0) {
		return;
	}

	write(fd, trigger, strlen(trigger));
	close(fd);
}

static void _led_trigger_timer(const char *name, char *delay_on, char *delay_off)
{
	int fd;
	char buf[64] = {0};

	sprintf(buf, "/sys/class/leds/neow200::%s/trigger", name);
	fd = open(buf, O_RDWR);
	if(fd < 0) {
		return;
	}

	write(fd, "timer", sizeof("timer"));
	close(fd);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/leds/neow200::%s/delay_on", name);
	fd = open(buf, O_RDWR);
	write(fd, delay_on, strlen(delay_on));
	close(fd);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/sys/class/leds/neow200::%s/delay_off", name);
	fd = open(buf, O_RDWR);
	write(fd, delay_off, strlen(delay_off));
	close(fd);
}

static void _led_set_color(const char *name, const char *color)
{
	int fd;
	char buf[64] = {0};

	sprintf(buf, "/sys/class/leds/neow200::%s/color", name);

	fd = open(buf, O_RDWR);
	if(fd < 0) {
		return;
	}

	if(color) {
		write(fd, color, strlen(color));
	}
	else {
		write(fd, "none", strlen("none"));
	}

	close(fd);

}

static void _led_set_brightness(const char *name, unsigned char value)
{
	int fd;
	char buf[64] = {0};
	uint8_t brightness = value;

	sprintf(buf, "/sys/class/leds/neow200::%s/brightness", name);

	fd = open(buf, O_RDWR);
	if(fd < 0) {
		return;
	}

	if(brightness) {
		write(fd, "255", 3);
	}
	else {
		write(fd, "0", 1);
	}

	close(fd);
}

static void _led_set_control(const char *name, unsigned char value)
{
	int fd;
	char buf[64] = {0};

	sprintf(buf, "/sys/class/leds/neow200::%s/manage", name);

	fd = open(buf, O_RDWR);
	if(fd < 0) {
		return;
	}

	write(fd, &value, 1);

	close(fd);
}

