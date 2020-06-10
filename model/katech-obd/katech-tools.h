#ifndef __KATECH_TOOLS_H__
#define __KATECH_TOOLS_H__

#include "katech-packet.h"

unsigned char get_checksum(unsigned char* buff, int buff_size);
void _debug_print_report1_pkt(REPORT_DATA_1_BODY_DATA* pkt_body);
void _debug_print_report2_pkt(REPORT_DATA_2_BODY_DATA* pkt_body);

void _pkt_data_convert(const int input_data, const int out_type, char* buff);

int katech_tools__set_svr_stat(int svr_stat);
int katech_tools__get_svr_stat();
int katech_tools__get_dev_id(char* dev_id);
int katech_tools__get_auth_key(char* auth_key);
int katech_tools__set_auth_key(char* auth_key);
int katech_tools__set_server_ip(char* server_ip);
int katech_tools__get_server_port(int server_port);


#endif
