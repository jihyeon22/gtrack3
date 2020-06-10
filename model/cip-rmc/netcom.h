#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#include <base/gpstool.h>
#include <util/transfer.h>

#define WAIT_PIPE_CLEAN_SECS	60

typedef struct period_data periodData_t;
struct period_data
{
	unsigned char *encbuf;
	int enclen;
};

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int make_partial_period_packet(unsigned char **pbuf, unsigned short *packet_len, gpsData_t *gpsdata);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int free_packet(void *packet);
int setting_network_param(void);

extern transferSetting_t gSetting_report;
extern transferSetting_t gSetting_request;

#endif

