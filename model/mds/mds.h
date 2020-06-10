#ifndef MDS_H
#define MDS_H

#include <base/gpstool.h>

int mds_init(void);

typedef enum
{
	MODEL_MDS_FAIL = -1,
	MODEL_MDS_SUCCESS = 0,
	MODEL_MDS_DONE = MODEL_MDS_SUCCESS,
	MODEL_MDS_SERVER_OK,
	MODEL_MDS_SERVER_NOK,
	MODEL_MDS_SERVER_ERR_AND_CHK,
}MODEL_MDS_RET;

//int model_mds_get_gpsdata(gpsData_t* pgpsdata);
void mds_load_and_set_config(void);
int model_mds_get_server_status();

void model_mds_send_poweroff_event_packet(gpsData_t gpsdata);
void model_mds_server_req(const unsigned char response_code);

#define SERVER_ERR_RETRY_SLEEP_TIME		(30 * 60)
#define SERVER_ERR_EXPIRE_TIME			(48 * 60 * 60)

#define DEFAULT_SETTING_SERVER_IP		"115.68.25.233"
#define DEFAULT_SETTING_SERVER_PORT		15082

#define DEFAULT_SETTING_COLLECT_INTERVAL_KEYON		1
#define DEFAULT_SETTING_COLLECT_INTERVAL_KEYOFF		600
#define DEFAULT_SETTING_REPORT_INTERVAL_KEYON		60
#define DEFAULT_SETTING_REPORT_INTERVAL_KEYOFF		600

#define DEFAULT_SETTING_SOCK_CONN_RETRY_CNT		2
#define DEFAULT_SETTING_SOCK_SEND_RETRY_CNT		3
#define DEFAULT_SETTING_SOCK_RCV_RETRY_CNT		3
#define DEFAULT_SETTING_SOCK_TIMEOUT_SEC		30

#define DEFAULT_SETTING_MODEL_NAME		"mds"

#define ONCE_SEND_PACKET_MAX_CNT		10

// 받은 패킷이 errror 패킷을경우 다시 바로 다시 시도하는 횟수
#define FAIL_PACKET_RETY_CNT		3

// --------------------------------------------------------------------
// send packet setting
// --------------------------------------------------------------------

// 총 send 를 몇번반복할것인가???
#define SEND_PACKET_RETRY_CNT		3

// 0 으로 리턴하면 패킷을 버린다.
// -1 로 리턴하면 패킷을 다시 뒤로 묶는다.
#define SEND_PACKET_RET				0

// 패킷 전송이 fail 날 경우 이시간 이후에 다시 시도한다.
#define SEND_FAIL_SLEEP_TIME		(60*3)	


#endif /* PACKET_H */
