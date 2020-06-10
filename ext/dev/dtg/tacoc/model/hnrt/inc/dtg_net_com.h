#ifndef __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__
#define __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__

#define HNURI_RESP_ERR_UNKOWN_DEVICE		200
#define HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE	101

#include "dtg_data_manage.h"
int disconnect_to_server(int sock_fd);
int connect_to_server(char *host_name, int host_port);
int send_to_dtg_server(int sock_fd, unsigned char *buffer_in, int buffer_len, char *func, int line, char *msg);
int receive_response(int sock_fd, int dtg_packet);
void build_mdt_report_msg(mdt_pck_t* report_msg, msg_mdt_t* stream, unsigned char event_num);
int save_mdt_buf(unsigned char *stream, int stream_size, int event_code);
int clear_mdt_buf(int clear_index);
#endif
