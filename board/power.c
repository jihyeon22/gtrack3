#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <board/power.h>
#include <board/gpio.h>
#include <util/tools.h>
#include <logd_rpc.h>

#include "board_system.h"

int power_get_ignition_status(void)
{
	int value = gpio_get_value(GPIO_SRC_NUM_IGNITION);

	if(value == POWER_IGNITION_ON)
	{
		return POWER_IGNITION_ON;
	}
	else if(value == POWER_IGNITION_OFF)
	{
		return POWER_IGNITION_OFF;
	}
	else
	{
		return -1;
	}
}

int power_get_power_source(void)
{
	int value = gpio_get_value(GPIO_SRC_NUM_POWER);

	if(value == POWER_SRC_DC)
	{
		return POWER_SRC_DC;
	}
	else if(value == POWER_SRC_BATTERY)
	{
		return POWER_SRC_BATTERY;
	}
	else
	{
		return -1;
	}

}

