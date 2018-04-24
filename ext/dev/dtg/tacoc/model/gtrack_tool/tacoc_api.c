#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <base/gpstool.h>

#include <wrapper/dtg_atcmd.h>

/* Insert Common Client Header */
#include <wrapper/dtg_log.h>
// #include "taco_rpc.h"
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>

#include <wrapper/dtg_taco_wrapper_rpc_clnt.h>

#include <tacom/tacom_inc.h>
#include <standard_protocol.h>


static pthread_mutex_t dtg_gtrack_tool_mutex = PTHREAD_MUTEX_INITIALIZER;


void taco_gtrack_tool__mutex_lock()
{
    pthread_mutex_lock(&dtg_gtrack_tool_mutex);
}

void taco_gtrack_tool__mutex_unlock()
{
    pthread_mutex_unlock(&dtg_gtrack_tool_mutex);
}

// ----------------------------------------------------------------------------------
// tacoc API ..
// ----------------------------------------------------------------------------------

int send_cmd_current_data()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    return 0;
}


//extern CLIENT *clnt_update;

void device_reset()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
	mdmc_api_reset_wrapper();
}

int network_connect()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
	return mdmc_api_net_conn_wrapper();
}

int net_time_sync()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
	return mdmc_api_time_sync_wrapper();
}

int data_req_to_taco()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
	return taco_request_call_wrapper();
}

void mdmc_power_off()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
}

int main_process()
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    return 0;
}

int breakdown_report(int integer)
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);

	if(integer == 0)
	{
		printf("breakdw msg: dtg problem occur\n");
        //devel_webdm_send_log("breakdw msg: dtg problem occur");
	}
	else if(integer == 1)
	{	
		printf("breakdw msg: dtg working\n");
        //devel_webdm_send_log("breakdw msg: dtg working");
	}
	else
    {
		printf("breakdw msg: incorrect code\n");
        //devel_webdm_send_log("breakdw msg: incorrect code");
    }
	
	return 0;
}

int data_req_to_taco_cmd(int command, int period, int size, int action_state)
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    return taco_command_call_wrapper(command, period, size);
}

int config_set_to_taco(int code, char *data)
{
	int ret;
	tacom_info_t info;
	info.code = code;
	info.data = data;
//	int result = -1;
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    
	ret = taco_set_info_call_wrapper(&info);
	
	return ret;
}

int reboot(int delay)
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
	return mdmc_api_reset_wrapper();
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    return 0;
}

// ----------------------------------------------------------------------------------
// tacoc wrapper tools..
// ----------------------------------------------------------------------------------

#define TACOC_DATA_ID__NONE           -1
#define TACOC_DATA_ID__CURRENT_DATA   999
#define TACOC_DATA_ID__BULK_DATA      1

#define GTRACK_TOOL_TACOC_RET__INIT           0
#define GTRACK_TOOL_TACOC_RET__FAIL           1
#define GTRACK_TOOL_TACOC_RET__API_CALL       2
#define GTRACK_TOOL_TACOC_RET__SUCCESS        3

typedef struct
{
    int ret_code;
    int tacoc_type;
    int size;
    char* buff;
}GTRACK_TOOL_TACOC_RET_T;

GTRACK_TOOL_TACOC_RET_T g_tacoc_ret;

static int _taco_gtrack_tool__ret_init()
{
    g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__FAIL;
    g_tacoc_ret.size = 0;
    g_tacoc_ret.tacoc_type = TACOC_DATA_ID__NONE;
    g_tacoc_ret.buff = NULL;

    return 0;
}

static int _taco_gtrack_tool__ret_fail()
{
    g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__FAIL;
    g_tacoc_ret.size = 0;
    g_tacoc_ret.tacoc_type = TACOC_DATA_ID__NONE;

    if (g_tacoc_ret.buff != NULL )
        free(g_tacoc_ret.buff);

    g_tacoc_ret.buff = NULL;
    
    return 0;
}



// ----------------------------------------------------------------------------------
// tacoc wrapper tools..
// ----------------------------------------------------------------------------------

int tx_data_to_tacoc(int type, char *stream, int len)
{
    char* tmp_ptr = NULL;

    //printf("---------------------------------------------------------\r\n");
   // printf(" -> type [%d] dat size => [%d] \r\n",type, len);
    //printf("---------------------------------------------------------\r\n");

    // 현재 call 상태인지 확인
    if ( g_tacoc_ret.ret_code != GTRACK_TOOL_TACOC_RET__API_CALL)
    {
        g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__FAIL;
        g_tacoc_ret.buff = NULL;
        g_tacoc_ret.size = 0;
        return 0;
    }

    tmp_ptr = malloc(len);
    
    if ( tmp_ptr == NULL )
    {
        g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__FAIL;
        g_tacoc_ret.buff = NULL;
        g_tacoc_ret.size = 0;
        return 0;
    }
    
    memcpy(tmp_ptr, stream, len);
    
    g_tacoc_ret.buff = tmp_ptr;
    g_tacoc_ret.size = len;
    g_tacoc_ret.tacoc_type = type;
    g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__SUCCESS;

    return 0;
}

