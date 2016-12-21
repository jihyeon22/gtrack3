#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <board/power.h>
#include <board/gpio.h>
#include <util/tools.h>
#include <logd_rpc.h>

int power_get_ignition_status(void)
{
	// tx500 is always..
	return POWER_IGNITION_ON;
}

int power_get_power_source(void)
{
	// tx500 is always..
	return POWER_SRC_DC;

}

