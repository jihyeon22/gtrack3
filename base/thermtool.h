#ifndef __BASE_THERMTOOL_H__
#define __BASE_THERMTOOL_H__

#include <board/thermometer.h>

#define THERM_DEFAULT_TIME_SENSE 180
#define THERM_MAX_FAULT 3
#define ALWAYS_READ_THERM

int therm_get_curr_data(THERMORMETER_DATA *out);
void therm_set_curr_data(THERMORMETER_DATA* in);
int therm_sense(void);
void therm_clear_fault(void);
void therm_set_sense_cycle(int val);
int therm_set_dev(char *dev, int len_dev);

#endif
