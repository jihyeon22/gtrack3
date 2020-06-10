<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <base/sender.h>

#include <util/transfer.h>
#include <logd_rpc.h>

#include <board/power.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include "dtg_gtrack_tool.h"

#include "netcom.h"
#include "debug.h"

#include "lila_packet.h"
#include "lila_tools.h"
#include "data-list.h"

#include "dtg_pkt_senario.h"
#include "dtg_gtrack_tool.h"
#include "dtg_carnum_tool.h"


static int _lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len, int pkt_idx, int lila_dtg_data_cnt);


int lila_dtg__send_dtg_header()
{
	sender_add_data_to_buffer(ePKT_TRANSFER_ID__DTG_INFO, NULL, ePIPE_1);
    return 0;
}


int lila_dtg__send_dtg_data()
{
	sender_add_data_to_buffer(ePKT_TRANSFER_ID__DTG_DATA, NULL, ePIPE_1);
    return 0;
}


// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__make_dtg_header(unsigned char **pbuf, unsigned short *packet_len)
{
    int lila_pkt_size = 0;
    char * tmp_mk_pkt_buff = NULL;

    tacom_std_hdr_t current_std_hdr; 
    tacom_std_data_t current_std_data;

    LILA_PKT_FRAME__HEADER_T*   p_lila_pkt_frame_header;
    LILA_PKT_DATA__DTG_INFO_T*  p_lila_pkt_data;
    LILA_PKT_FRAME__TAIL_T*     p_lila_pkt_frame_tail;

    memset(&current_std_hdr, 0x00, sizeof(current_std_hdr));
    memset(&current_std_data, 0x00, sizeof(current_std_hdr));

    lila_dtg__get_current_dtg_data(&current_std_hdr, &current_std_data);

    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__HEADER_T);
    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_DATA__DTG_INFO_T);
    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__TAIL_T);

    tmp_mk_pkt_buff = malloc(lila_pkt_size);

    if (tmp_mk_pkt_buff == NULL)
        return LILA_PKT_RET__FAIL;

    memset(tmp_mk_pkt_buff, 0x00, lila_pkt_size);

    p_lila_pkt_frame_header = (LILA_PKT_FRAME__HEADER_T*)(tmp_mk_pkt_buff + 0);
    p_lila_pkt_data = (LILA_PKT_DATA__DTG_INFO_T*)(tmp_mk_pkt_buff + sizeof(LILA_PKT_FRAME__HEADER_T));
    p_lila_pkt_frame_tail = (LILA_PKT_FRAME__TAIL_T*)(tmp_mk_pkt_buff + sizeof(LILA_PKT_FRAME__HEADER_T) + sizeof(LILA_PKT_DATA__DTG_INFO_T));

    // make header..
    p_lila_pkt_frame_header->pkt_prefix = LILA_PKT_PREFIX;
    p_lila_pkt_frame_header->dev_id = LILA_PKT_ID__DEVICE_ID;
    p_lila_pkt_frame_header->phone_no = lila_tools__get_phonenum_int_type();
    strcpy(p_lila_pkt_frame_header->pkt_command, lila_tools__conv_buff_to_str_reverse(LILA_PKT_ID__INFO, strlen(LILA_PKT_ID__INFO)));
    p_lila_pkt_frame_header->idx = 0 ;
    p_lila_pkt_frame_header->data_cnt = 1;
    p_lila_pkt_frame_header->data_len = sizeof(LILA_PKT_DATA__DTG_INFO_T);

    lila_dtg_debug__print_frame_header(p_lila_pkt_frame_header); // NOTE: debug

    // data...
    //p_lila_pkt_data->reserved_1;
    //p_lila_pkt_data->reserved_2;
    //p_lila_pkt_data->driver_name;
    strcpy(p_lila_pkt_data->driver_code, lila_tools__conv_buff_to_str(current_std_hdr.driver_code, 18));
    lila_dtg__convert_car_num(&current_std_hdr, p_lila_pkt_data->car_regi_no);
    strcpy(p_lila_pkt_data->car_code, lila_tools__conv_buff_to_str(current_std_hdr.vehicle_id_num , 17));
    //p_lila_pkt_data->company_name;
    strcpy(p_lila_pkt_data->company_regi_no_1, lila_tools__conv_buff_to_str(current_std_hdr.business_license_num, 10));
    //p_lila_pkt_data->company_regi_no_2;
    //p_lila_pkt_data->serial_no;
    strcpy(p_lila_pkt_data->model_no, lila_tools__conv_buff_to_str(current_std_hdr.vehicle_model, 20));
    //p_lila_pkt_data->k_factor = 0; // not support
    //p_lila_pkt_data->rpm_factor = 0;
    //p_lila_pkt_data->reserved_3;
    //p_lila_pkt_data->firmware_ver;
    //p_lila_pkt_data->reserved_4;
    //p_lila_pkt_data->reserved_5;
    //p_lila_pkt_data->reserved_6;
    //p_lila_pkt_data->reserved_7;
    //p_lila_pkt_data->reserved_8;
    //p_lila_pkt_data->reserved_9;
    //p_lila_pkt_data->reserved_10;

    lila_dtg_debug__print_dtg_info(p_lila_pkt_data); // NOTE: debug

    // tail..
    p_lila_pkt_frame_tail->crc = lila_tools__crc16_ccitt(tmp_mk_pkt_buff, (lila_pkt_size - sizeof(LILA_PKT_FRAME__TAIL_T))) ;
    p_lila_pkt_frame_tail->pkt_suffix = LILA_PKT_SUFFIX;

    lila_dtg_debug__print_frame_tail(p_lila_pkt_frame_tail); // NOTE: debug

    *pbuf = (unsigned char *)tmp_mk_pkt_buff;
    *packet_len = lila_pkt_size;

    return 0;
}

// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__make_dtg_data_dummy(unsigned char **pbuf, unsigned short *packet_len)
{
    int lila_pkt_size = 8;
    void * tmp_mk_pkt_buff = NULL;

    tmp_mk_pkt_buff = malloc(lila_pkt_size);

    memset(tmp_mk_pkt_buff, 0xff, lila_pkt_size);

    *pbuf = (unsigned char *)tmp_mk_pkt_buff;
    *packet_len = lila_pkt_size;

    return 0;
}

// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len)
{
    return _lila_dtg__make_dtg_data(pbuf, packet_len, 0, LILA_DTG_PKT_BULK_CHK_CNT);;
}


// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__parse_dtg_header(LILA_PKT_RESP__DTST_T* resp)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> RECV DATA ++ <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
    printf("lila_dtg__parse_dtg_header start\r\n");
    lila_dtg_debug__print_frame_header(&resp->header);
    /*
    printf(" pkt_command -------------------------------------------\r\n");
    for ( i = 0 ; i < 4 ; i ++)
        printf("[0x%x]\r\n", resp->header.pkt_command[i]);
    printf("-------------------------------------------\r\n");
    printf("lila_dtg__parse_dtg_header data [%d]\r\n", resp->data);
    */
    lila_dtg_debug__print_frame_tail(&resp->tail);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> RECV DATA -- <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
    return 0;
}

