<<<<<<< HEAD
#include <stdlib.h>
#include <unistd.h>

#include <base/devel.h>
#include <util/tools.h>
#include <util/poweroff.h>
#include <board/gpio.h>

void poweroff(const char *log_buff, const int log_buff_len)
{
	int power_off_cnt = 0;
	if(log_buff != NULL)
	{
		devel_log_poweroff(log_buff, log_buff_len);
	}

	tools_alive_end(); //njw 150924
	
#ifdef USE_GPS_MODEL
	gps_valid_data_write();
	mileage_write();
#endif

#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
	gpio_set_value(15, 0);
#endif

	while(1)
	{
		system("poweroff &");
		sleep(10);
		if(power_off_cnt++ > 10) {
			system("echo c > /proc/sysrq-trigger");
		}
	}
}

=======
#include <stdlib.h>
#include <unistd.h>

#include <base/devel.h>
#include <util/tools.h>
#include <util/poweroff.h>
#include <board/gpio.h>

void poweroff(const char *log_buff, const int log_buff_len)
{
	int power_off_cnt = 0;
	if(log_buff != NULL)
	{
		devel_log_poweroff(log_buff, log_buff_len);
	}

	tools_alive_end(); //njw 150924
	
#ifdef USE_GPS_MODEL
	gps_valid_data_write();
	mileage_write();
#endif

#if defined (BOARD_TL500S) || defined (BOARD_TL500K) || defined (BOARD_TL500L)
	gpio_set_value(15, 0);
#endif

	while(1)
	{
		system("poweroff &");
		sleep(10);
		if(power_off_cnt++ > 10) {
			system("echo c > /proc/sysrq-trigger");
		}
	}
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