int tx_data_to_tacoc_2(int type, char *stream, int len)
{
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;

    int i = 0;

    int body_cnt = 0;
    int header_size = sizeof(tacom_std_hdr_t);
    int body_size = sizeof(tacom_std_data_t);
    
    len = len - header_size;
    body_cnt = len / body_size;

    
    printf("---------------------------------------------------------\r\n");
    printf(" -> type [%d] dat size = header [%d], body [%d], cnt [%d] \r\n",type, header_size, body_size, body_cnt);

	p_std_hdr = (tacom_std_hdr_t *)stream;
    
    for ( i = 0 ; i < body_cnt ; i ++ )
    {
        char tmp_buff[128] = { 0,};
	    p_std_data = (tacom_std_data_t *)&stream[header_size + body_size*i];
        sprintf(tmp_buff, "[%02d] : time :%.14s",i, p_std_data->date_time);
        printf(" >>> %s\r\n", tmp_buff);
    }
    printf("---------------------------------------------------------\r\n");
    return 0;
}



//(tm->tm_setup->conf_flag & (0x1 << SUMMARY_ENABLE_BIT)
int taco_gtrack_tool__save_dtg_data()
{
    int ret = 0;
    ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save
    DTG_LOGD("[DTG GTRACK TOOL] %s() call : ret [%d] \r\n", __func__, ret);
    return ret;
}


int taco_gtrack_tool__get_current_data(char** buff)
{
    int ret = 0;

    taco_gtrack_tool__mutex_lock();
    _taco_gtrack_tool__ret_init();
    
    DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    
    g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__API_CALL;
    //ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, NULL, NULL);
    ret = data_req_to_taco_cmd(CURRENT_DATA, TACOC_DATA_ID__CURRENT_DATA,0, 0);
    if ( ret <= 0 )
    {
        DTG_LOGD("[DTG GTRACK TOOL] %s() call fail [%d]\r\n", __func__, __LINE__);
        ret = -1;
        goto FINISH;
    }
    
    if ( g_tacoc_ret.ret_code != GTRACK_TOOL_TACOC_RET__SUCCESS )
    {
        DTG_LOGD("[DTG GTRACK TOOL] %s() call fail [%d]\r\n", __func__, __LINE__);
        ret = -1;
        goto FINISH;
    }

    if ( g_tacoc_ret.tacoc_type != TACOC_DATA_ID__CURRENT_DATA)
    {
        DTG_LOGD("[DTG GTRACK TOOL] %s() call fail [%d]\r\n", __func__, __LINE__);
        ret = -1;
        goto FINISH;
    }
    
FINISH:
    if ( ret <= 0 )
    {
        _taco_gtrack_tool__ret_fail();
        ret = -1;
    }
    else
    {
        *buff = g_tacoc_ret.buff;
        ret = g_tacoc_ret.size;
    }

    taco_gtrack_tool__mutex_unlock();
    
    return ret;
}


int taco_gtrack_tool__get_remain_cnt()
{
    int ret = 0;
    //ret = data_req_to_taco_cmd(REMAIN_DATA, 0, 0, 1);
    ret = tacom_unreaded_records_num();
    //DTG_LOGD("[DTG GTRACK TOOL] %s() call : ret [%d] \r\n", __func__, ret);
    return ret;
}


int taco_gtrack_tool__get_bulk_data(int cnt, char** buff)
{
    int ret = 0;

    taco_gtrack_tool__mutex_lock();
    _taco_gtrack_tool__ret_init();
    
    //DTG_LOGD("[DTG GTRACK TOOL] %s() call \r\n", __func__);
    
    g_tacoc_ret.ret_code = GTRACK_TOOL_TACOC_RET__API_CALL;
    //ret = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, NULL, NULL);
    ret = data_req_to_taco_cmd(ACCUMAL_DATA, cnt, 0, 0);
    if ( ret <= 0 )
    {
        DTG_LOGD("[DTG GTRACK TOOL] %s() call fail [%d]\r\n", __func__, __LINE__);
        ret = -1;
        goto FINISH;
    }
    
    if ( g_tacoc_ret.ret_code != GTRACK_TOOL_TACOC_RET__SUCCESS )
    {
        DTG_LOGD("[DTG GTRACK TOOL] %s() call fail [%d]\r\n", __func__, __LINE__);
        ret = -1;
        goto FINISH;
    }

    if ( g_tacoc_ret.tacoc_type != TACOC_DATA_ID__BULK_DATA)
    {
        DTG_LOGD("[DTG GTRACK TOOL] %s() call fail [%d]\r\n", __func__, __LINE__);
        ret = -1;
        goto FINISH;
    }
    
FINISH:
    if ( ret <= 0 )
    {
        _taco_gtrack_tool__ret_fail();
        ret = -1;
    }
    else
    {
        // clear data..
        data_req_to_taco_cmd(CLEAR_DATA, 0, 0, 1);
        *buff = g_tacoc_ret.buff;
        ret = g_tacoc_ret.size;
    }

    taco_gtrack_tool__mutex_unlock();
    
    return ret;
}

tacom_std_data_t g_std_data;

