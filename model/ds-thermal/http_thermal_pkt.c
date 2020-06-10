<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/devel.h>
#include <base/sender.h>
#include <at/at_util.h>

#include <jansson.h>
#include <logd_rpc.h>

#include <board/modem-time.h>

#include "config.h"
#include "http_thermal_pkt.h"

#include <base/thermtool.h>

#define LOG_TARGET eSVC_COMMON

static int _g_parse_fail_cnt = 0;
int _thermal_sensor_val_filter(short val)
{
	short convert_val = val;

	if ( val >= THERMAL_SENSOR_FILTER__MAX )
		convert_val =THERMAL_SENSOR_FILTER__MAX;

	if ( val <= THERMAL_SENSOR_FILTER__MIN)
		convert_val = THERMAL_SENSOR_FILTER__MIN;

    return convert_val;
}

void ds_get_thermal_val(thermaldata_t* sensor_val)
{
	THERMORMETER_DATA tmp_therm;
//	short thermal_sensor[4] = {0,};	// max 4 ch.
	int thermal_sensor_val[4] = {0,};

	int idx = 0;
	int i = 0;

//	char tmp_thermal_sensor_str[6+1] = {0};	// 6byte : each ch size 3byte : total 2byte.

	for( i = 0 ; i < 4 ; i ++)
	{
		thermal_sensor_val[i] = THERMAL_SENSOR_VAL__INVALID;
	}

    idx = 0;
    
	if( therm_get_curr_data(&tmp_therm) == 0)
	{
		int i = 0;
		int tmp_val = 0;

		for(i=0 ; i < tmp_therm.channel; i++)
		{
			switch(tmp_therm.temper[i].status)
			{
				case eOK:
					LOGT(LOG_TARGET, "[THERMAL] sensor [%d] => [%d]\n", idx, tmp_therm.temper[i].data);
					thermal_sensor_val[i] = _thermal_sensor_val_filter(tmp_therm.temper[i].data);
					break;
				case eOPEN:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
					break;
				case eSHORT:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
                    break;
				case eUNUSED:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
                    break;
				case eNOK:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
                    break;
				default:
					break;
			}

		}
	}

    sensor_val->thermal_val_1 = thermal_sensor_val[0];
    sensor_val->thermal_val_2 = thermal_sensor_val[1];
}

static int __make_http_header__send_thermal_val(char* buff, int thermal_val_1, int thermal_val_2)
{

    // http://1.229.253.27:8090/device/TempCollection.do?Conn=LGU&TidId=XXXXXX&SDate=20170914&STime=101000&DegreeA=-10&DegreeB=20

    configurationModel_t *conf = get_config_model();

    char cur_str_date[8+1] = {0,};
    char cur_str_time[6+1] = {0,};
    char comm_provider[16] = {0,};

    char phonenum[AT_LEN_PHONENUM_BUFF] = {0};

    int str_len = 0;

    char host_ip[60] = {0,};
    int  host_port = 0;

	struct tm cur_time;
    time_t system_time;
    struct tm *timeinfo;

    // devel_webdm_send_log("RIFD TARGET VER [%s]\r\n", target_ver);
    // ---------------------------------------------------------------------
    #if defined (BOARD_TL500S) 
        strcpy(comm_provider, "SKT");
    #endif

    #if defined (BOARD_TL500K) 
        strcpy(comm_provider, "KT");
    #endif
    
    #if defined (BOARD_TL500L) 
        strcpy(comm_provider, "LGU");
    #endif

    // ---------------------------------------------------------------------
	at_get_phonenum(phonenum, sizeof(phonenum));
    //strcpy(phonenum, "01211112222"); // for test 

    // ----------------------------------------------------------
    
	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
    }
    
    snprintf(cur_str_date, 8+1, "%04d%02d%02d", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday);
    snprintf(cur_str_time, 6+1, "%02d%02d%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    strcpy(host_ip, conf->model.request_http_ip);
    host_port = conf->model.request_http_port;

    // change tiny2 -> tiny3 : 20180821 : cl requre 
    // /device/TempCollection.do?Conn=LGU&TidId=XXXXXX&SDate=20170914&STime=101000&DegreeA=-10&DegreeB=20
    str_len += sprintf(buff + str_len, "GET /device/TempCollection.do?Conn=%s&TidId=%s&SDate=%s&STime=%s&DegreeA=%d&DegreeB=%d HTTP/1.1\r\n", 
                                        comm_provider, phonenum,
                                        cur_str_date, cur_str_time,
                                        thermal_val_1/10, thermal_val_2/10);
    //str_len += sprintf(buff + str_len, "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%s HTTP/1.1\r\n", phonenum, target_ver);
    str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
    str_len += sprintf(buff + str_len, "\r\n");

    return str_len;
}



