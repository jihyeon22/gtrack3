#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <board/gpio.h>
#include <board/battery.h>
#include <util/tools.h>
#include <logd_rpc.h>

static int _adjust_battlevel_car(const unsigned char val);

int fd_adc = -1;

battLevel_t adc2volt_car_batt[] =
{
	{0, 0}
};

int battery_init_adc(void)
{
	return 0;
}

void battery_deinit_adc(void)
{
	return;
}

int battery_get_battlevel_car(void)
{
	return 12000;
	//return _adjust_battlevel_car(adc_raw);
}

int battery_get_battlevel_internal(void)
{
	return 3000;
}

static int _adjust_battlevel_car(const unsigned char val)
{
	return 0;
}

