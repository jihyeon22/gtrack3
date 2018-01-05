#include <stdio.h>
#include <stdlib.h>

#include <base/devel.h>
#include <base/sender.h>
#include <at/at_util.h>

#include <jansson.h>
#include <logd_rpc.h>

#include "config.h"
#include "cl_rfid_pkt.h"
#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_COMMON

static int __make_http_header__req_passenger(char* buff, char* version)
{
    // http://test.2bt.kr:8887/cd/getpassengerlist_tiny2.aspx?dn=01220003257&v=0
    // char *hdr_format = "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%d HTTP/1.1\r\nHost: test.2bt.kr:8887\r\n\r\n";

    configurationModel_t *conf = get_config_model();
    int ver_int_date = 0;
    int ver_int_time = 0;

    char ver_str_date[16] = {0,};
    char ver_str_time[16] = {0,};

    char target_ver[32] = {0,};

    char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    int str_len = 0;

    char host_ip[60] = {0,};
    int  host_port = 0;

    int  ver_int_sec = 0;

    if ( version == NULL )
    {
        strcpy(target_ver,"0");
        kjtec_rfid_mgr__clr_all_user_data();
    }
    
    // 170807 202552 
    if ( strlen(version) == 12 )
    {
        strcpy(target_ver, version);
        #if 0
        strncpy(ver_str_date, version, 6);
        strncpy(ver_str_time, version + 6, 6);
        ver_int_date = atoi(ver_str_date);
        ver_int_time = atoi(ver_str_time);

        // �ð� ��Ʈ���� �ʷ� ��ȯ
        ver_int_sec += (ver_int_time % 100);
        // printf("ver_int_sec [%d] => [%d]\r\n", ver_int_sec, (ver_int_time % 100) );
        ver_int_sec += ( (ver_int_time / 100) % 100 )  * 60;
        // printf("ver_int_sec [%d] => [%d]\r\n", ver_int_sec, ( (ver_int_time / 100) % 100 ) );
        ver_int_sec += ( (ver_int_time / 10000) % 100 ) * 3600;
        // printf("ver_int_sec [%d] => [%d]\r\n", ver_int_sec, ( (ver_int_time / 10000) % 100 )  );

        // �ʷ� ��ȯ�� �����͸� ����ϰ�..
        ver_int_sec -= REQ_PASSENGER_DATE_MARGIN_SEC;

        // ����� �ʵ����͸� �ٽ� ���ڿ��� �����.
        ver_int_time = ( ver_int_sec % 60 );
        //printf("ver_int_time [%d] => [%d]\r\n", ver_int_time, ( ver_int_sec % 60 ) );
        ver_int_time += (( ver_int_sec / 60 ) % 60) * 100;
        //printf("ver_int_time [%d] => [%d]\r\n", ver_int_time,  (( ver_int_sec / 60 ) % 60) * 100);
        ver_int_time += (( ver_int_sec / 3600 ) ) * 10000;
        //printf("ver_int_time [%d] => [%d]\r\n", ver_int_time,   (( ver_int_sec / 3600 ) ) * 10000);
        
        sprintf(target_ver, "%06d%06d", ver_int_date, ver_int_time);

        printf("ver_str_date [%s] / ver_str_time [%s] / ver_int_date [%d] / ver_int_time [%d]\r\n", ver_str_date, ver_str_time, ver_int_date, ver_int_time);
        #endif
    }
    else
    {
        strcpy(target_ver,"0");
        kjtec_rfid_mgr__clr_all_user_data();
    }

    devel_webdm_send_log("RIFD TARGET VER [%s]\r\n", target_ver);

	at_get_phonenum(phonenum, sizeof(phonenum));

    //strcpy(phonenum, "01220003257"); // http://test.2bt.kr:8887 테스트용 
    
    strcpy(host_ip, conf->model.request_rfid);
    host_port = conf->model.request_rfid_port;

    // change tiny2 -> tiny3 : 20180821 : cl requre 
    str_len += sprintf(buff + str_len, "GET /cd/getpassengerlist_tiny3.aspx?dn=%s&v=%s HTTP/1.1\r\n", phonenum, target_ver);
    //str_len += sprintf(buff + str_len, "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%s HTTP/1.1\r\n", phonenum, target_ver);
    str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
    str_len += sprintf(buff + str_len, "\r\n");

    return str_len;
}