// -------------------------------------------------------------------------------
// pkt tools
// -------------------------------------------------------------------------------
static int lila_dtg__insert_dtg_data(tacom_std_data_t* p_current_std_data)
{
    LILA_PKT_DATA__DATA_T*      p_lila_pkt_data;

    struct timeval tv;
	struct tm ttm;

    int dtg_time_year;
    int dtg_time_mon;
    int dtg_time_day;
    int dtg_time_hour;
    int dtg_time_min;
    int dtg_time_sec;
    unsigned int dtg_time_utc;

    float tmp_val = 0;
    p_lila_pkt_data = malloc(sizeof(LILA_PKT_DATA__DATA_T));

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);

    // modem time..
    /*
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
    */

    dtg_time_year = 2000 + char_mbtol(p_current_std_data->date_time, 2);
    dtg_time_mon = char_mbtol(p_current_std_data->date_time+2, 2);
    dtg_time_day = char_mbtol(p_current_std_data->date_time+4, 2);
    dtg_time_hour = char_mbtol(p_current_std_data->date_time+6, 2);
    dtg_time_min = char_mbtol(p_current_std_data->date_time+8, 2);
    dtg_time_sec = char_mbtol(p_current_std_data->date_time+10, 2);

    ttm.tm_year = dtg_time_year - 1900;
    ttm.tm_mon = dtg_time_mon - 1;
    ttm.tm_mday = dtg_time_day;
    ttm.tm_hour = dtg_time_hour;
    ttm.tm_min = dtg_time_min;
    ttm.tm_sec = dtg_time_sec;
    dtg_time_utc = mktime(&ttm);


    p_lila_pkt_data->utc_time = dtg_time_utc;                 // Time 	4	u32 
    p_lila_pkt_data->speed = char_mbtol(p_current_std_data->speed,3);                    // Speed 	1	u8 
    p_lila_pkt_data->speed_float = 0;              // Speed Float 	1	u8 
    p_lila_pkt_data->rpm = char_mbtol(p_current_std_data->rpm,4);                     // RPM 	2	u16 
    p_lila_pkt_data->car_signal = lila_tools__get_car_signal();              // Signal 	2	u16 
    p_lila_pkt_data->car_status  = lila_tools__get_car_stat(lila_tools__conv_buff_to_str(p_current_std_data->status,2));              // Status 	2	u16
    p_lila_pkt_data->gps_lat = char_mbtol(p_current_std_data->gps_x, 9);                   // Latitude 	4	u32 
    p_lila_pkt_data->gps_lon = char_mbtol(p_current_std_data->gps_y, 9);                   // Longitude 	4	u32 
    p_lila_pkt_data->gps_azimuth = char_mbtol(p_current_std_data->azimuth, 9);             // Azimuth 	2	u16 
    p_lila_pkt_data->gps_status =  lila_tools__get_gps_stat(p_lila_pkt_data->gps_lat, p_lila_pkt_data->gps_lon);              // GPS Status 	1	char // TODO: 구현해야함
    p_lila_pkt_data->gps_speed =  char_mbtol(p_current_std_data->speed, 2);                // GPS Speed 	1	u8 
    p_lila_pkt_data->acceleration_x = char_mbtol(p_current_std_data->accelation_x, 6);          // 가속도 Vx 	2	s16 
    p_lila_pkt_data->acceleration_y = char_mbtol(p_current_std_data->accelation_y, 6);          // 가속도 Vy 	2	s16 
    p_lila_pkt_data->trip_cnt = 0;                 // Trip count 	1	u8 
    p_lila_pkt_data->driver_no = 0;                // Driver No 	1	u8 
    p_lila_pkt_data->modem_rssi = lila_tools__get_rssi(dtg_time_utc);               // RSSI 	1	u8 
    p_lila_pkt_data->reserved_1 = 0;                // reserved 	1	u8
    p_lila_pkt_data->cur_distance = char_mbtof(p_current_std_data->cumulative_run_distance, 7);;             // 주행거리 	4	float 
    p_lila_pkt_data->day_distance = char_mbtof(p_current_std_data->day_run_distance, 4);             // 일주행거리 	4	float 
    p_lila_pkt_data->total_distance = char_mbtof(p_current_std_data->cumulative_run_distance, 7);           // 총주행거리 	4	float 
    p_lila_pkt_data->cur_fuel_consumption = 0;     // 연료소모량 	4	float 
    p_lila_pkt_data->day_fuel_consumption = 0;     // 일연료소모량 	4	float 
    p_lila_pkt_data->total_fuel_consumption = 0;   // 총연료소모량 	4	float 
    p_lila_pkt_data->batt_volt = lila_tools__get_car_batt(dtg_time_utc);               // Battery volt 	2	u16 
    {
        unsigned char aebs_data, ldw_data;
        lila_tools__get_adas_data(dtg_time_utc, &aebs_data, &ldw_data);
        p_lila_pkt_data->adas_aebs = aebs_data;               // AEBS	1	u8
        p_lila_pkt_data->adas_ldw = ldw_data;                // LDW	1	u8
    }
    p_lila_pkt_data->temp_1 = 0;                  // Temp1	2	s16
    p_lila_pkt_data->temp_2 = 0;                  // Temp2	2	s16

	if(list_add(&lila_dtg_data_buffer_list, p_lila_pkt_data) < 0)
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
		free(p_lila_pkt_data);
	}

    // lila_dtg_debug__print_dtg_data(p_lila_pkt_data);
    return 0;
}

