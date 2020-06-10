#include <unistd.h>

#include <board/gpio.h>
#include "buzzer.h"

int buzzer_init(void)
{
	gpio_set_direction(GPIO_SRC_NUM_BUZZER1, eGpioOutput);
	gpio_set_value(GPIO_SRC_NUM_BUZZER1,0);

	gpio_set_direction(GPIO_SRC_NUM_BUZZER2, eGpioOutput);
	gpio_set_value(GPIO_SRC_NUM_BUZZER2,0);
	
	return 0;
}

int buzzer_run(int type, int n_repeat, int buzzer_ms, int delay_ms)
{
	int gpio = 0;

	if(BUZZER1 == type)
	{
		gpio = GPIO_SRC_NUM_BUZZER1;
	}
	else
	{
		gpio = GPIO_SRC_NUM_BUZZER2;
	}

	while(n_repeat-- > 0)
	{
		gpio_set_value(gpio,1);
		usleep(buzzer_ms*1000);
		gpio_set_value(gpio,0);
		usleep(delay_ms*1000);
	}

	return 0;
}