int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len, char* version)
{
    char pkt_buff[1024] = {0,};
    unsigned char *packet_buf;

    int pkt_total_len = 0;

    __make_http_header__req_passenger(pkt_buff, version);
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

#define MAX_CLRFID_PKT_PARSE_FAIL   2
static int _g_parse_fail_cnt = 0;
int clear_req_passenger_fail_cnt()
{
    _g_parse_fail_cnt = 0;
}

int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff)
{
    int result;
    int i = 0;
    int j = 0;

    int passenger_list_add = 0;
    int passenger_list_del = 0;

    char* json_start_p = NULL;
    const char* return_str = NULL;
    json_t *array;
    json_t *element;
	json_t *root = NULL;
	json_error_t error;

    int json_array_size_cnt = 0;

    RFID_USER_INFO_T cur_user;
    
    memset(&cur_user, 0x00, sizeof(RFID_USER_INFO_T));

    if ( buff == NULL )
        return -1;
    
    if ( strstr(buff, "HTTP/1.1 200 OK") == NULL )
    {
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt -1 [%d]", _g_parse_fail_cnt);
        if ( _g_parse_fail_cnt++ > MAX_CLRFID_PKT_PARSE_FAIL )
        {
            //parse_fail_cnt = 0;
            return 0;
        }
        return -1;
    }

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");

    
    if ( strncmp("HTTP/1.1 200 OK", buff, strlen("HTTP/1.1 200 OK")  ) )
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt -2 [%d]", _g_parse_fail_cnt);
        if ( _g_parse_fail_cnt++ > MAX_CLRFID_PKT_PARSE_FAIL )
        {
            //parse_fail_cnt = 0;
            return 0;
        }
        return -1;
    }
    

    json_start_p = strstr(buff, "\r\n\r\n");

    if ( json_start_p == NULL ) 
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail 2 [%d]\r\n", __func__, __LINE__, _g_parse_fail_cnt );
        devel_webdm_send_log("DOWNLOAD USER : server ret fail 2 [%d]", _g_parse_fail_cnt);
        if ( _g_parse_fail_cnt++ > MAX_CLRFID_PKT_PARSE_FAIL )
        {
            //parse_fail_cnt = 0;
            return 0;
        }
        return -1;
    }

    // init passenger info
    rfid_tool__user_info_init();
    
    json_start_p = strstr(buff, "\r\n\r\n");
    // [["61B849D9","0","1","0",""], ... ]

    
	root = json_loads(json_start_p, 0, &error);
	if(!root){
		LOGE(LOG_TARGET, "%s:%d> json_loads \n", __func__, __LINE__);
		LOGE(LOG_TARGET, "error : on line %d: %s\n", error.line, error.text);
		result = -20;
        devel_webdm_send_log("DOWNLOAD USER : invaild json [%s] [%d]", error.text, _g_parse_fail_cnt);
        _g_parse_fail_cnt = 0;
		return 0;
	}

    json_array_size_cnt = json_array_size(root);
    LOGI(LOG_TARGET, "json_array_size is [%d]\n", json_array_size_cnt);

    for( i = 0 ; i < json_array_size_cnt ; i ++ )
    {
        array = json_array_get(root, i);
        memset(&cur_user, 0x00, sizeof(RFID_USER_INFO_T));
        j = 0;
        //printf("[%d]----------------------------------------\r\n",i );
        json_array_foreach(array, j, element){
            switch(j)
            {
                case 0:
                {
                    return_str = json_string_value(element);
                    if ( return_str == NULL)
                    {
                        devel_webdm_send_log("json Parse fail - 0");
                        return -1;
                    }
                    strcpy(cur_user.rfid_uid, return_str);
                    break;
                }
                case 1:
                {
                    return_str = json_string_value(element);
                    if ( return_str == NULL)
                    {
                        devel_webdm_send_log("json Parse fail - 1");
                        return -1;
                    }
                    cur_user.day_limit = atoi(return_str);
                    break;
                }
                case 2:
                {
                    return_str = json_string_value(element);
                    if ( return_str == NULL)
                    {
                        devel_webdm_send_log("json Parse fail - 2");
                        return -1;
                    }
                    cur_user.is_use = atoi(return_str);
                    if ( cur_user.is_use == 1)
                        passenger_list_add++;
                    else
                        passenger_list_del++;
                    break;
                }
                case 3:
                {
                    return_str = json_string_value(element);
                    if ( return_str == NULL)
                    {
                        devel_webdm_send_log("json Parse fail - 3");
                        return -1;
                    }
                    cur_user.boarding_cont = atoi(return_str);
                    break;
                }
                case 4:
                {
                    return_str = json_string_value(element);
                    if ( return_str == NULL)
                    {
                        devel_webdm_send_log("json Parse fail - 4");
                        return -1;
                    }
                    strcpy(cur_user.last_boarding_date, return_str);
                    break;
                }
            }
        }

        if ( rfid_tool__user_info_insert(cur_user) != 0 )
        {
            devel_webdm_send_log("DOWNLOAD USER : FAIL OVERFLOW CNT \r\n");
            kjtec_rfid_mgr__clr_all_user_data();
            return -1;
        }

       // printf("----------------------------------------\r\n");
    }

    //if ( ( passenger_list_add < 1000 ) && (passenger_list_del > 10000) )
    if ( (passenger_list_del > 10000) )
    {
        printf( "DOWNLOAD USER INFO SUCCESS : CNT [%d] - ADD [%d] DEL [%d] reset case 1\r\n", rfid_tool__user_info_total_cnt(), passenger_list_add, passenger_list_del);
        LOGI(LOG_TARGET, "DOWNLOAD USER INFO SUCCESS : CNT [%d] - ADD [%d] DEL [%d] reset case 1\r\n", rfid_tool__user_info_total_cnt(), passenger_list_add, passenger_list_del);
        devel_webdm_send_log("DOWNLOAD USER INFO SUCCESS : CNT [%d] - ADD [%d] DEL [%d] reset case 1\r\n", rfid_tool__user_info_total_cnt(), passenger_list_add, passenger_list_del);
        kjtec_rfid_mgr__clr_all_user_data();
        return -1;
    }

    printf( "DOWNLOAD USER INFO SUCCESS : CNT [%d] - ADD [%d] DEL [%d]\r\n", rfid_tool__user_info_total_cnt(), passenger_list_add, passenger_list_del);
    LOGI(LOG_TARGET, "DOWNLOAD USER INFO SUCCESS : CNT [%d] - ADD [%d] DEL [%d]\r\n", rfid_tool__user_info_total_cnt(), passenger_list_add, passenger_list_del);
    devel_webdm_send_log("DOWNLOAD USER INFO SUCCESS : CNT [%d] - ADD [%d] DEL [%d]\r\n", rfid_tool__user_info_total_cnt(), passenger_list_add, passenger_list_del);
    _g_parse_fail_cnt = 0;

    

