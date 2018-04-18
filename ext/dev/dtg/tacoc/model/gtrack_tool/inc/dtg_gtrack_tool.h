#ifndef __H_DTG_GTRACK_TOOL_H__
#define __H_DTG_GTRACK_TOOL_H__

#include <tacom/tacom_std_protocol.h>

#define TACOC_GTRACK_TOOL__API_RET_SUCCESS  1
#define TACOC_GTRACK_TOOL__API_RET_FAIL     -1

int taco_gtrack_tool__save_dtg_data();

int taco_gtrack_tool__get_current_data(char** buff);
int taco_gtrack_tool__get_remain_cnt();
int taco_gtrack_tool__get_bulk_data(int cnt, char** buff);

int taco_gtrack_tool__conv_dtg_to_gps(tacom_std_data_t *p_std_data, gpsData_t * p_gps_data);

int get_dtg_report_period();
int get_dtg_create_period();
void set_dtg_report_period(int num);
void set_dtg_create_period(int num);

#endif
