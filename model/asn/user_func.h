<<<<<<< HEAD
#pragma once

void check_internal_uart_use();
int check_uart_fd(int *fd);

int check_time_request_packet(unsigned char *packet_buf, int packet_len);
int request_server_time(unsigned char *req_buf, int req_len, unsigned char *recv_buf, int recv_len);
int check_cu_power_on_reset_packet(unsigned char *packet_buf, int packet_len);
void uart_buffer_data_network_push();
void add_uart_buffer(unsigned char *packet_buf, int packet_len);
void dump_data(char *debug_title, unsigned char *data, int data_len) ;
=======
#pragma once

void check_internal_uart_use();
int check_uart_fd(int *fd);

int check_time_request_packet(unsigned char *packet_buf, int packet_len);
int request_server_time(unsigned char *req_buf, int req_len, unsigned char *recv_buf, int recv_len);
int check_cu_power_on_reset_packet(unsigned char *packet_buf, int packet_len);
void uart_buffer_data_network_push();
void add_uart_buffer(unsigned char *packet_buf, int packet_len);
void dump_data(char *debug_title, unsigned char *data, int data_len) ;
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
