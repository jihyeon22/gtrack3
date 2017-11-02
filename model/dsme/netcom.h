#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#define WAIT_PIPE_CLEAN_SECS	60

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int free_packet(void *packet);
int setting_network_param(void);

#define MAX_WAIT_RETRY_TIME		180	//unit : sec
#define WAIT_INTERVAL_TIME		5	//unit : sec
#define PACKET_RETRY_COUNT		5

#endif

