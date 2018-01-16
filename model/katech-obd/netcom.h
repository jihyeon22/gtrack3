#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#define WAIT_PIPE_CLEAN_SECS	60

#define DEFAULT_SETTING_SOCK_CONN_RETRY_CNT     2
#define DEFAULT_SETTING_SOCK_SEND_RETRY_CNT     3
#define DEFAULT_SETTING_SOCK_RCV_RETRY_CNT      3
#define DEFAULT_SETTING_SOCK_TIMEOUT_SEC        60

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int free_packet(void *packet);

#endif

