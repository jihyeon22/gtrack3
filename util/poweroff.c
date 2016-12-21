#include <stdlib.h>
#include <unistd.h>

#include <base/devel.h>
#include <util/tools.h>

void poweroff(const char *log_buff, const int log_buff_len)
{
	int power_off_cnt = 0;
	if(log_buff != NULL)
	{
		devel_log_poweroff(log_buff, log_buff_len);
	}

	tools_alive_end(); //njw 150924
	
	while(1)
	{
		system("poweroff &");
		sleep(10);
		if(power_off_cnt++ > 10) {
			system("echo c > /proc/sysrq-trigger");
		}
	}
}

