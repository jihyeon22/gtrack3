#ifndef __DISC_DTG_PKT_H__
#define __DISC_DTG_PKT_H__

#include "dsic_dtg_data_manage.h"


// dtg gtrack tool util..
void set_current_dtg_data(unsigned char *std_buff, int std_buff_len);
int bulk_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest);
int current_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int ev);

// pkt tool
int dtg_dsic__make_bulk_pkt(char* stream, int len, char** buf);
int dtg_dsic__make_evt_pkt(char* stream, int len, char** buf, int evt);
void dtg_dsic__send_key_evt(int power);
void dtg_dsic__send_power_evt(int power);


#endif // __DISC_DTG_PKT_H__