/*
    while(1)
    {
        if ( json_unpack(root, "[s,i,i,i,s]", user_info.rfid_uid, &user_info.day_limit , &user_info.is_use , &user_info.boarding_cont, user_info.last_boarding_date) )
        {
            LOGE(LOG_TARGET, "%s:%d> Error json_unpack \n", __func__, __LINE__);
            break;
        }
        else
        {

        }
    }
*/
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

    blist_str_len += sprintf(blist_str + blist_str_len, "{");

    for ( i = 0 ; i < boarding->rfid_boarding_idx ; i ++ )
    {
        blist_str_len += sprintf(blist_str + blist_str_len, 
                                "%d:[\"%s\",\"%d\",\"%s\",\"%d\"]"
                                , i
                                , boarding->boarding_info[i].rfid_uid
                                , boarding->boarding_info[i].boarding
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

    str_len += sprintf(buff + str_len, "GET /cd/setboardinglist.aspx?dn=%s&br=%s HTTP/1.1\r\n", phonenum, blist_str);
    str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
    str_len += sprintf(buff + str_len, "\r\n");

    return str_len;
}



int make_clrfid_pkt__set_boarding(unsigned char **pbuf, unsigned short *packet_len, RFID_BOARDING_MGR_T* boarding)
{
    char pkt_buff[1024] = {0,};
    unsigned char *packet_buf;

    int pkt_total_len = 0;
    printf("%s() -> [%d]\r\n", __func__, __LINE__);

    __make_http_header__set_boarding(pkt_buff, boarding);
    printf("%s() -> [%d]\r\n", __func__, __LINE__);
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
    

    if ( strstr(buff, "HTTP/1.1 200 OK") == NULL )
    {
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt -22 [%d]", _g_parse_fail_cnt);
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