#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include <base/config.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>

#include <logd_rpc.h>
#include <callback.h>

#include "config.h"
#include "alloc2_pkt.h"
#include "alloc2_senario.h"
#include "netcom.h"


#define MAX_SVR_RETRY_MDM_SETTING_VAL  3

int make_packet(char op, unsigned char **packet_buf, unsigned short *packet_len, const void *param)
{
	int res = 0;
	printf("------------- make packet :: pkt id [0x%x] start -------------------\r\n", op);
	switch (op)	{
		case e_mdm_setting_val :   // 0x01 : 단말 기본 설정 정보
		{
			res = make_pkt__mdm_setting_val(packet_buf, packet_len);
			break;
		}
    	case e_mdm_stat_evt :      // 0x02 : 단말 상태 정보 (이벤트)
		{
			LOGI(eSVC_MODEL, "[ALLOC2 NETCOMM] send evt code [%d]\r\n", *((int *)param));
			res = make_pkt__mdm_stat_evt(packet_buf, packet_len, *((int *)param));
			break;
		}
    	case e_mdm_gps_info :       // 0x03 : GPS 정보
		{
			res = make_pkt__mdm_gps_info(packet_buf, packet_len);
			break;
		}
    	case e_obd_dev_info :   // 0x11 : OBD 기본 설정 정보
		{
			res = make_pkt__obd_dev_info(packet_buf, packet_len);
			break;
		}
    	case e_obd_stat :      // 0x12 : OBD 상태 정보 (이벤트)
		{
			printf("???");
			res = make_pkt__obd_stat(packet_buf, packet_len, *((ALLOC_PKT_SEND__OBD_STAT_ARG *)param));
			break;
		}
    	case e_obd_data :          // 0x13 : OBD 수집 정보
		{
			res = make_pkt__obd_data(packet_buf, packet_len);
			break;
		}
    	case e_obd_chk_code :      // 0x14 : OBD 차량 진단코드
		{
			break;
		}
    	case e_dtg_setting_val :   // 0x21 : DTG 기본 정보
		{
			break;
		}
    	case e_dtg_data :          // 0x22 : DTG 수집 정보
		{
			break;
		}
    	case e_mdm_geofence_setting_val : // 0x31 : zone 설정 정보
		{
			break;
		}
    	case e_mdm_geofence_evt :  // 0x32 : zone 입출 정보
		{
			break;
		}
    	case e_bcm_stat_evt :      // 0x41 : All key 상태 정보 (이벤트)
		{
			break;
		}
    	case e_bcm_statting_val :  // 0x42 : All key 설정 정보
		{
			break;
		}
    	case e_bcm_mastercard_regi : // 0x45 : 마스터카드 등록
		{
			break;
		}
    	case e_bcm_reserv_val :    // 0x47 : 예약정보
		{
			break;
		}
    	case e_bcm_knocksensor_setting_val : // 0x51 : 노크센서 설정 정보
		{
			break;
		}
    	case e_firm_info : // 0x71 : 펌웨어 정보
		{
			break;
		}
    	case e_firm_update : // 0x72 : 펌웨어 업데이트
		{
			break;
		}
    	case e_firm_complete : // 0x79 : 펌웨어 업데이트 완료
		{
			break;
		}
    	case e_sms_recv_info : // 0xF0 : SMS 수신 정보
		{
			//LOGI(eSVC_MODEL, "[ALLOC2 NETCOMM] send sms recv code [%d]\r\n", *((int *)param));
			res = make_pkt__sms_recv_info(packet_buf, packet_len, *((ALLOC_PKT_SEND__SMS_PKT_ARG *)param));

			break;
		}
	}

	printf("------------- make packet :: pkt id [0x%x] end -------------------\r\n", op);
	return 0;
}

