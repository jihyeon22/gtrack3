#ifndef __MODEL_DATA_LIST_H__
#define __MODEL_DATA_LIST_H__

#include <util/list.h>

extern listInstance_t lila_dtg_data_buffer_list;
extern listInstance_t lila_adas_data_buffer_list;

int get_lila_dtg_data_count();
int get_lila_adas_data_count();
#endif
