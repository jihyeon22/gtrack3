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
	int fd = 0, value = 0;
	char status[2] = {0};
	char buf[32] = {0};

	// TODO : TX500 is not gpio support
	if (gpio == GPIO_SRC_NUM_IGNITION)
		return 0;
	if (gpio == GPIO_SRC_NUM_POWER)
		return 1;

	/* check valied gpio number */
	if(!_valid_gpio(gpio)) {
		fprintf(stderr, "Gpio[%d] is not valied\n", gpio);
		goto err;
	}

	sprintf(buf, GPIO_VALUE, gpio);

	/* open gpio sysfs */
	fd = open(buf, O_RDWR);
	if(fd < 0) {
		fprintf(stderr, "Could not open gpio sysfs\n");
		goto err;
	}

	/* read gpio value */
	read(fd, status, 1);
	value = atoi(status);

	/* close gpio sysfs */
	close(fd);
	return value;

err:
	return -1;
}

int gpio_set_value(const int gpio, const int value)
{
	int fd = 0;
	char buf[32] = {0};

	/* check valied gpio number */
	if(!_valid_gpio(gpio)) {
		printf("gpio[%d] is not valied\n", gpio);
		return -1;
	}

	sprintf(buf, GPIO_VALUE, gpio);

	fd = open(buf, O_RDWR);
	if(fd < 0) {
		printf("gpio sysfs open failed\n");
		return -1;
	}

	if(value) {
		write(fd, "1", 1);
	}
	else {
		write(fd, "0", 1);
	}

	close(fd);
	return 0;
}

int gpio_set_direction(const int gpio, gpioDirection_t direction)
{
	int fd = 0;
	char buf[32] = {0};

	/* check valied gpio number */
	if(!_valid_gpio(gpio)) {
		printf("gpio[%d] is not valied\n", gpio);
		return -1;
	}

	sprintf(buf, GPIO_DIRECTION, gpio);

	fd = open(buf, O_RDWR);
	if(fd < 0) {
		printf("gpio sysfs open failed\n");
		return -1;
	}

	if(direction) {
		write(fd, "out", sizeof("out"));
	}
	else {
		write(fd, "in", sizeof("in"));
	}

	close(fd);
	return 0;
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
