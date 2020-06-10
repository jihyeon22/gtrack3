#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#include <include/defines.h>
#include <base/gpstool.h>

#define WAIT_PIPE_CLEAN_SECS	60

typedef struct paramMsi paramMsi_t;
struct paramMsi
{
	unsigned char ip[DEFINES_IP_BUFF_LEN];
	unsigned int port;
};

typedef struct paramMit paramMit_t;
struct paramMit
{
	unsigned int interval;
	unsigned int max_packet;
};

enum PACKET_TYPE
{
	PACKET_TYPE_EVENT = 0,
	PACKET_TYPE_REPORT,
	PACKET_TYPE_RFID,
	PACKET_TYPE_MSI,
	PACKET_TYPE_MIT,
	PACKET_TYPE_CSS,
	PACKET_TYPE_RAW,
};

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int make_partial_period_packet(unsigned char **pbuf, unsigned short *packet_len, gpsData_t *gpsdata);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int free_packet(void *packet);

#endif

