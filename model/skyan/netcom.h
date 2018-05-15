#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#define WAIT_PIPE_CLEAN_SECS	60
#define MAX_SEND_RETRY_CNT      5

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int free_packet(void *packet);
int setting_network_param(void);


typedef enum {
    e_skyan_pkt__evt,
    e_skyan_pkt__req_geofence_info,
    e_skyan_pkt__geofence_evt,
    e_skyan_pkt__normal_period,
} e_ALLOC2_MSG_TYPE;

#endif

