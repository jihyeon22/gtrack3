<<<<<<< HEAD
#ifndef __MDT_PKT_SERNAIO_H__
#define __MDT_PKT_SERNAIO_H__

struct bizincar_mdt_response {
	unsigned char packet_ret_code;
}__attribute__((packed));
typedef struct bizincar_mdt_response bizincar_mdt_response_t;

#define KEY_STAT_HOLD_CNT   10


int bizincar_mdt_pkt_sernaio();

int bizincar_mdt__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len);
int bizincar_mdt__make_event_pkt(unsigned char **pbuf, unsigned short *packet_len, int eventCode);
int bizincar_mdt__parse_resp(bizincar_mdt_response_t* resp);

int get_fake_ignition_key_stat();


#endif
=======
#ifndef __MDT_PKT_SERNAIO_H__
#define __MDT_PKT_SERNAIO_H__

struct bizincar_mdt_response {
	unsigned char packet_ret_code;
}__attribute__((packed));
typedef struct bizincar_mdt_response bizincar_mdt_response_t;

#define KEY_STAT_HOLD_CNT   10


int bizincar_mdt_pkt_sernaio();

int bizincar_mdt__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len);
int bizincar_mdt__make_event_pkt(unsigned char **pbuf, unsigned short *packet_len, int eventCode);
int bizincar_mdt__parse_resp(bizincar_mdt_response_t* resp);

int get_fake_ignition_key_stat();


#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
