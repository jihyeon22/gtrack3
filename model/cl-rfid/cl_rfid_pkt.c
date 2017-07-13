#include <stdio.h>
#include <stdlib.h>

#include <base/devel.h>
#include <base/sender.h>
#include <at/at_util.h>

#include <jansson.h>
#include <logd_rpc.h>

#include "config.h"
#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_COMMON

static int __make_http_header__req_passenger(char* buff, char* version)
{
    // http://test.2bt.kr:8887/cd/getpassengerlist_tiny2.aspx?dn=01220003257&v=0
    // char *hdr_format = "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%d HTTP/1.1\r\nHost: test.2bt.kr:8887\r\n\r\n";

    configurationModel_t *conf = get_config_model();

	char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    int str_len = 0;

    char host_ip[60] = {0,};
    int  host_port = 0;

    at_get_phonenum(phonenum, sizeof(phonenum));

    //strcpy(phonenum, "01222591191");
    strcpy(host_ip, conf->model.request_rfid);
    host_port = conf->model.request_rfid_port;

    str_len += sprintf(buff + str_len, "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%s HTTP/1.1\r\n", phonenum, version);
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

int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff)
{
    int result;
    int i = 0;
    int j = 0;

    char* json_start_p = NULL;
    const char* return_str = NULL;
    json_t *array;
    json_t *element;
	json_t *root = NULL;
	json_error_t error;

    int json_array_size_cnt = 0;

    RFID_USER_INFO_T cur_user;
    memset(&cur_user, 0x00, sizeof(RFID_USER_INFO_T));

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");


    if ( strncmp("HTTP/1.1 200 OK", buff, strlen("HTTP/1.1 200 OK")  ) )
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        devel_webdm_send_log("DOWNLOAD USER : server ret fail");
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
        devel_webdm_send_log("DOWNLOAD USER : invaild json [%s]", error.text);
		return -1;
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
                    strcpy(cur_user.rfid_uid, return_str);
                    break;
                }
                case 1:
                {
                    return_str = json_string_value(element);
                    cur_user.day_limit = atoi(return_str);
                    break;
                }
                case 2:
                {
                    return_str = json_string_value(element);
                    cur_user.is_use = atoi(return_str);
                    break;
                }
                case 3:
                {
                    return_str = json_string_value(element);
                    cur_user.boarding_cont = atoi(return_str);
                    break;
                }
                case 4:
                {
                    return_str = json_string_value(element);
                    strcpy(cur_user.last_boarding_date, return_str);
                    break;
                }
            }
        }

        rfid_tool__user_info_insert(cur_user);

       // printf("----------------------------------------\r\n");
    }

    printf( "DOWNLOAD USER INFO SUCCESS : CNT [%d]\r\n", rfid_tool__user_info_total_cnt());
    LOGI(LOG_TARGET, "DOWNLOAD USER INFO SUCCESS : CNT [%d]\r\n", rfid_tool__user_info_total_cnt());
    devel_webdm_send_log("DOWNLOAD USER INFO SUCCESS : CNT [%d]", rfid_tool__user_info_total_cnt());
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

    char blist_str[512] ={0,};
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
    
    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");

    LOGI(LOG_TARGET, "SET BOARDING SERVER RET [%s]\r\n", buff);
    return 0;

    
    if ( strncmp("HTTP/1.1 200 OK", buff, strlen("HTTP/1.1 200 OK")  ) )
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        return -1;
    }
    
    ret_str_p = strstr(buff, "\r\n\r\n");
    if ( ret_str_p != NULL )
        printf(" return val is [%d]\r\n", atoi(ret_str_p));
    //json_start_p = buff;
    return 1;

}