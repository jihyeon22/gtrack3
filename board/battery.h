#ifndef __BOARD_BATTERY_H__
#define __BOARD_BATTERY_H__

#include "board_system.h"

typedef struct battLevel battLevel_t;
struct battLevel
{
	int volt;
	unsigned char adc;
};

int battery_init_adc(void);
void battery_deinit_adc(void);
int battery_get_battlevel_car(void);
int battery_get_battlevel_internal(void);

#endif