int make_http_thermal_pkt__send_themal_val(unsigned char **pbuf, unsigned short *packet_len, thermaldata_t * param)
{
    char pkt_buff[1024] = {0,};
    unsigned char *packet_buf;

    int pkt_total_len = 0;

    __make_http_header__send_thermal_val(pkt_buff, param->thermal_val_1, param->thermal_val_2);
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


int parse_http_thermal_pkt__send_themal_val(unsigned char * buff, int len_buff)
{
    int result;
    int i = 0;
    int j = 0;

    char* success_str = NULL;
    const char* return_str = NULL;


    int json_array_size_cnt = 0;

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");

    success_str = strstr(buff, "{\"returnValue\":\"SUCCESS\"}");
    
    if ( success_str == NULL ) 
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt [%d]", _g_parse_fail_cnt);
        if ( _g_parse_fail_cnt++ > MAX_HTTP_THERMAL_PKT_PARSE_FAIL )
        {
            //parse_fail_cnt = 0;
            return 0;
        }
        return -1;
    }
    else
    {
        LOGI(LOG_TARGET, "%s:%d> http return success \r\n", __func__, __LINE__);
    }
    /*
    parse_http_thermal_pkt__send_themal_val() [-4]=> dump pkt -------------------------------------------------
    HTTP/1.1 200 OK
    Content-Language: ko-KR
    Date: Wed, 27 Sep 2017 06:27:17 GMT
    Expires: Thu, 01 Jan 1970 00:00:00 GMT
    Content-Type: application/json;charset=UTF-8
    Cache-Control: no-cache, no-store, max-age=0
    Pragma: no-cache
    Transfer-Encoding: chunked

    19
    {"returnValue":"SUCCESS"}
    0

    */

#if 0
    json_t *array;
    json_t *element;
	json_t *root = NULL;
	json_error_t error;
    memset(&cur_user, 0x00, sizeof(RFID_USER_INFO_T));

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");


    if ( strncmp("HTTP/1.1 200 OK", buff, strlen("HTTP/1.1 200 OK")  ) )
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt [%d]", _g_parse_fail_cnt);
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
                    if ( cur_user.is_use == 1)
                        passenger_list_add++;
                    else
                        passenger_list_del++;
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
#endif
    return 0;

}
=======
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/devel.h>
#include <base/sender.h>
#include <at/at_util.h>

#include <jansson.h>
#include <logd_rpc.h>

#include <board/modem-time.h>

#include "config.h"
#include "http_thermal_pkt.h"

#include <base/thermtool.h>

#define LOG_TARGET eSVC_COMMON

static int _g_parse_fail_cnt = 0;
int _thermal_sensor_val_filter(short val)
{
	short convert_val = val;

	if ( val >= THERMAL_SENSOR_FILTER__MAX )
		convert_val =THERMAL_SENSOR_FILTER__MAX;

	if ( val <= THERMAL_SENSOR_FILTER__MIN)
		convert_val = THERMAL_SENSOR_FILTER__MIN;

    return convert_val;
}

void ds_get_thermal_val(thermaldata_t* sensor_val)
{
	THERMORMETER_DATA tmp_therm;
//	short thermal_sensor[4] = {0,};	// max 4 ch.
	int thermal_sensor_val[4] = {0,};

	int idx = 0;
	int i = 0;

//	char tmp_thermal_sensor_str[6+1] = {0};	// 6byte : each ch size 3byte : total 2byte.

	for( i = 0 ; i < 4 ; i ++)
	{
		thermal_sensor_val[i] = THERMAL_SENSOR_VAL__INVALID;
	}

    idx = 0;
    
	if( therm_get_curr_data(&tmp_therm) == 0)
	{
		int i = 0;
		int tmp_val = 0;

		for(i=0 ; i < tmp_therm.channel; i++)
		{
			switch(tmp_therm.temper[i].status)
			{
				case eOK:
					LOGT(LOG_TARGET, "[THERMAL] sensor [%d] => [%d]\n", idx, tmp_therm.temper[i].data);
					thermal_sensor_val[i] = _thermal_sensor_val_filter(tmp_therm.temper[i].data);
					break;
				case eOPEN:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
					break;
				case eSHORT:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
                    break;
				case eUNUSED:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
                    break;
				case eNOK:
                    thermal_sensor_val[i] =  THERMAL_SENSOR_VAL__INVALID;
                    break;
				default:
					break;
			}

		}
	}

    sensor_val->thermal_val_1 = thermal_sensor_val[0];
    sensor_val->thermal_val_2 = thermal_sensor_val[1];
}

