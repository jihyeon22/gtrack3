#ifndef __BOARD_LED_H__
#define __BOARD_LED_H__

#include <pthread.h>

typedef enum ledName ledName_t;
enum ledName
{
	ePOWER_LED = 0,
	eWCDMA_LED,
	eGPS_LED,
	eMAX_LED
};

typedef enum ledColor ledColor_t;
enum ledColor
{
	eCOLOR_RED = 0,
	eCOLOR_GREEN,
	eCOLOR_YELLOW,
	eCOLOR_BLUE,
	eCOLOR_MAGENTA,
	eCOLOR_CYAN,
	eCOLOR_WHITE,
	eCOLOR_MAX
};

typedef enum ledCmd ledCmd_t;
enum ledCmd
{
	eLedcmd_SYSTEM_INIT = 0,
	eLedcmd_GPS_SEARCH,
	eLedcmd_GPS_BAD,
	eLedcmd_GPS_GOOD,
	eLedcmd_KEY_NOTI_OFF,
	eLedcmd_KEY_NOTI,
	eLedcmd_ERR_NOTI,
	eLedcmd_PWR_ON_NOTI,
	eLedcmd_PWR_OFF_NOTI,
	eLedcmd_INIT_NOTI,
};

typedef struct ledStatus ledStatus_t;
struct ledStatus
{
	pthread_mutex_t status_lock;
	int back_status;
	int status;
};

void led_on(const ledName_t led, const ledColor_t color);
void led_off(const ledName_t led);
void led_blink(const ledName_t led, const ledColor_t color, const int delay_on, const int delay_off);
int led_noti(const ledCmd_t cmd);
void led_set_all(const ledColor_t color, const int on);

#endif

