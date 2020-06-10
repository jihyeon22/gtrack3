#ifndef __LINUX_NETCOM_H
#define __LINUX_NETCOM_H

#include <base/gpstool.h>
#include <util/transfer.h>

#define REPORT_PERIOD_EVENT	5
#define REPORT_POWER_EVENT	6
#define REPORT_TURNON_EVENT	26
#define REPORT_TURNOFF_EVENT	27
#define REPORT_DEPARTURE_EVENT	90
#define REPORT_RETURN_EVENT	91
#define REPORT_ARRIVAL_EVENT	92
#define REPORT_WAITING_EVENT	93

#define WAIT_PIPE_CLEAN_SECS 60

typedef struct period_data
{
	unsigned char *encbuf;
	int enclen;
} PERIOD_DATA;

int send_status_webdm_server(char opcode);
int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
void init_network_param();
int setting_network_param(void);
int get_report_network_param(transferSetting_t* param);
int free_packet(void *packet);

#endif

