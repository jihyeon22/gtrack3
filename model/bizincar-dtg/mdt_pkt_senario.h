#ifndef __MDT_PKT_SERNAIO_H__
#define __MDT_PKT_SERNAIO_H__

struct bizincar_mdt_response {
	unsigned char packet_ret_code;
}__attribute__((packed));
typedef struct bizincar_mdt_response bizincar_mdt_response_t;


int bizincar_mdt_pkt_sernaio();

int bizincar_mdt__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len);
int bizincar_mdt__make_event_pkt(unsigned char **pbuf, unsigned short *packet_len, int eventCode);
int bizincar_mdt__parse_resp(bizincar_mdt_response_t* resp);


#endif
