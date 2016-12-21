#ifndef __BOARD_GPIO_H__
#define __BOARD_GPIO_H__

typedef enum gpioDirection gpioDirection_t;
enum gpioDirection
{
	eGpioInput = 0,
	eGpioOutput
};

int gpio_get_value(const int gpio);
int gpio_set_value(const int gpio, const int value);
int gpio_set_direction(const int gpio, gpioDirection_t direction);

#ifdef USE_EXTGPIO_EVT
int init_gpioinput_module(const char* gpio_list);
int get_gpioinput_module_stat(void);	
#endif

#endif
