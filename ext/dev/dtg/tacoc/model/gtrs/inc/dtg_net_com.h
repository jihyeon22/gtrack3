#ifndef __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__
#define __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__

int connect_to_server(char *host_name, int host_port);
int send_to_dtg_server(int sock_fd, unsigned char *buffer_in, int buffer_len, char *func, int line, char *msg);
int receive_response(int sock_fd, unsigned char *recv_buf, int max_recv_buf_len);
#endif