int send_packet(char op, unsigned char *packet_buf, int packet_len)
{
	transferSetting_t network_setting_info = {0,};

	int recv_buff_len = 0;
	int recv_ret = 0;

	int send_pkt_ret = 0;


	ALLOC_PKT_RECV__MDM_SETTING_VAL* p_mdm_setting_val = NULL;

	network_setting_info.retry_count_connect = 3;
	network_setting_info.retry_count_send = 3;
	network_setting_info.retry_count_receive = 3;
	network_setting_info.timeout_secs = 10;

	printf("------------- send packet :: pkt id [0x%x] start -------------------\r\n", op);

	p_mdm_setting_val = get_mdm_setting_val();

	if ( (p_mdm_setting_val != NULL ) && ( strlen(p_mdm_setting_val->proxy_server_ip) > 0 ) && (p_mdm_setting_val->proxy_server_ip > 0))
	{
		strcpy(network_setting_info.ip, p_mdm_setting_val->proxy_server_ip);
		network_setting_info.port = p_mdm_setting_val->proxy_server_port;
		LOGI(eSVC_MODEL, "GET server info - case 1 [%s]:[%d]\r\n", network_setting_info.ip, network_setting_info.port);
	}
	else
	{
		get_user_cfg_report_ip(network_setting_info.ip);
		get_user_cfg_report_port(&network_setting_info.port);
		LOGI(eSVC_MODEL, "GET server info - case 2 [%s]:[%d]\r\n", network_setting_info.ip, network_setting_info.port);
	}



	switch (op)
	{
		case e_mdm_setting_val :   // 0x01 : 단말 기본 설정 정보
		{
			ALLOC_PKT_RECV__MDM_SETTING_VAL recv_buff;
			static int fail_retry_cnt = 0;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);

			// printf("[ALLOC2 PKT TRANS] evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
			if (recv_ret == 0 )
			{
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				parse_pkt__mdm_setting_val(&recv_buff, network_setting_info.ip, network_setting_info.port);
				// printf("[ALLOC2 PKT TRANS] evtcode [%d] success!!!", e_mdm_setting_val);
				fail_retry_cnt = 0;
				send_pkt_ret = 0;
			}
			else
			{
				fail_retry_cnt++;
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				set_cur_status(e_SEND_TO_SETTING_INFO);
				send_pkt_ret = -1;
			}

			// 몇번 시도해보고 응답없으면 그냥 obd 형태로간다.
			if ( fail_retry_cnt > MAX_SVR_RETRY_MDM_SETTING_VAL )
				set_cur_status(e_SEND_TO_OBD_INFO);
				// 설정정보를 미수신시... 0x02 패킷을 보낸다음에 0x03 계속보냄
				// printf("[ALLOC2 PKT TRANS] evtcode [%d] error !!!", e_mdm_setting_val);

			break;
		}
    	case e_mdm_stat_evt :      // 0x02 : 단말 상태 정보 (이벤트)
		{
			ALLOC_PKT_RECV__MDM_STAT_EVT recv_buff;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);
			
			if (recv_ret == 0 )
			{
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				parse_pkt__mdm_stat_evt(&recv_buff);
				send_pkt_ret = 0;
			}
			else
			{
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				send_pkt_ret = -1;
			}
				

			break;
		}
    	case e_mdm_gps_info :       // 0x03 : GPS 정보
		{
			ALLOC_PKT_RECV__MDM_GPS_INFO recv_buff;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);
			
			printf("[ALLOC2 PKT TRANS] evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
			if (recv_ret == 0 )
			{
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				parse_pkt__mdm_gps_info(&recv_buff);
				send_pkt_ret = 0;
			}
			else
			{
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				send_pkt_ret = -1;
			}

			break;
		}
    	case e_obd_dev_info :   // 0x11 : OBD 기본 설정 정보
		{
			ALLOC_PKT_RECV__OBD_DEV_INFO recv_buff;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);
			
			if (recv_ret == 0 )
			{
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				parse_pkt__obd_dev_info(&recv_buff);
				send_pkt_ret = 0;
			}
			else
			{
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				send_pkt_ret = -1;
			}

			
			break;
		}
    	case e_obd_stat :      // 0x12 : OBD 상태 정보 (이벤트)
		{
			ALLOC_PKT_RECV__OBD_STAT recv_buff;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);
			
			if (recv_ret == 0 )
			{
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				parse_pkt__obd_stat(&recv_buff);
				send_pkt_ret = 0;
			}
			else
			{
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				send_pkt_ret = -1;
			}

			
			break;
		}
    	case e_obd_data :          // 0x13 : OBD 수집 정보
		{
			ALLOC_PKT_RECV__OBD_DATA recv_buff;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);
			
			if (recv_ret == 0 )
			{
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);

				parse_pkt__obd_data(&recv_buff);

				send_pkt_ret = 0;
			}
			else
			{
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				send_pkt_ret = -1;
			}

			
			break;
		}
    	case e_obd_chk_code :      // 0x14 : OBD 차량 진단코드
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_dtg_setting_val :   // 0x21 : DTG 기본 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_dtg_data :          // 0x22 : DTG 수집 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_mdm_geofence_setting_val : // 0x31 : zone 설정 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_mdm_geofence_evt :  // 0x32 : zone 입출 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_bcm_stat_evt :      // 0x41 : All key 상태 정보 (이벤트)
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_bcm_statting_val :  // 0x42 : All key 설정 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_bcm_mastercard_regi : // 0x45 : 마스터카드 등록
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_bcm_reserv_val :    // 0x47 : 예약정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_bcm_knocksensor_setting_val : // 0x51 : 노크센서 설정 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_firm_info : // 0x71 : 펌웨어 정보
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_firm_update : // 0x72 : 펌웨어 업데이트
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_firm_complete : // 0x79 : 펌웨어 업데이트 완료
		{
			send_pkt_ret = 0;
			break;
		}
    	case e_sms_recv_info : // 0xF0 : SMS 수신 정보
		{
			ALLOC_PKT_RECV__SMS_RECV_INFO recv_buff;

			recv_buff_len = sizeof(recv_buff);
			memset(&recv_buff, 0x00, recv_buff_len);
			
			recv_ret = transfer_packet_recv(&network_setting_info, packet_buf, packet_len, (unsigned char *)&recv_buff, recv_buff_len);
			
			if (recv_ret == 0 )
			{
				LOGI(eSVC_MODEL, "[ALLOC2 PKT TRANS] SUCCESS evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				//printf("recv -------------------------------------------------\r\n");
				//mds_api_debug_hexdump_buff(&recv_buff, recv_buff_len);
				//printf("-------------------------------------------------\r\n");
				parse_pkt__sms_recv_info(&recv_buff);
				send_pkt_ret = 0;
			}
			else
			{	
				LOGE(eSVC_MODEL, "[ALLOC2 PKT TRANS] ERROR evtcode [0x%x] recv_ret is [%d] \r\n", op, recv_ret);
				send_pkt_ret = -1;
			}
				

			break;
		}
	}

	printf("------------- send packet :: pkt id [0x%x] end -------------------\r\n", op);
	return send_pkt_ret;
}

int free_packet(void *packet)
{
	if(packet != NULL)
	{
		free(packet);
	}
	
	return 0;
}

