<<<<<<< HEAD
#ifndef __DTG_PKT_SERNAIO_H__
#define __DTG_PKT_SERNAIO_H__

int bizincar_dtg_init();
int bizincar_dtg_pkt_sernaio();

int bizincar_dtg__clr_dtg_taco_buff();
int bizincar_dtg__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len);
int bizincar_dtg__make_evt_pkt(unsigned char **pbuf, unsigned short *packet_len, int evt);

#define CY_DTG_CMD__LOC_LOOKUP      "27"
#define CY_DTG_CMD__DTG_DEV_INFO    "01"


struct bizincar_dtg_respose {
	unsigned char packet_ret_code;
}__attribute__((packed));
typedef struct bizincar_dtg_respose bizincar_dtg_respose_t;


int bizincar_dtg__vehicle_odo();
int bizincar_dtg__vehicle_odo_diff_mdt();
int bizincar_dtg__vehicle_odo_diff_dvr();
int bizincar_dtg__key_stat();
int bizincar_dtg__vehicle_speed();


#endif

=======
#ifndef __DTG_PKT_SERNAIO_H__
#define __DTG_PKT_SERNAIO_H__

int bizincar_dtg_init();
int bizincar_dtg_pkt_sernaio();

int bizincar_dtg__clr_dtg_taco_buff();
int bizincar_dtg__make_period_pkt(unsigned char **pbuf, unsigned short *packet_len);
int bizincar_dtg__make_evt_pkt(unsigned char **pbuf, unsigned short *packet_len, int evt);

#define CY_DTG_CMD__LOC_LOOKUP      "27"
#define CY_DTG_CMD__DTG_DEV_INFO    "01"


struct bizincar_dtg_respose {
	unsigned char packet_ret_code;
}__attribute__((packed));
typedef struct bizincar_dtg_respose bizincar_dtg_respose_t;


int bizincar_dtg__vehicle_odo();
int bizincar_dtg__vehicle_odo_diff_mdt();
int bizincar_dtg__vehicle_odo_diff_dvr();
int bizincar_dtg__key_stat();
int bizincar_dtg__vehicle_speed();


#endif

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