// -------------------------------------------------------------------------------
// pkt tools
// -------------------------------------------------------------------------------
static int _lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len, int pkt_idx, int lila_dtg_data_cnt)
{
    int lila_pkt_size = 0;
    // int lila_dtg_data_cnt = 0;

    int i = 0;

    void * tmp_mk_pkt_buff = NULL;
    char * p_tmp_lila_pkt_data = NULL;

    LILA_PKT_FRAME__HEADER_T*   p_lila_pkt_frame_header;
    LILA_PKT_DATA__DATA_T*      p_lila_pkt_data;
    LILA_PKT_FRAME__TAIL_T*     p_lila_pkt_frame_tail;

    printf("lila_dtg_data_cnt is [%d]\r\n", lila_dtg_data_cnt);

    int lila_dtg_data_total_cnt = get_lila_dtg_data_count();

    if ( ( lila_dtg_data_cnt <= 0 )|| ( lila_dtg_data_total_cnt <= 0) )
        return -1;
    
    if ( lila_dtg_data_total_cnt < lila_dtg_data_cnt )
        lila_dtg_data_cnt = lila_dtg_data_cnt;

    // lila_dtg__get_current_dtg_data(&current_std_hdr, &current_std_data);

    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__HEADER_T);
    lila_pkt_size = lila_pkt_size + (sizeof(LILA_PKT_DATA__DATA_T) * lila_dtg_data_cnt);
    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__TAIL_T);

    tmp_mk_pkt_buff = malloc(lila_pkt_size);

    p_lila_pkt_frame_header = (LILA_PKT_FRAME__HEADER_T*)(tmp_mk_pkt_buff);
    p_lila_pkt_data = (LILA_PKT_DATA__DATA_T*)(tmp_mk_pkt_buff + sizeof(LILA_PKT_FRAME__HEADER_T));
    p_lila_pkt_frame_tail = (LILA_PKT_FRAME__TAIL_T*)(tmp_mk_pkt_buff + (sizeof(LILA_PKT_DATA__DATA_T) * lila_dtg_data_cnt));


    if (tmp_mk_pkt_buff == NULL)
        return LILA_PKT_RET__FAIL;

    memset(tmp_mk_pkt_buff, 0x00, lila_pkt_size);

    // make header..
    p_lila_pkt_frame_header->pkt_prefix = LILA_PKT_PREFIX;
    p_lila_pkt_frame_header->dev_id = LILA_PKT_ID__DEVICE_ID;
    p_lila_pkt_frame_header->phone_no = lila_tools__get_phonenum_int_type();
    strcpy(p_lila_pkt_frame_header->pkt_command, lila_tools__conv_buff_to_str_reverse(LILA_PKT_ID__DATA_SEND_SYNC, strlen(LILA_PKT_ID__DATA_SEND_SYNC)));
    p_lila_pkt_frame_header->idx = pkt_idx ;
    p_lila_pkt_frame_header->data_cnt = lila_dtg_data_cnt;
    p_lila_pkt_frame_header->data_len = sizeof(LILA_PKT_DATA__DTG_INFO_T) * lila_dtg_data_cnt;

    for ( i = 0 ; i < lila_dtg_data_cnt; i ++ ) 
    {
        if(list_pop(&lila_dtg_data_buffer_list, (void *)&p_tmp_lila_pkt_data) < 0)
            return -1;
        
        memcpy(p_lila_pkt_data, p_tmp_lila_pkt_data, sizeof(LILA_PKT_DATA__DATA_T));
        /*
        if ( p_tmp_lila_pkt_data != NULL)
        {
            printf("p_tmp_lila_pkt_data is not null ... try free() [%d]\r\n", *p_tmp_lila_pkt_data);
            free(p_tmp_lila_pkt_data);
        }
        */
        free(p_tmp_lila_pkt_data);
        // lila_dtg_debug__print_dtg_data(p_lila_pkt_data);
        p_lila_pkt_data++;
    }

    // tail..
    p_lila_pkt_frame_tail->crc = lila_tools__crc16_ccitt(tmp_mk_pkt_buff, (lila_pkt_size - sizeof(LILA_PKT_FRAME__TAIL_T))) ;
    p_lila_pkt_frame_tail->pkt_suffix = LILA_PKT_SUFFIX;

    // lila_dtg_debug__print_frame_tail(p_lila_pkt_frame_tail); // NOTE: debug

    *pbuf = (unsigned char *)tmp_mk_pkt_buff;
    *packet_len = lila_pkt_size;

    return lila_pkt_size;
}


int lila_dtg__convert_and_insert_bulk_stream(char* stream, int len, char** buf)
{
    int cnt = 0;
    int pack_buffer_size = 0;
    // int send_pack_size = 0;
    // unsigned char* dtg_pack_buf = NULL;

    char* p_idx_stream = stream;

    int i = 0 ;

    cnt = (len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t) ;
    p_idx_stream += sizeof(tacom_std_hdr_t);

    for (i = 0 ; i < cnt ; i ++)
    {
        tacom_std_data_t* p_tmp_std_data = NULL;
        p_tmp_std_data = (tacom_std_data_t*)p_idx_stream;
        lila_dtg__insert_dtg_data(p_tmp_std_data);
        p_idx_stream += sizeof(tacom_std_data_t);
    }

	return pack_buffer_size;
}


int lila_dtg__convert_dtg_data_to_pkt()
{
    char * tmp_bluk_stream_buff = NULL;
    char * tmp_mk_pkt_buff = NULL;

    int taco_data_size = 0;
    int disc_pkt_size = 0;

    if ( (taco_data_size = taco_gtrack_tool__get_bulk_data(LILA_DTG_PKT_BULK_CHK_CNT, &tmp_bluk_stream_buff)) > 0 )
    {
        disc_pkt_size = lila_dtg__convert_and_insert_bulk_stream(tmp_bluk_stream_buff, taco_data_size, &tmp_mk_pkt_buff);
        
        free(tmp_bluk_stream_buff);
    }

    return 0;
}


int lila_dtg__clr_dtg_taco_buff()
{
    char * tmp_bluk_stream_buff = NULL;
    char * tmp_cur_stream_buff = NULL;
    // char * tmp_mk_pkt_buff = NULL;

    int taco_data_size = 0;
    int ret_bufflen = 0;
 
    if ( (taco_data_size = taco_gtrack_tool__get_bulk_data(10, &tmp_bluk_stream_buff)) > 0 )
    {
        // first current data set
        if (( ret_bufflen = taco_gtrack_tool__get_current_data(&tmp_cur_stream_buff) ) > 0 )
        {
            //set_current_dtg_data((unsigned char*)tmp_cur_stream_buff, ret_bufflen);
            free(tmp_cur_stream_buff);
        }

        free(tmp_bluk_stream_buff);
    }
    
    return 0;
}

=======
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <base/sender.h>

#include <util/transfer.h>
#include <logd_rpc.h>

#include <board/power.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include "dtg_gtrack_tool.h"

