<<<<<<< HEAD
#ifndef __BOARD_GPIO_H__
#define __BOARD_GPIO_H__

#include <mdsapi/mds_api.h>

int gpio_get_value(const int gpio);
int gpio_set_value(const int gpio, const int value);
int gpio_set_direction(const int gpio, gpioDirection_t direction);

#ifdef USE_EXTGPIO_EVT
int init_gpioinput_module(const char* gpio_list);
int get_gpioinput_module_stat(void);	
#endif

#endif
=======
#ifndef __BOARD_GPIO_H__
#define __BOARD_GPIO_H__

#include <mdsapi/mds_api.h>

int gpio_get_value(const int gpio);
int gpio_set_value(const int gpio, const int value);
int gpio_set_direction(const int gpio, gpioDirection_t direction);

#ifdef USE_EXTGPIO_EVT
int init_gpioinput_module(const char* gpio_list);
int get_gpioinput_module_stat(void);	
#endif

#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
