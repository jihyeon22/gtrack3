#include <stdio.h>
#include <stdlib.h>

#include <base/devel.h>
#include <base/sender.h>
#include <at/at_util.h>

#include <jansson.h>
#include <logd_rpc.h>

#include "config.h"
#include "ext/rfid/cl_rfid_pkt.h"
#include "ext/rfid/cl_rfid_tools.h"

#include "ext/rfid/cl_rfid_tools.h"
#include "ext/rfid/cust1_rfid/cust1_rfid_pkt.h"


#include "ext/rfid/cust1_rfid/cust1_rfid_pkt.h"

#define LOG_TARGET eSVC_COMMON

int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len, char* version)
{
    // empty
    return 0;
}

int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff)
{
    // empty
    return 0;

}

static int __make_http_header__set_boarding(char* buff, RFID_BOARDING_MGR_T* boarding )
{
    // http://test.2bt.kr:8887/cd/getpassengerlist_tiny2.aspx?dn=01220003257&v=0
    // char *hdr_format = "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%d HTTP/1.1\r\nHost: test.2bt.kr:8887\r\n\r\n";

    configurationModel_t *conf = get_config_model();

	char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    int str_len = 0;

    char host_ip[64] ={0,};
    int host_port = 0;

    char blist_str[1024] ={0,};
    int blist_str_len = 0;

    int i = 0;

    // 호출 url 타입을 변경시킨다 : 
    //  + 기존 호출 url 방식
    if ( conf->model.request_rfid_type  == 0 )
    {
        blist_str_len += sprintf(blist_str + blist_str_len, "{");

        for ( i = 0 ; i < boarding->rfid_boarding_idx ; i ++ )
        {
            blist_str_len += sprintf(blist_str + blist_str_len, 
                                    "%d:[\"%s\",\"%d\",\"%s\",\"%d\"]"
                                    , i
                                    , boarding->boarding_info[i].rfid_uid
                                    , 1
                                    , boarding->boarding_info[i].date
                                    , boarding->boarding_info[i].chk_result );

            if ( boarding->rfid_boarding_idx - i > 1 )
                blist_str_len += sprintf(blist_str + blist_str_len, ",");
        }
        blist_str_len += sprintf(blist_str + blist_str_len, "}");
        at_get_phonenum(phonenum, sizeof(phonenum));

    //    strcpy(phonenum, "01222591191");
        strcpy(host_ip, conf->model.request_rfid);
        host_port = conf->model.request_rfid_port;

        str_len += sprintf(buff + str_len, "GET %s?dn=%s&br=%s HTTP/1.1\r\n", conf->model.request_rfid_api_0, phonenum, blist_str);
        str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
        str_len += sprintf(buff + str_len, "\r\n");

    }
    //  + 새로운 스펙서 변경사항적용
    else if ( conf->model.request_rfid_type  == 1 )
    {
        blist_str_len += sprintf(blist_str + blist_str_len, "[");

        for ( i = 0 ; i < boarding->rfid_boarding_idx ; i ++ )
        {
            blist_str_len += sprintf(blist_str + blist_str_len, "%s", boarding->boarding_info[i].rfid_uid);

            if ( boarding->rfid_boarding_idx - i > 1 )
                blist_str_len += sprintf(blist_str + blist_str_len, ",");
        }
        blist_str_len += sprintf(blist_str + blist_str_len, "]");
        at_get_phonenum(phonenum, sizeof(phonenum));

    //    strcpy(phonenum, "01222591191");
        strcpy(host_ip, conf->model.request_rfid);
        host_port = conf->model.request_rfid_port;

        str_len += sprintf(buff + str_len, "GET %s?dn=%s&br=%s HTTP/1.1\r\n", conf->model.request_rfid_api_0, phonenum, blist_str);
        str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
        str_len += sprintf(buff + str_len, "\r\n");
    }

    return str_len;
}



int make_clrfid_pkt__set_boarding(unsigned char **pbuf, unsigned short *packet_len, RFID_BOARDING_MGR_T* boarding)
{
    char pkt_buff[1024] = {0,};
    unsigned char *packet_buf;

    int pkt_total_len = 0;
    //printf("%s() -> [%d]\r\n", __func__, __LINE__);


    __make_http_header__set_boarding(pkt_buff, boarding);


    //printf("%s() -> [%d]\r\n", __func__, __LINE__);

    pkt_total_len = strlen(pkt_buff);

     *packet_len = pkt_total_len;
    packet_buf = (unsigned char *)malloc(*packet_len);
    
    if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

    memset(packet_buf, 0, pkt_total_len);
	*pbuf = packet_buf;

    memcpy(packet_buf, &pkt_buff, pkt_total_len);

    printf("%s() => dump pkt -------------------------------------------------\r\n", __func__);
    printf("%s\r\n", pkt_buff);
    printf("-------------------------------------------------\r\n");

    return 0;

}


int parse_clrfid_pkt__set_boarding(unsigned char * buff, int len_buff)
{
    char* ret_str_p = NULL;
    static int _g_parse_fail_cnt2 = 0;

    if ( buff == NULL ) 
        return -1;
    
    // success
    if ( strstr(buff, "HTTP/1.1 200 OK") == NULL )
    {
        devel_webdm_send_log("RFID CUST 1 SET BOARDING : server ret fail cnt -22 [%d]", get_req_passenger_fail_cnt());
        if ( _g_parse_fail_cnt2++ > MAX_CLRFID_PKT_PARSE_FAIL )
        {
            //parse_fail_cnt = 0;
            return 0;
        }
        return -1;
    }
    LOGI(LOG_TARGET, "SET BOARDING SERVER RET [%s]\r\n", buff);

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");

    return 1;

}

// empty