#include "netcom.h"
#include "debug.h"

#include "lila_packet.h"
#include "lila_tools.h"
#include "data-list.h"

#include "dtg_pkt_senario.h"
#include "dtg_gtrack_tool.h"
#include "dtg_carnum_tool.h"


static int _lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len, int pkt_idx, int lila_dtg_data_cnt);


int lila_dtg__send_dtg_header()
{
	sender_add_data_to_buffer(ePKT_TRANSFER_ID__DTG_INFO, NULL, ePIPE_1);
    return 0;
}


int lila_dtg__send_dtg_data()
{
	sender_add_data_to_buffer(ePKT_TRANSFER_ID__DTG_DATA, NULL, ePIPE_1);
    return 0;
}


// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__make_dtg_header(unsigned char **pbuf, unsigned short *packet_len)
{
    int lila_pkt_size = 0;
    char * tmp_mk_pkt_buff = NULL;

    tacom_std_hdr_t current_std_hdr; 
    tacom_std_data_t current_std_data;

    LILA_PKT_FRAME__HEADER_T*   p_lila_pkt_frame_header;
    LILA_PKT_DATA__DTG_INFO_T*  p_lila_pkt_data;
    LILA_PKT_FRAME__TAIL_T*     p_lila_pkt_frame_tail;

    memset(&current_std_hdr, 0x00, sizeof(current_std_hdr));
    memset(&current_std_data, 0x00, sizeof(current_std_hdr));

    lila_dtg__get_current_dtg_data(&current_std_hdr, &current_std_data);

    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__HEADER_T);
    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_DATA__DTG_INFO_T);
    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__TAIL_T);

    tmp_mk_pkt_buff = malloc(lila_pkt_size);

    if (tmp_mk_pkt_buff == NULL)
        return LILA_PKT_RET__FAIL;

    memset(tmp_mk_pkt_buff, 0x00, lila_pkt_size);

    p_lila_pkt_frame_header = (LILA_PKT_FRAME__HEADER_T*)(tmp_mk_pkt_buff + 0);
    p_lila_pkt_data = (LILA_PKT_DATA__DTG_INFO_T*)(tmp_mk_pkt_buff + sizeof(LILA_PKT_FRAME__HEADER_T));
    p_lila_pkt_frame_tail = (LILA_PKT_FRAME__TAIL_T*)(tmp_mk_pkt_buff + sizeof(LILA_PKT_FRAME__HEADER_T) + sizeof(LILA_PKT_DATA__DTG_INFO_T));

    // make header..
    p_lila_pkt_frame_header->pkt_prefix = LILA_PKT_PREFIX;
    p_lila_pkt_frame_header->dev_id = LILA_PKT_ID__DEVICE_ID;
    p_lila_pkt_frame_header->phone_no = lila_tools__get_phonenum_int_type();
    strcpy(p_lila_pkt_frame_header->pkt_command, lila_tools__conv_buff_to_str_reverse(LILA_PKT_ID__INFO, strlen(LILA_PKT_ID__INFO)));
    p_lila_pkt_frame_header->idx = 0 ;
    p_lila_pkt_frame_header->data_cnt = 1;
    p_lila_pkt_frame_header->data_len = sizeof(LILA_PKT_DATA__DTG_INFO_T);

    lila_dtg_debug__print_frame_header(p_lila_pkt_frame_header); // NOTE: debug

    // data...
    //p_lila_pkt_data->reserved_1;
    //p_lila_pkt_data->reserved_2;
    //p_lila_pkt_data->driver_name;
    strcpy(p_lila_pkt_data->driver_code, lila_tools__conv_buff_to_str(current_std_hdr.driver_code, 18));
    lila_dtg__convert_car_num(&current_std_hdr, p_lila_pkt_data->car_regi_no);
    strcpy(p_lila_pkt_data->car_code, lila_tools__conv_buff_to_str(current_std_hdr.vehicle_id_num , 17));
    //p_lila_pkt_data->company_name;
    strcpy(p_lila_pkt_data->company_regi_no_1, lila_tools__conv_buff_to_str(current_std_hdr.business_license_num, 10));
    //p_lila_pkt_data->company_regi_no_2;
    //p_lila_pkt_data->serial_no;
    strcpy(p_lila_pkt_data->model_no, lila_tools__conv_buff_to_str(current_std_hdr.vehicle_model, 20));
    //p_lila_pkt_data->k_factor = 0; // not support
    //p_lila_pkt_data->rpm_factor = 0;
    //p_lila_pkt_data->reserved_3;
    //p_lila_pkt_data->firmware_ver;
    //p_lila_pkt_data->reserved_4;
    //p_lila_pkt_data->reserved_5;
    //p_lila_pkt_data->reserved_6;
    //p_lila_pkt_data->reserved_7;
    //p_lila_pkt_data->reserved_8;
    //p_lila_pkt_data->reserved_9;
    //p_lila_pkt_data->reserved_10;

    lila_dtg_debug__print_dtg_info(p_lila_pkt_data); // NOTE: debug

    // tail..
    p_lila_pkt_frame_tail->crc = lila_tools__crc16_ccitt(tmp_mk_pkt_buff, (lila_pkt_size - sizeof(LILA_PKT_FRAME__TAIL_T))) ;
    p_lila_pkt_frame_tail->pkt_suffix = LILA_PKT_SUFFIX;

    lila_dtg_debug__print_frame_tail(p_lila_pkt_frame_tail); // NOTE: debug

    *pbuf = (unsigned char *)tmp_mk_pkt_buff;
    *packet_len = lila_pkt_size;

    return 0;
}

// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__make_dtg_data_dummy(unsigned char **pbuf, unsigned short *packet_len)
{
    int lila_pkt_size = 8;
    void * tmp_mk_pkt_buff = NULL;

    tmp_mk_pkt_buff = malloc(lila_pkt_size);

    memset(tmp_mk_pkt_buff, 0xff, lila_pkt_size);

    *pbuf = (unsigned char *)tmp_mk_pkt_buff;
    *packet_len = lila_pkt_size;

    return 0;
}

// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len)
{
    return _lila_dtg__make_dtg_data(pbuf, packet_len, 0, LILA_DTG_PKT_BULK_CHK_CNT);;
}


// -------------------------------------------------------------------------------
// pkt make util : call from netcom.
// -------------------------------------------------------------------------------
int lila_dtg__parse_dtg_header(LILA_PKT_RESP__DTST_T* resp)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> RECV DATA ++ <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
    printf("lila_dtg__parse_dtg_header start\r\n");
    lila_dtg_debug__print_frame_header(&resp->header);
    /*
    printf(" pkt_command -------------------------------------------\r\n");
    for ( i = 0 ; i < 4 ; i ++)
        printf("[0x%x]\r\n", resp->header.pkt_command[i]);
    printf("-------------------------------------------\r\n");
    printf("lila_dtg__parse_dtg_header data [%d]\r\n", resp->data);
    */
    lila_dtg_debug__print_frame_tail(&resp->tail);
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> RECV DATA -- <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\r\n");
    return 0;
}