static int __make_http_header__send_thermal_val(char* buff, int thermal_val_1, int thermal_val_2)
{

    // http://1.229.253.27:8090/device/TempCollection.do?Conn=LGU&TidId=XXXXXX&SDate=20170914&STime=101000&DegreeA=-10&DegreeB=20

    configurationModel_t *conf = get_config_model();

    char cur_str_date[8+1] = {0,};
    char cur_str_time[6+1] = {0,};
    char comm_provider[16] = {0,};

    char phonenum[AT_LEN_PHONENUM_BUFF] = {0};

    int str_len = 0;

    char host_ip[60] = {0,};
    int  host_port = 0;

	struct tm cur_time;
    time_t system_time;
    struct tm *timeinfo;

    // devel_webdm_send_log("RIFD TARGET VER [%s]\r\n", target_ver);
    // ---------------------------------------------------------------------
    #if defined (BOARD_TL500S) 
        strcpy(comm_provider, "SKT");
    #endif

    #if defined (BOARD_TL500K) 
        strcpy(comm_provider, "KT");
    #endif
    
    #if defined (BOARD_TL500L) 
        strcpy(comm_provider, "LGU");
    #endif

    // ---------------------------------------------------------------------
	at_get_phonenum(phonenum, sizeof(phonenum));
    //strcpy(phonenum, "01211112222"); // for test 

    // ----------------------------------------------------------
    
	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
    }
    
    snprintf(cur_str_date, 8+1, "%04d%02d%02d", timeinfo->tm_year+1900, timeinfo->tm_mon+1, timeinfo->tm_mday);
    snprintf(cur_str_time, 6+1, "%02d%02d%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    strcpy(host_ip, conf->model.request_http_ip);
    host_port = conf->model.request_http_port;

    // change tiny2 -> tiny3 : 20180821 : cl requre 
    // /device/TempCollection.do?Conn=LGU&TidId=XXXXXX&SDate=20170914&STime=101000&DegreeA=-10&DegreeB=20
    str_len += sprintf(buff + str_len, "GET /device/TempCollection.do?Conn=%s&TidId=%s&SDate=%s&STime=%s&DegreeA=%d&DegreeB=%d HTTP/1.1\r\n", 
                                        comm_provider, phonenum,
                                        cur_str_date, cur_str_time,
                                        thermal_val_1/10, thermal_val_2/10);
    //str_len += sprintf(buff + str_len, "GET /cd/getpassengerlist_tiny2.aspx?dn=%s&v=%s HTTP/1.1\r\n", phonenum, target_ver);
    str_len += sprintf(buff + str_len, "Host: %s:%d\r\n",host_ip, host_port);
    str_len += sprintf(buff + str_len, "\r\n");

    return str_len;
}



int make_http_thermal_pkt__send_themal_val(unsigned char **pbuf, unsigned short *packet_len, thermaldata_t * param)
{
    char pkt_buff[1024] = {0,};
    unsigned char *packet_buf;

    int pkt_total_len = 0;

    __make_http_header__send_thermal_val(pkt_buff, param->thermal_val_1, param->thermal_val_2);
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


int parse_http_thermal_pkt__send_themal_val(unsigned char * buff, int len_buff)
{
    int result;
    int i = 0;
    int j = 0;

    char* success_str = NULL;
    const char* return_str = NULL;


    int json_array_size_cnt = 0;

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");

    success_str = strstr(buff, "{\"returnValue\":\"SUCCESS\"}");
    
    if ( success_str == NULL ) 
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt [%d]", _g_parse_fail_cnt);
        if ( _g_parse_fail_cnt++ > MAX_HTTP_THERMAL_PKT_PARSE_FAIL )
        {
            //parse_fail_cnt = 0;
            return 0;
        }
        return -1;
    }
    else
    {
        LOGI(LOG_TARGET, "%s:%d> http return success \r\n", __func__, __LINE__);
    }
    /*
    parse_http_thermal_pkt__send_themal_val() [-4]=> dump pkt -------------------------------------------------
    HTTP/1.1 200 OK
    Content-Language: ko-KR
    Date: Wed, 27 Sep 2017 06:27:17 GMT
    Expires: Thu, 01 Jan 1970 00:00:00 GMT
    Content-Type: application/json;charset=UTF-8
    Cache-Control: no-cache, no-store, max-age=0
    Pragma: no-cache
    Transfer-Encoding: chunked

    19
    {"returnValue":"SUCCESS"}
    0

    */

#if 0
    json_t *array;
    json_t *element;
	json_t *root = NULL;
	json_error_t error;
    memset(&cur_user, 0x00, sizeof(RFID_USER_INFO_T));

    printf("%s() [%d]=> dump pkt -------------------------------------------------\r\n", __func__, len_buff);
    printf("%s\r\n", buff);
    printf("-------------------------------------------------\r\n");


    if ( strncmp("HTTP/1.1 200 OK", buff, strlen("HTTP/1.1 200 OK")  ) )
    {
        LOGE(LOG_TARGET, "%s:%d> http return fail \r\n", __func__, __LINE__);
        devel_webdm_send_log("DOWNLOAD USER : server ret fail cnt [%d]", _g_parse_fail_cnt);
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
                    if ( cur_user.is_use == 1)
                        passenger_list_add++;
                    else
                        passenger_list_del++;
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
#endif
    return 0;

}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