int taco_gtrack_tool__set_cur_std_data(tacom_std_data_t *p_std_data_in)
{
    
    int dtg_year = 2000 + char_mbtol(p_std_data_in->date_time, 2);
    
    if ( dtg_year > 2016 )
    {
        taco_gtrack_tool__mutex_lock();
        memcpy(&g_std_data, p_std_data_in, sizeof(tacom_std_data_t));
        taco_gtrack_tool__mutex_unlock();
    }

    printf("taco_gtrack_tool__set_cur_std_data => key stat[%d]\r\n", p_std_data_in->key_stat);
#ifdef USE_GPS_FAKE
    if ( dtg_year > 2016 )
    {
        gpsData_t gps_data = {0,};
        taco_gtrack_tool__conv_dtg_to_gps(p_std_data_in, &gps_data);
        gps_set_curr_data(&gps_data);
    }
    
#endif
    return 0;
}


int taco_gtrack_tool__get_cur_std_data(tacom_std_data_t *p_std_data_out)
{
    int dtg_year = 0;

    taco_gtrack_tool__mutex_lock();
	memcpy(p_std_data_out, &g_std_data, sizeof(tacom_std_data_t));
    taco_gtrack_tool__mutex_unlock();

    dtg_year = 2000 + char_mbtol(p_std_data_out->date_time, 2);
    
    if ( dtg_year <= 2016)
    {
        memset(p_std_data_out, 0x00, sizeof(tacom_std_data_t));
        return -1;
    }
    
    /*
    printf("taco_gtrack_tool__get_cur_std_data ++ \r\n");
    printf("p_std_data_out->date_time [%s]\r\n", p_std_data_out->date_time);
    printf("p_std_data_out->day_run_dist [%s]\r\n", p_std_data_out->day_run_dist);
    printf("p_std_data_out->status [%s]\r\n", p_std_data_out->status);
    printf("taco_gtrack_tool__get_cur_std_data -- \r\n");
    */
   
    return 0;
}

int taco_gtrack_tool__conv_dtg_to_gps(tacom_std_data_t *p_std_data, gpsData_t * p_gps_data)
{
    struct timeval tv;
	struct tm ttm;

    int taco_year = 2000 + char_mbtol(p_std_data->date_time, 2);
    
    float tmp_float_val = 0 ;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);

    // init time... default modem system time..
    p_gps_data->utc_sec = mktime(&ttm);
    p_gps_data->year = ttm.tm_year + 1900;
    p_gps_data->mon = ttm.tm_mon + 1;
    p_gps_data->day = ttm.tm_mday;
    p_gps_data->hour = ttm.tm_hour;
    p_gps_data->min = ttm.tm_min;
    p_gps_data->sec = ttm.tm_sec;

    if ( taco_year < 2016 )
    {
        p_gps_data->active = 0;
        return 0;
    }

    p_gps_data->year = 2000 + char_mbtol(p_std_data->date_time, 2);
    p_gps_data->mon = char_mbtol(p_std_data->date_time+2, 2);
    p_gps_data->day = char_mbtol(p_std_data->date_time+4, 2);
    p_gps_data->hour = char_mbtol(p_std_data->date_time+6, 2);
    p_gps_data->min = char_mbtol(p_std_data->date_time+8, 2);
    p_gps_data->sec = char_mbtol(p_std_data->date_time+10, 2);

    ttm.tm_year = p_gps_data->year - 1900;
    ttm.tm_mon = p_gps_data->mon - 1;
    ttm.tm_mday = p_gps_data->day;
    ttm.tm_hour = p_gps_data->hour;
    ttm.tm_min = p_gps_data->min;
    ttm.tm_sec = p_gps_data->sec;
    p_gps_data->utc_sec = mktime(&ttm);

    p_gps_data->satellite = 0;
    tmp_float_val=  char_mbtol(p_std_data->gps_y, 9);			// longitude
    p_gps_data->lat = tmp_float_val / 1000000;
    tmp_float_val = char_mbtol(p_std_data->gps_x, 9);			// latitude
    p_gps_data->lon = tmp_float_val / 1000000;
    p_gps_data->speed = char_mbtol(p_std_data->speed, 3);;			// speed(km/s)

    if ( ( p_gps_data->lat > 0 ) && ( p_gps_data->lon ))
        p_gps_data->active = 1;
    else
        p_gps_data->active = 0;
    
    p_gps_data->angle = char_mbtof(p_std_data->azimuth, 3);;
    p_gps_data->hdop = 0 ;
    p_gps_data->altitude = 0;  // meter.. new filed (170913)


    printf("set current data : [%d][%d][%d][%d][%d][%d] / [%f][%f]\r\n", 
            p_gps_data->year,
            p_gps_data->mon,
            p_gps_data->day,
            p_gps_data->hour,
            p_gps_data->min,
            p_gps_data->sec,
            p_gps_data->lat,
            p_gps_data->lon);

    //printf("p_std_data->gps_x [%s]\r\n",p_std_data->gps_x);
    //printf("p_std_data->gps_y [%s]\r\n",p_std_data->gps_y);

}

int taco_gtrack_tool__pre_init()
{
    // call dtg model...
    init_dtg_callback();

}