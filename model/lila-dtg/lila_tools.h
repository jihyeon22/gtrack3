#ifndef __LILA_TOOLS_H__
#define __LILA_TOOLS_H__

unsigned short lila_tools__crc16_ccitt(const void *buf, int len);

int lila_tools__get_phonenum_int_type();
char* lila_tools__conv_buff_to_str(char* src, int src_size);
char* lila_tools__conv_buff_to_str_reverse(char* src, int src_size);

#include "dtg_gtrack_tool.h"

void lila_dtg__set_current_dtg_data(tacom_std_hdr_t* p_current_std_hdr, tacom_std_data_t* p_current_std_data);
void lila_dtg__get_current_dtg_data(tacom_std_hdr_t* p_current_std_hdr, tacom_std_data_t* p_current_std_data);

unsigned char lila_dtg__convert_angle(int bearing);


#define LILA_TOOLS__CALC_INTERVAL   60

unsigned char  lila_tools__get_adas_data_aebs(unsigned int cur_time);
unsigned char  lila_tools__get_adas_data_ldw(unsigned int cur_time);
unsigned char  lila_tools__get_rssi(unsigned int cur_time);
unsigned short lila_tools__get_car_batt(unsigned int cur_time);
char lila_tools__get_gps_stat(int gps_lat, int gps_lon);
unsigned short lila_tools__get_car_stat(char* buff);
unsigned short lila_tools__get_car_signal();

#endif // __LILA_TOOLS_H__

