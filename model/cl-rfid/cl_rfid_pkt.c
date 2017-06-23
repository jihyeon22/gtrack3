#include <stdio.h>
#include <stdlib.h>

#include <at/at_util.h>
#include <jansson.h>
#include <logd_rpc.h>

#include "config.h"
#include "cl_rfid_passenger.h"

#define LOG_TARGET eSVC_COMMON

static int __make_http_header__req_passenger(char* buff, int version)
{
    // http://test.2bt.kr:8887/cd/getpassengerlist_tiny2.aspx?dn=01220003257&v=0
    // char *hdr_format = "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%d HTTP/1.1\r\nHost: test.2bt.kr:8887\r\n\r\n";

    configurationModel_t *conf = get_config_model();

	char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    int str_len = 0;

    char host_ip[64] ={0,};
    int host_port = 0;

    at_get_phonenum(phonenum, sizeof(phonenum));

    strncpy(host_ip, "test.2bt.kr", 40);
    host_port = 8887;

    str_len += sprintf(buff + str_len, "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%d HTTP/1.1\r\n", phonenum, version);
    str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
    str_len += sprintf(buff + str_len, "\r\n");

    return str_len;
}



int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len)
{
    char pkt_buff[1024] = {0,};
    unsigned char *packet_buf;

    int pkt_total_len = 0;

    __make_http_header__req_passenger(pkt_buff, 0);
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
    char* return_str = NULL;
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
        return -1;
    }
    
    // init passenger info
    user_info_init();
    
    json_start_p = strstr(buff, "\r\n\r\n");
    // [["61B849D9","0","1","0",""], ... ]

	root = json_loads(json_start_p, 0, &error);
	if(!root){
		LOGE(LOG_TARGET, "%s:%d> json_loads \n", __func__, __LINE__);
		LOGE(LOG_TARGET, "error : on line %d: %s\n", error.line, error.text);
		result = -20;
		return 0;
	}

    json_array_size_cnt = json_array_size(root);
    LOGI(LOG_TARGET, "json_array_size is [%d]\n", json_array_size_cnt);

    for( i = 0 ; i < json_array_size_cnt ; i ++ )
    {
        array = json_array_get(root, i);
        memset(&cur_user, 0x00, sizeof(RFID_USER_INFO_T));
        j = 0;
        printf("[%d]----------------------------------------\r\n",i );
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

        user_info_insert(cur_user);

        printf("----------------------------------------\r\n");
    }
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


}