// -------------------------------------------------------------------------------
// pkt tools
// -------------------------------------------------------------------------------
static int lila_dtg__insert_dtg_data(tacom_std_data_t* p_current_std_data)
{
    LILA_PKT_DATA__DATA_T*      p_lila_pkt_data;

    struct timeval tv;
	struct tm ttm;

    int dtg_time_year;
    int dtg_time_mon;
    int dtg_time_day;
    int dtg_time_hour;
    int dtg_time_min;
    int dtg_time_sec;
    unsigned int dtg_time_utc;

    float tmp_val = 0;
    p_lila_pkt_data = malloc(sizeof(LILA_PKT_DATA__DATA_T));

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);

    // modem time..
    /*
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
    */

    dtg_time_year = 2000 + char_mbtol(p_current_std_data->date_time, 2);
    dtg_time_mon = char_mbtol(p_current_std_data->date_time+2, 2);
    dtg_time_day = char_mbtol(p_current_std_data->date_time+4, 2);
    dtg_time_hour = char_mbtol(p_current_std_data->date_time+6, 2);
    dtg_time_min = char_mbtol(p_current_std_data->date_time+8, 2);
    dtg_time_sec = char_mbtol(p_current_std_data->date_time+10, 2);

    ttm.tm_year = dtg_time_year - 1900;
    ttm.tm_mon = dtg_time_mon - 1;
    ttm.tm_mday = dtg_time_day;
    ttm.tm_hour = dtg_time_hour;
    ttm.tm_min = dtg_time_min;
    ttm.tm_sec = dtg_time_sec;
    dtg_time_utc = mktime(&ttm);


    p_lila_pkt_data->utc_time = dtg_time_utc;                 // Time 	4	u32 
    p_lila_pkt_data->speed = char_mbtol(p_current_std_data->speed,3);                    // Speed 	1	u8 
    p_lila_pkt_data->speed_float = 0;              // Speed Float 	1	u8 
    p_lila_pkt_data->rpm = char_mbtol(p_current_std_data->rpm,4);                     // RPM 	2	u16 
    p_lila_pkt_data->car_signal = lila_tools__get_car_signal();              // Signal 	2	u16 
    p_lila_pkt_data->car_status  = lila_tools__get_car_stat(lila_tools__conv_buff_to_str(p_current_std_data->status,2));              // Status 	2	u16
    p_lila_pkt_data->gps_lat = char_mbtol(p_current_std_data->gps_x, 9);                   // Latitude 	4	u32 
    p_lila_pkt_data->gps_lon = char_mbtol(p_current_std_data->gps_y, 9);                   // Longitude 	4	u32 
    p_lila_pkt_data->gps_azimuth = char_mbtol(p_current_std_data->azimuth, 9);             // Azimuth 	2	u16 
    p_lila_pkt_data->gps_status =  lila_tools__get_gps_stat(p_lila_pkt_data->gps_lat, p_lila_pkt_data->gps_lon);              // GPS Status 	1	char // TODO: 구현해야함
    p_lila_pkt_data->gps_speed =  char_mbtol(p_current_std_data->speed, 2);                // GPS Speed 	1	u8 
    p_lila_pkt_data->acceleration_x = char_mbtol(p_current_std_data->accelation_x, 6);          // 가속도 Vx 	2	s16 
    p_lila_pkt_data->acceleration_y = char_mbtol(p_current_std_data->accelation_y, 6);          // 가속도 Vy 	2	s16 
    p_lila_pkt_data->trip_cnt = 0;                 // Trip count 	1	u8 
    p_lila_pkt_data->driver_no = 0;                // Driver No 	1	u8 
    p_lila_pkt_data->modem_rssi = lila_tools__get_rssi(dtg_time_utc);               // RSSI 	1	u8 
    p_lila_pkt_data->reserved_1 = 0;                // reserved 	1	u8
    p_lila_pkt_data->cur_distance = char_mbtof(p_current_std_data->cumulative_run_distance, 7);;             // 주행거리 	4	float 
    p_lila_pkt_data->day_distance = char_mbtof(p_current_std_data->day_run_distance, 4);             // 일주행거리 	4	float 
    p_lila_pkt_data->total_distance = char_mbtof(p_current_std_data->cumulative_run_distance, 7);           // 총주행거리 	4	float 
    p_lila_pkt_data->cur_fuel_consumption = 0;     // 연료소모량 	4	float 
    p_lila_pkt_data->day_fuel_consumption = 0;     // 일연료소모량 	4	float 
    p_lila_pkt_data->total_fuel_consumption = 0;   // 총연료소모량 	4	float 
    p_lila_pkt_data->batt_volt = lila_tools__get_car_batt(dtg_time_utc);               // Battery volt 	2	u16 
    {
        unsigned char aebs_data, ldw_data;
        lila_tools__get_adas_data(dtg_time_utc, &aebs_data, &ldw_data);
        p_lila_pkt_data->adas_aebs = aebs_data;               // AEBS	1	u8
        p_lila_pkt_data->adas_ldw = ldw_data;                // LDW	1	u8
    }
    p_lila_pkt_data->temp_1 = 0;                  // Temp1	2	s16
    p_lila_pkt_data->temp_2 = 0;                  // Temp2	2	s16

	if(list_add(&lila_dtg_data_buffer_list, p_lila_pkt_data) < 0)
	{
		LOGE(LOG_TARGET, "%s : list add fail\n", __func__);
		free(p_lila_pkt_data);
	}

    // lila_dtg_debug__print_dtg_data(p_lila_pkt_data);
    return 0;
}

