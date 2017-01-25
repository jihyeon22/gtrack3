#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "board_system.h"

#include "gpio.h"


#ifdef USE_EXTGPIO_EVT
#include "util/tools.h"
#endif

static int _valid_gpio(const int gpio);

int gpio_get_value(const int gpio)
{
	// TODO : TX501 is not gpio support
#if defined (BOARD_TX501S) || defined (BOARD_TX500S)
	if (gpio == GPIO_SRC_NUM_IGNITION)
		return 0;
	if (gpio == GPIO_SRC_NUM_POWER)
		return 1;
#endif
	return mds_api_gpio_get_value(gpio) ;
}

int gpio_set_value(const int gpio, const int value)
{
	return mds_api_gpio_set_value(gpio, value);
}

int gpio_set_direction(const int gpio, gpioDirection_t direction)
{
	return mds_api_gpio_set_direction(gpio, direction);
}


static int _valid_gpio(const int gpio)
{
	return (gpio < GPIO_NO_MIN || gpio > GPIO_NO_MAX) ? 0 : 1;
}

#ifdef USE_EXTGPIO_EVT

static int g_gpioinput_module_stat = 0 ;

int init_gpioinput_module(const char* gpio_list)
{
	char cmd[128] = {0,};

	int max_retry = 5;

	g_gpioinput_module_stat = 0;

	if (tools_check_exist_file(INPUT_KEY_MODULE_NAME, 2) < 0)
		return -1;

	sprintf(cmd,"%s %s set_gpio=%s &",INSMODE_CMD, INPUT_KEY_MODULE_NAME, gpio_list);

	while( max_retry-- > 0 )
	{
		// check module..
		if (tools_get_module_list("gpio_input_keys") == 0 )
		{
			g_gpioinput_module_stat = 1;
			break;
		}

		// insert module cmd...
		system(cmd);
		sleep(1);
	}

	return -1;

}

int get_gpioinput_module_stat()
{
	return g_gpioinput_module_stat;
}
#endif
