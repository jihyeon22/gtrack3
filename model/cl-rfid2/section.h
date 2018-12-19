#ifndef __MODEL_SECTION_H__
#define __MODEL_SECTION_H__

#include <base/gpstool.h>

int section_check(gpsData_t *gpsdata);
int section_setup_from_str(int index, char *str_cond);
int section_setup_get(int index);
void section_setup_set(int index, unsigned int value);
void section_setup_from_config(void);
void section_setup_to_config(void);
void section_clear_acc_dist(void);

#endif