// -------------------------------------------------------------------------------
// pkt tools
// -------------------------------------------------------------------------------
static int _lila_dtg__make_dtg_data(unsigned char **pbuf, unsigned short *packet_len, int pkt_idx, int lila_dtg_data_cnt)
{
    int lila_pkt_size = 0;
    // int lila_dtg_data_cnt = 0;

    int i = 0;

    void * tmp_mk_pkt_buff = NULL;
    char * p_tmp_lila_pkt_data = NULL;

    LILA_PKT_FRAME__HEADER_T*   p_lila_pkt_frame_header;
    LILA_PKT_DATA__DATA_T*      p_lila_pkt_data;
    LILA_PKT_FRAME__TAIL_T*     p_lila_pkt_frame_tail;

    printf("lila_dtg_data_cnt is [%d]\r\n", lila_dtg_data_cnt);

    int lila_dtg_data_total_cnt = get_lila_dtg_data_count();

    if ( ( lila_dtg_data_cnt <= 0 )|| ( lila_dtg_data_total_cnt <= 0) )
        return -1;
    
    if ( lila_dtg_data_total_cnt < lila_dtg_data_cnt )
        lila_dtg_data_cnt = lila_dtg_data_cnt;

    // lila_dtg__get_current_dtg_data(&current_std_hdr, &current_std_data);

    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__HEADER_T);
    lila_pkt_size = lila_pkt_size + (sizeof(LILA_PKT_DATA__DATA_T) * lila_dtg_data_cnt);
    lila_pkt_size = lila_pkt_size + sizeof(LILA_PKT_FRAME__TAIL_T);

    tmp_mk_pkt_buff = malloc(lila_pkt_size);

    p_lila_pkt_frame_header = (LILA_PKT_FRAME__HEADER_T*)(tmp_mk_pkt_buff);
    p_lila_pkt_data = (LILA_PKT_DATA__DATA_T*)(tmp_mk_pkt_buff + sizeof(LILA_PKT_FRAME__HEADER_T));
    p_lila_pkt_frame_tail = (LILA_PKT_FRAME__TAIL_T*)(tmp_mk_pkt_buff + (sizeof(LILA_PKT_DATA__DATA_T) * lila_dtg_data_cnt));


    if (tmp_mk_pkt_buff == NULL)
        return LILA_PKT_RET__FAIL;

    memset(tmp_mk_pkt_buff, 0x00, lila_pkt_size);

    // make header..
    p_lila_pkt_frame_header->pkt_prefix = LILA_PKT_PREFIX;
    p_lila_pkt_frame_header->dev_id = LILA_PKT_ID__DEVICE_ID;
    p_lila_pkt_frame_header->phone_no = lila_tools__get_phonenum_int_type();
    strcpy(p_lila_pkt_frame_header->pkt_command, lila_tools__conv_buff_to_str_reverse(LILA_PKT_ID__DATA_SEND_SYNC, strlen(LILA_PKT_ID__DATA_SEND_SYNC)));
    p_lila_pkt_frame_header->idx = pkt_idx ;
    p_lila_pkt_frame_header->data_cnt = lila_dtg_data_cnt;
    p_lila_pkt_frame_header->data_len = sizeof(LILA_PKT_DATA__DTG_INFO_T) * lila_dtg_data_cnt;

    for ( i = 0 ; i < lila_dtg_data_cnt; i ++ ) 
    {
        if(list_pop(&lila_dtg_data_buffer_list, (void *)&p_tmp_lila_pkt_data) < 0)
            return -1;
        
        memcpy(p_lila_pkt_data, p_tmp_lila_pkt_data, sizeof(LILA_PKT_DATA__DATA_T));
        /*
        if ( p_tmp_lila_pkt_data != NULL)
        {
            printf("p_tmp_lila_pkt_data is not null ... try free() [%d]\r\n", *p_tmp_lila_pkt_data);
            free(p_tmp_lila_pkt_data);
        }
        */
        free(p_tmp_lila_pkt_data);
        // lila_dtg_debug__print_dtg_data(p_lila_pkt_data);
        p_lila_pkt_data++;
    }

    // tail..
    p_lila_pkt_frame_tail->crc = lila_tools__crc16_ccitt(tmp_mk_pkt_buff, (lila_pkt_size - sizeof(LILA_PKT_FRAME__TAIL_T))) ;
    p_lila_pkt_frame_tail->pkt_suffix = LILA_PKT_SUFFIX;

    // lila_dtg_debug__print_frame_tail(p_lila_pkt_frame_tail); // NOTE: debug

    *pbuf = (unsigned char *)tmp_mk_pkt_buff;
    *packet_len = lila_pkt_size;

    return lila_pkt_size;
}


int lila_dtg__convert_and_insert_bulk_stream(char* stream, int len, char** buf)
{
    int cnt = 0;
    int pack_buffer_size = 0;
    // int send_pack_size = 0;
    // unsigned char* dtg_pack_buf = NULL;

    char* p_idx_stream = stream;

    int i = 0 ;

    cnt = (len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t) ;
    p_idx_stream += sizeof(tacom_std_hdr_t);

    for (i = 0 ; i < cnt ; i ++)
    {
        tacom_std_data_t* p_tmp_std_data = NULL;
        p_tmp_std_data = (tacom_std_data_t*)p_idx_stream;
        lila_dtg__insert_dtg_data(p_tmp_std_data);
        p_idx_stream += sizeof(tacom_std_data_t);
    }

	return pack_buffer_size;
}


int lila_dtg__convert_dtg_data_to_pkt()
{
    char * tmp_bluk_stream_buff = NULL;
    char * tmp_mk_pkt_buff = NULL;

    int taco_data_size = 0;
    int disc_pkt_size = 0;

    if ( (taco_data_size = taco_gtrack_tool__get_bulk_data(LILA_DTG_PKT_BULK_CHK_CNT, &tmp_bluk_stream_buff)) > 0 )
    {
        disc_pkt_size = lila_dtg__convert_and_insert_bulk_stream(tmp_bluk_stream_buff, taco_data_size, &tmp_mk_pkt_buff);
        
        free(tmp_bluk_stream_buff);
    }

    return 0;
}


int lila_dtg__clr_dtg_taco_buff()
{
    char * tmp_bluk_stream_buff = NULL;
    char * tmp_cur_stream_buff = NULL;
    // char * tmp_mk_pkt_buff = NULL;

    int taco_data_size = 0;
    int ret_bufflen = 0;
 
    if ( (taco_data_size = taco_gtrack_tool__get_bulk_data(10, &tmp_bluk_stream_buff)) > 0 )
    {
        // first current data set
        if (( ret_bufflen = taco_gtrack_tool__get_current_data(&tmp_cur_stream_buff) ) > 0 )
        {
            //set_current_dtg_data((unsigned char*)tmp_cur_stream_buff, ret_bufflen);
            free(tmp_cur_stream_buff);
        }

        free(tmp_bluk_stream_buff);
    }
    
    return 0;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
