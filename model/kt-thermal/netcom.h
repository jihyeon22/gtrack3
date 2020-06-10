#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#include <base/gpstool.h>



#define WAIT_PIPE_CLEAN_SECS	60
#define USE_PACKET_64 0

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int make_partial_period_packet(unsigned char **pbuf, unsigned short *packet_len, gpsData_t *gpsdata);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int setting_network_param(void);
int free_packet(void *packet);
int make_record_thermal(char *record);

//only for lotte ++
#define MAX_WAIT_RETRY_TIME		180	//unit : sec
#define WAIT_INTERVAL_TIME		5	//unit : sec
#define PACKET_RETRY_COUNT		5
//only for lotte --

//======================================================
//      FEATURE_DIVIDE_MERGED_PACKET
// if server doesn't support merged packet,
// send single data per packet.
#define FEATURE_DONT_USE_MERGED_PACKET	0
//======================================================

#endif

