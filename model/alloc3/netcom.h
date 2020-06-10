#ifndef __MODEL_CALLBACK_H__
#define __MODEL_CALLBACK_H__

#include <base/gpstool.h>
#include <util/transfer.h>

#define WAIT_PIPE_CLEAN_SECS	60

#define DEFAULT_SETTING_SOCK_CONN_RETRY_CNT     2
#define DEFAULT_SETTING_SOCK_SEND_RETRY_CNT     3
#define DEFAULT_SETTING_SOCK_RCV_RETRY_CNT      3
#define DEFAULT_SETTING_SOCK_TIMEOUT_SEC        5

typedef struct period_data periodData_t;
struct period_data
{
	unsigned char *encbuf;
	int enclen;
};

enum PACKET_TYPE
{
	PACKET_TYPE_EVENT = 0,
	PACKET_TYPE_REPORT,
	PACKET_TYPE_GET_RFID,
	PACKET_TYPE_GET_GEOFENCE, 
	PACKET_TYPE_GEO_FENCE_IN,
	PACKET_TYPE_GEO_FENCE_OUT,
	PACKET_TYPE_TAGGING,
	PACKET_TYPE_RESP_SMS,
	PACKET_TYPE_REQUEST_BUS_STOP_INFO, //jwrho
};

// report pkt info
typedef struct {
	gpsData_t gpsdata;
	int diff_distance;
	int evt_code;
	char zone_id[8];
	int  zone_idx;
	int  zone_stat;
}__attribute__((packed))alloc_evt_pkt_info_t;



typedef struct {
	char version[8];
}__attribute__((packed))alloc_get_rfid_pkt_info_t;

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param);
int make_partial_period_packet(unsigned char **pbuf, unsigned short *packet_len, gpsData_t *gpsdata);
int send_packet(char op, unsigned char *packet_buf, int packet_len);
int free_packet(void *packet);
int transfer_packet_recv_call(int op, const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len);
int setting_network_param(void);

#endif

