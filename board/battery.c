#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <board/gpio.h>
#include <board/battery.h>
#include <util/tools.h>
#include <logd_rpc.h>


#include <mdsapi/mds_api.h>
#include <at/at_util.h>

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
	int batt_volt = 0;
	if (at_get_adc_main_pwr(&batt_volt) == AT_RET_SUCCESS)
		return batt_volt*1000;
	else
		return 0;
	//return _adjust_battlevel_car(adc_raw);
}

int battery_get_battlevel_internal(void)
{
#if defined (BOARD_TX501S) || defined (BOARD_TX500S) || defined (BOARD_TX500L)
	return 3000;
#endif

#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
	int batt_volt = 0;
	if ( mds_api_get_internal_batt_tl500(&batt_volt) == DEFINES_MDS_API_OK )
	{
		printf("internal batt get success [%d]\r\n", batt_volt);
		return batt_volt*10;
	}
	else 
	{
		printf("internal batt get fail [%d]\r\n", batt_volt);
		return 0;
	}
#endif

}

static int _adjust_battlevel_car(const unsigned char val)
{
	return 0;
}

