#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "packet.h"
#include <base/gpstool.h>
#include <base/sender.h>
#include <board/battery.h>

#include "mds.h"
#include "mds_ctx.h"
#include "netcom.h"

#include "logd/logd_rpc.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

int _get_cur_packet_free_cnt(unsigned int packet);

// --------------------------------------------------------
//  global variable
// --------------------------------------------------------

static PKT_CONTEXT_T pkt_ctx;

static TRIPHOS_PACKET_FRAME__MDT_KEYON 	mdt_packet_frame_keyon[MDT_DATA_PACKET__MAX_FRAME];
static TRIPHOS_PACKET_FRAME__MDT_KEYOFF mdt_packet_frame_keyoff[MDT_DATA_PACKET__MAX_FRAME];

// ----------------------------------------------
// context
// ----------------------------------------------

int set_pkt_ctx_deivce_phone_num(char* phone_num, int size)
{
	// argument TODO Error 처리
	if (size != PACKET_DEFINE_TEL_NO_LEN)
	{
		//printf("%s %d line : Error(%d) \r\n","set_pkt_ctx_deivce_phone_num", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}

	strncpy(pkt_ctx.device_phone_num, phone_num, size);
	
	LOGI(LOG_TARGET, "set context - dial [%s]\n", pkt_ctx.device_phone_num);
	
	return PACKET_RET_SUCCESS;
}


int set_pkt_ctx_keyon_time(int year, int month, int day, int hour, int min, int sec)
{
	// year 값은 xxxx 이어야한다.
	if (year < 1900)
	{
		//printf("%s %d line : Error(%d) \r\n","set_pkt_ctx_keyon_time", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
	
	pkt_ctx.keyon_time.time_YY = (year - 1900) % 100;
	pkt_ctx.keyon_time.time_MM = month ;
	pkt_ctx.keyon_time.time_DD = day;
	pkt_ctx.keyon_time.time_hour = hour;
	pkt_ctx.keyon_time.time_min = min;
	pkt_ctx.keyon_time.time_sec = sec;
	pkt_ctx.keyon_time.time_mil_sec = 0;

	LOGI(LOG_TARGET, "set context - keyon time [%d/%d/%d] %d:%d:%d.%d\r\n", 
									pkt_ctx.keyon_time.time_YY, 
									pkt_ctx.keyon_time.time_MM,
									pkt_ctx.keyon_time.time_DD,
									pkt_ctx.keyon_time.time_hour,
									pkt_ctx.keyon_time.time_min,
									pkt_ctx.keyon_time.time_sec,
									pkt_ctx.keyon_time.time_mil_sec);
	
	return PACKET_RET_SUCCESS ;
}

int set_pkt_ctx_keyoff_time(int year, int month, int day, int hour, int min, int sec)
{
	// year 값은 xxxx 이어야한다.
	if (year < 1900)
	{
		//printf("%s %d line : Error(%d) \r\n","set_pkt_ctx_keyoff_time", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
	
	pkt_ctx.keyoff_time.time_YY = (year - 1900) % 100;
	pkt_ctx.keyoff_time.time_MM = month ;
	pkt_ctx.keyoff_time.time_DD = day;
	pkt_ctx.keyoff_time.time_hour = hour;
	pkt_ctx.keyoff_time.time_min = min;
	pkt_ctx.keyoff_time.time_sec = sec;
	pkt_ctx.keyoff_time.time_mil_sec = 0;
	
	LOGI(LOG_TARGET, "set context - keyoff time [%d/%d/%d] %d:%d:%d.%d\r\n", 
									pkt_ctx.keyoff_time.time_YY, 
									pkt_ctx.keyoff_time.time_MM,
									pkt_ctx.keyoff_time.time_DD,
									pkt_ctx.keyoff_time.time_hour,
									pkt_ctx.keyoff_time.time_min,
									pkt_ctx.keyoff_time.time_sec,
									pkt_ctx.keyoff_time.time_mil_sec);
	
	return PACKET_RET_SUCCESS;
}



// -------------------------------------
// packet make util
// -------------------------------------

int __make_packet_telno(unsigned char * buf, const char * telno)
{
	// telno size check
	int telno_size = strlen(telno);

	if (telno_size != PACKET_DEFINE_TEL_NO_LEN)
	{
		//printf("%s %d line : Error(%d) \r\n","__make_packet_telno", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}

	strncpy(buf, telno, PACKET_DEFINE_TEL_NO_LEN);
	/*
	for ( i = 0 ; i < PACKET_DEFINE_TEL_NO_LEN ; i++)
	{
		printf("__make_packet_telno [%d]\r\n", i);
		buf[i] = telno[i];
	}
	*/
	return PACKET_RET_SUCCESS;
}


int __make_packet_keyon_time(TRIPHOS_COMMON_PACKET_TIME *time_keyon)
{

	time_keyon->time_YY = pkt_ctx.keyon_time.time_YY;
	time_keyon->time_MM = pkt_ctx.keyon_time.time_MM;
	time_keyon->time_DD = pkt_ctx.keyon_time.time_DD;
	time_keyon->time_hour = pkt_ctx.keyon_time.time_hour;
	time_keyon->time_min = pkt_ctx.keyon_time.time_min;
	time_keyon->time_sec = pkt_ctx.keyon_time.time_sec;
	time_keyon->time_mil_sec = 0;

	return PACKET_RET_SUCCESS;
}

int __make_packet_keyoff_time(TRIPHOS_COMMON_PACKET_TIME *time_keyoff)
{
	time_keyoff->time_YY = pkt_ctx.keyoff_time.time_YY;
	time_keyoff->time_MM = pkt_ctx.keyoff_time.time_MM;
	time_keyoff->time_DD = pkt_ctx.keyoff_time.time_DD;
	time_keyoff->time_hour = pkt_ctx.keyoff_time.time_hour;
	time_keyoff->time_min = pkt_ctx.keyoff_time.time_min;
	time_keyoff->time_sec = pkt_ctx.keyoff_time.time_sec;
	time_keyoff->time_mil_sec = 0;
	
	return PACKET_RET_SUCCESS;
}

int __make_packet_main_pwr_volt(unsigned short * mainpwr_volt)
{
	int voltage;
	
	voltage = battery_get_battlevel_car();
//	printf("voltage is [%d]\r\n",voltage);
	
	if (voltage > 0)
		* mainpwr_volt = htons((unsigned short)voltage/100);
	else
		* mainpwr_volt = htons((unsigned short)0);
		
	return PACKET_RET_SUCCESS;
}

int __make_packet_keyon_internal(unsigned short *interval)
{
	// TODO : get contex api
	unsigned short keyon_interval = get_ctx_keyon_send_to_data_interval() / 60;
	
	*interval = htons((unsigned short)keyon_interval);
	
	return PACKET_RET_SUCCESS;
}
	
	
	
// -------------------------------------------------------------
// MDT keyon packet make
// -------------------------------------------------------------

int _make_mdt_keyon_packet_frame__clr_frame(unsigned int packet_index)
{	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyon_packet_frame__clr_frame", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
	
	memset(&mdt_packet_frame_keyon[packet_index], 0x00, sizeof(TRIPHOS_PACKET_FRAME__MDT_KEYON));
	
	mdt_packet_frame_keyon[packet_index].status = MDT_DATA_PACKET__STATUS_CLEAR;
	mdt_packet_frame_keyon[packet_index].body_cnt = 0;
	
	return PACKET_RET_SUCCESS;
}


int _make_mdt_keyon_packet_frame__data_packet(unsigned int packet_index, gpsData_t * gpsdata)
{
	int body_count = 0;
	int insert_body_index = 0;
	int free_cnt = _get_cur_packet_free_cnt(MDT_PACKET_KEYON);
	
	TRIPHOS_DATA_PACKET__MDT_KEYON_DATA * pdata;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyon_packet_frame__data_packet", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	body_count = mdt_packet_frame_keyon[packet_index].body_cnt;
	insert_body_index = body_count;
	
	if (body_count > MDT_DATA_PACKET__MAX_KEYON_DATA_PACKET)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyon_packet_frame__data_packet", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}

	pdata = &mdt_packet_frame_keyon[packet_index].body[insert_body_index];
	
	// mileage
	pdata->mileage_day = htons((unsigned short)PACKET_DEFINE_MILEAGE_NO);
	pdata->mileage_total = htonl((unsigned long)PACKET_DEFINE_MILEAGE_NO);
	
	// time time data
	pdata->cur_time.time_YY 	= gpsdata->year % 100;
	pdata->cur_time.time_MM 	= gpsdata->mon;
	pdata->cur_time.time_DD 	= gpsdata->day;
	pdata->cur_time.time_hour 	= gpsdata->hour;
	pdata->cur_time.time_min 	= gpsdata->min;
	pdata->cur_time.time_sec 	= gpsdata->sec;
	
	LOGT(LOG_TARGET, " -- [%d/%d] mk keyon pkt [%d/%d/%d] %d:%d:%d.%d (%f.%f)\r\n", 
									packet_index,
									free_cnt,
									pdata->cur_time.time_YY, 
									pdata->cur_time.time_MM,
									pdata->cur_time.time_DD,
									pdata->cur_time.time_hour,
									pdata->cur_time.time_min,
									pdata->cur_time.time_sec,
									pdata->cur_time.time_mil_sec,
									gpsdata->lon,
									gpsdata->lat);
	
	pdata->cur_time.time_mil_sec = 0;
	
	// fill gps data
	
	
	pdata->gps_data.gps_speed		= gpsdata->speed;
	pdata->gps_data.gps_longitude	= htonl(gpsdata->lon * 1000000);
	pdata->gps_data.gps_latitude	= htonl(gpsdata->lat * 1000000);
	pdata->gps_data.gps_azimuth		= htons((unsigned short)gpsdata->angle);
	
	// fill tmperature data
	pdata->temperature_A = htons((unsigned short)PACKET_DEFINE_TEMPER_SENSOR_NO * 10);
	pdata->temperature_B = htons((unsigned short)PACKET_DEFINE_TEMPER_SENSOR_NO * 10);
	
	// fill main power
	__make_packet_main_pwr_volt (&pdata->mainpwr_volt);
	
	// increase body_index
	mdt_packet_frame_keyon[packet_index].body_cnt = (body_count + 1) ;
	
	return PACKET_RET_SUCCESS;
}


int _make_mdt_keyon_packet_frame__alloc_and_get_buff(unsigned int packet_index, unsigned short *size, unsigned char **pbuf)
{	
	int body_count = 0;
	int packet_size = 0 ;

	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	body_count = mdt_packet_frame_keyon[packet_index].body_cnt;
	
	if (body_count <= 0)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
		
	if (body_count > MDT_DATA_PACKET__MAX_KEYON_DATA_PACKET)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
	
	if (mdt_packet_frame_keyon[packet_index].status != MDT_DATA_PACKET__STATUS_PREPARE_COMPLETE)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE);
		return PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE;
	}
	
	packet_size = (sizeof(TRIPHOS_HEAD_PACKET__MDT_KEYON_DATA) + (sizeof(TRIPHOS_DATA_PACKET__MDT_KEYON_DATA) * body_count ));
	
	
	
	*size = packet_size;
	//printf("size is [%d]\r\n", *size);
	//phead->body_cnt = htons((unsigned short)1);
	
	*pbuf = malloc (packet_size);
	memcpy(*pbuf, &mdt_packet_frame_keyon[packet_index], packet_size);
	
	LOGI(LOG_TARGET, "-- key on --> [%d] alloc buffer bodycnt [%d] / packet_size [%d]\r\n",packet_index, body_count , packet_size);
	
	_make_mdt_keyon_packet_frame__clr_frame(packet_index);
	
	return PACKET_RET_SUCCESS;
}

int _make_mdt_keyon_packet_frame__complete_packet(unsigned int packet_index)
{
	int body_count = 0;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyon_packet_frame__complete_packet", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	body_count = mdt_packet_frame_keyon[packet_index].body_cnt;

	if (body_count <= 0)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyon_packet_frame__complete_packet", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
		
	if (body_count > MDT_DATA_PACKET__MAX_KEYON_DATA_PACKET)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyon_packet_frame__complete_packet", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
	
	// fill protocol
	mdt_packet_frame_keyon[packet_index].header.protocol_id = MDT_DATA_SEND;
	
	// fill tel no
	__make_packet_telno(mdt_packet_frame_keyon[packet_index].header.tel_no, pkt_ctx.device_phone_num);
	
	// fill interval min 
	__make_packet_keyon_internal(&mdt_packet_frame_keyon[packet_index].header.send_interval_min);
	
	// fill keyon time
	__make_packet_keyon_time( &mdt_packet_frame_keyon[packet_index].header.keyon_time );
	
	// fill body count
	mdt_packet_frame_keyon[packet_index].header.body_cnt = htons(body_count);

	mdt_packet_frame_keyon[packet_index].status = MDT_DATA_PACKET__STATUS_PREPARE_COMPLETE;
	
	//printf(" -- make Keyon Packet : Body count is [%d]\r\n", body_count);
	
	return PACKET_RET_SUCCESS;
}


// -------------------------------------------------------------
// MDT keyoff packet make
// -------------------------------------------------------------

int _make_mdt_keyoff_packet_frame__clr_frame(unsigned int packet_index)
{		
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__clr_frame", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
	
	memset(&mdt_packet_frame_keyoff[packet_index], 0x00, sizeof(TRIPHOS_PACKET_FRAME__MDT_KEYOFF));
	
	mdt_packet_frame_keyoff[packet_index].status = MDT_DATA_PACKET__STATUS_CLEAR;
	mdt_packet_frame_keyoff[packet_index].body_cnt = 0;
	
	return PACKET_RET_SUCCESS;
}

int _make_mdt_keyoff_packet_frame__data_packet(unsigned int packet_index, gpsData_t * gpsdata)
{
	int body_count = 0;
	int insert_body_index = 0;
	int free_cnt = _get_cur_packet_free_cnt(MDT_PACKET_KEYOFF);
	
	TRIPHOS_DATA_PACKET__MDT_KEYOFF_DATA * pdata;
	
	//printf("make_mdt_packet_frame__data_packet index is [%d]\r\n", body_index);
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__data_packet", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}	
	
	body_count = mdt_packet_frame_keyoff[packet_index].body_cnt;
	insert_body_index = body_count;
	
	if (body_count > MDT_DATA_PACKET__MAX_KEYOFF_DATA_PACKET)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__data_packet", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
		
	pdata = &mdt_packet_frame_keyoff[packet_index].body[insert_body_index];
	
	// time time data
	pdata->cur_time.time_YY 	= gpsdata->year % 100;
	pdata->cur_time.time_MM 	= gpsdata->mon;
	pdata->cur_time.time_DD 	= gpsdata->day;
	pdata->cur_time.time_hour 	= gpsdata->hour;
	pdata->cur_time.time_min 	= gpsdata->min;
	pdata->cur_time.time_sec 	= gpsdata->sec;
	
	LOGT(LOG_TARGET, " -- [%d/%d] mk keyoff pkt [%d/%d/%d] %d:%d:%d.%d (%f.%f)\r\n", 
									packet_index,
									free_cnt,
									pdata->cur_time.time_YY, 
									pdata->cur_time.time_MM,
									pdata->cur_time.time_DD,
									pdata->cur_time.time_hour,
									pdata->cur_time.time_min,
									pdata->cur_time.time_sec,
									pdata->cur_time.time_mil_sec,
									gpsdata->lon,
									gpsdata->lat);
	
	pdata->cur_time.time_mil_sec = 0;
	
	// fill gps data
	pdata->gps_longitude	= htonl(gpsdata->lon * 1000000);
	pdata->gps_latitude	= htonl(gpsdata->lat * 1000000);

	
	// fill votage
	__make_packet_main_pwr_volt (&pdata->mainpwr_volt);

	// increase body_index
	mdt_packet_frame_keyoff[packet_index].body_cnt = (body_count + 1) ;

	return PACKET_RET_SUCCESS;
}

int _make_mdt_keyoff_packet_frame__alloc_and_get_buff(unsigned int packet_index, unsigned short *size, unsigned char **pbuf)
{
	int body_count = 0;
	int packet_size = 0 ;
	
	//printf(" - %s : [%d]\r\n", __FUNCTION__, body_count);
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__alloc_and_get_buff", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
	
	body_count = mdt_packet_frame_keyoff[packet_index].body_cnt;
	
	if (body_count <= 0)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__alloc_and_get_buff", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
	
	if (body_count > MDT_DATA_PACKET__MAX_KEYOFF_DATA_PACKET)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__alloc_and_get_buff", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
	
	
	if (mdt_packet_frame_keyoff[packet_index].status != MDT_DATA_PACKET__STATUS_PREPARE_COMPLETE)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__alloc_and_get_buff", __LINE__, PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE);
		return PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE;
	}
	
	packet_size = (sizeof(TRIPHOS_DATA_PACKET__MDT_KEYOFF_DATA) * body_count );
	
	//printf("alloc_and_get_buff bodycnt  [%d] / packet_size [%d]\r\n", body_count , packet_size);
	//printf("alloc_and_get_buff - KEYOFF :  bodycnt  [%d] / packet_size [%d]\r\n", body_count , packet_size);
	
	*size = packet_size;
	//printf("size is [%d]\r\n", *size);
	//phead->body_cnt = htons((unsigned short)1);
	
	*pbuf = malloc (packet_size);
	memcpy(*pbuf, &mdt_packet_frame_keyoff[packet_index], packet_size);
	
	LOGI(LOG_TARGET, "-- key off --> [%d] alloc buffer bodycnt [%d] / packet_size [%d]\r\n",packet_index, body_count , packet_size);
	
	_make_mdt_keyoff_packet_frame__clr_frame(packet_index);
	
	return PACKET_RET_SUCCESS;
}


int _make_mdt_keyoff_packet_frame__complete_packet(unsigned int packet_index)
{
	int body_count = 0;
	int body_index = 0;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
		
	body_count = mdt_packet_frame_keyoff[packet_index].body_cnt;
	
	if (body_count <= 0)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__complete_packet", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
		
	if (body_count > MDT_DATA_PACKET__MAX_KEYOFF_DATA_PACKET)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__complete_packet", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
		
	body_index = body_count - 1;
	//printf(" -- make Keyoff Packet : Body count is [%d]\r\n", body_count);
	
	if (body_index > MDT_DATA_PACKET__MAX_KEYOFF_DATA_PACKET)
	{
		//printf("%s %d line : Error(%d) \r\n","_make_mdt_keyoff_packet_frame__complete_packet", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
	
	// fill protocol
	mdt_packet_frame_keyoff[packet_index].body[body_index].protocol_id = KEY_OFF_DATA_SEND;
	
	// fill tel no
	__make_packet_telno(mdt_packet_frame_keyoff[packet_index].body[body_index].tel_no, pkt_ctx.device_phone_num);
	
	// fill keyon time
	__make_packet_keyoff_time( &mdt_packet_frame_keyoff[packet_index].body[body_index].keyoff_time );
	
	// calculate total packet size TODO!!!!!!!!!!!!!!!!!
	mdt_packet_frame_keyoff[packet_index].status = MDT_DATA_PACKET__STATUS_PREPARE_COMPLETE;	
	
	return PACKET_RET_SUCCESS;
}

#if 0
int _get_time_cur_packet(unsigned int packet_index)
{
	
	int body_count = 0;
	int body_index = 0;
	
	TRIPHOS_COMMON_PACKET_TIME *packet_time;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
		
	// packet_index 에 대한 아규먼트 검사
	
	int total_sec = 0;
	
	if (mdt_packet_frame_keyon[packet_index].status == MDT_DATA_PACKET__STATUS_USED)
	{
		packet_time = &mdt_packet_frame_keyon[packet_index].body[0].cur_time;

		// 단순히 패킷의 [날짜 / 시간 / 분 / 초]를 초로 계산한다.
		total_sec += packet_time->time_DD * 24 * 60 * 60;
		total_sec += packet_time->time_hour * 60 * 60;
		total_sec += packet_time->time_min * 60;
		total_sec += packet_time->time_sec;
	   
		return total_sec;
	}
	else
	{
		return 0;
	}

}
#endif


// --------------------------------------------------------
// packet management
// --------------------------------------------------------
//#define MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT

static __MANAGE_MDT_PACKET_FRAME manage_packet[MDT_PACKET_MAX];
//static __MANAGE_MDT_PACKET_FRAME manage_packet;

int __get_cur_packet_index_front(unsigned int packet)
{
	return manage_packet[packet].front;
}

int __get_cur_packet_index_rear(unsigned int packet)
{
	return manage_packet[packet].rear;
}

int _get_cur_packet_use_cnt(unsigned int packet)
{
	return manage_packet[packet].total_use_cnt;
}

int _get_cur_packet_free_cnt(unsigned int packet)
{
	int count = MDT_DATA_PACKET__MAX_FRAME - manage_packet[packet].total_use_cnt;
	
	return count;
}

int __set_increase_packet_front(unsigned int packet)
{
	int index = manage_packet[packet].front;
	int next_index = ( (index + 1) % MDT_DATA_PACKET__MAX_FRAME );

	
	if ( manage_packet[packet].rear == next_index )
	{
		//printf("%s %d line : Warnning Full packet chain  \r\n","__set_increase_packet_front", __LINE__);
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
		; // TODO NOTHING
#else
		manage_packet[packet].rear = ( (next_index + 1) % MDT_DATA_PACKET__MAX_FRAME );
		manage_packet[packet].front = next_index;
#endif
	}
	else
	{
		manage_packet[packet].front = next_index;
	}
	
	return manage_packet[packet].front ;
}

int __set_increase_packet_rear(unsigned int packet)
{
	int index = manage_packet[packet].rear;
	int next_index = ( (index + 1) % MDT_DATA_PACKET__MAX_FRAME );
	/*
	if ( manage_packet[packet].front == next_index )
	{
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
		; // TODO NOTHING
#else
		manage_packet[packet].front++;
		manage_packet[packet].rear = next_index;
#endif
	}
	else
	{
		manage_packet[packet].rear = next_index;
	}*/
	
	manage_packet[packet].rear = next_index;
	
	return manage_packet[packet].rear;
}

/*
int __set_decrease_packet_rear()
{
	int index = manage_packet.rear;
	
	index--;
	
	if (index < 0)
		manage_packet.rear = (MDT_DATA_PACKET__MAX_FRAME + index);
		
	return manage_packet.rear;
}*/

int __set_increase_use_cnt(int packet)
{
	int count = manage_packet[packet].total_use_cnt;
	
	count ++;

	if (count > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Warnning Full packet chain  \r\n","__set_increase_use_cnt", __LINE__);
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
		return PACKET_RET_FAIL;
#else
		count = MDT_DATA_PACKET__MAX_FRAME;
#endif
	}
	
	// TODO check max buffer
	manage_packet[packet].total_use_cnt = count;
	
	return manage_packet[packet].total_use_cnt;
}

int __set_decrease_use_cnt(int packet)
{
	int count = manage_packet[packet].total_use_cnt;
	
	count --;
	if (count < 0)
	{
		//printf("%s %d line : Warnning Full packet chain  \r\n","__set_decrease_use_cnt", __LINE__);
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
		return PACKET_RET_FAIL;
#else
		count = 0;
#endif
	}	
	// TODO check max buffer
	manage_packet[packet].total_use_cnt = count;
	
	return manage_packet[packet].total_use_cnt;
}



int make_packet_keyon_data_insert(gpsData_t* pgpsdata)
{
	unsigned int packet_index = __get_cur_packet_index_front(MDT_PACKET_KEYON);
	int free_cnt = _get_cur_packet_free_cnt(MDT_PACKET_KEYON);
	
	static int last_packet_index = 0;
	
	int ret = PACKET_RET_FAIL;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","make_packet_keyon_data_insert", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
		

	if (last_packet_index != packet_index)
	{
		//printf("packet index is change... clear packet index\r\n");
		last_packet_index = packet_index;
		_make_mdt_keyon_packet_frame__clr_frame(packet_index);
	}
	
	LOGD(LOG_TARGET, " -- key on : make packet index [%d] / free cnt [%d]\r\n", packet_index, free_cnt);
	
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	if ( free_cnt > 0 )
	{
		ret = _make_mdt_keyon_packet_frame__data_packet(packet_index, pgpsdata);
	}
	else
	{
		ret = PACKET_RET_FAIL;
	}
#else

	ret = _make_mdt_keyon_packet_frame__data_packet(packet_index, pgpsdata);
#endif
		
	return ret;
}



int make_packet_keyon_data_done()
{
	unsigned int packet_index = __get_cur_packet_index_front(MDT_PACKET_KEYON);
	//int free_cnt = _get_cur_packet_free_cnt(MDT_PACKET_KEYON);
	
	int ret = PACKET_RET_FAIL;
	
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","\make_packet_keyon_data_done", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	LOGI(LOG_TARGET, " -- keyon pkt done [%d]\r\n", packet_index);
	//printf("- %s : packet_index [%d]\r\n",__FUNCTION__, packet_index);
	
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	if ( free_cnt > 0 )
	{
		ret = _make_mdt_keyon_packet_frame__complete_packet(packet_index);
		
		if (ret == PACKET_RET_SUCCESS)
		{
			__set_increase_packet_front(MDT_PACKET_KEYON);
			__set_increase_use_cnt(MDT_PACKET_KEYON);
		}
	}
	else
	{
		ret = PACKET_RET_FAIL;
	}
#else

	ret = _make_mdt_keyon_packet_frame__complete_packet(packet_index);

	if (ret == PACKET_RET_SUCCESS)
	{
		__set_increase_packet_front(MDT_PACKET_KEYON);
		__set_increase_use_cnt(MDT_PACKET_KEYON);
	}
#endif

	return ret;
	
}

int check_packet_keyon_data_count()
{
	return _get_cur_packet_use_cnt(MDT_PACKET_KEYON);
}

int get_packet_keyon_first(unsigned short *size, unsigned char **pbuf)
{
	unsigned int packet_index = __get_cur_packet_index_rear(MDT_PACKET_KEYON);
	int res = 0;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","get_packet_keyon_first", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
		
	//printf("- %s : packet_index (%d)\r\n","get_packet_keyon_first", packet_index);
	//printf("- %s : use count (%d)\r\n","get_packet_keyon_first", _get_cur_packet_use_cnt(MDT_PACKET_KEYON));
	
	res = _make_mdt_keyon_packet_frame__alloc_and_get_buff(packet_index, size, pbuf);

	if (res == PACKET_RET_SUCCESS)
	{
		__set_increase_packet_rear(MDT_PACKET_KEYON);
		__set_decrease_use_cnt(MDT_PACKET_KEYON);
	}

	return res;
}






int make_packet_keyoff_data_insert(gpsData_t* pgpsdata)
{
	unsigned int packet_index = __get_cur_packet_index_front(MDT_PACKET_KEYOFF);
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	int free_cnt = _get_cur_packet_free_cnt(MDT_PACKET_KEYOFF);
#endif
	
	static int last_packet_index = 0;
	
	int ret = PACKET_RET_FAIL;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","make_packet_keyoff_data_insert", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
	
	if (last_packet_index != packet_index)
	{
		//printf("packet index is change... clear packet index\r\n");
		last_packet_index = packet_index;
		_make_mdt_keyoff_packet_frame__clr_frame(packet_index);
	}
	
	
	
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	if ( free_cnt > 0 )
	{
		ret = _make_mdt_keyoff_packet_frame__data_packet(packet_index, pgpsdata);
	}
	else
	{
		ret = PACKET_RET_FAIL;
	}
#else
	ret = _make_mdt_keyoff_packet_frame__data_packet(packet_index, pgpsdata);
#endif
		
	return ret;
}


int make_packet_keyoff_data_done()
{
	unsigned int packet_index = __get_cur_packet_index_front(MDT_PACKET_KEYOFF);
	//int free_cnt = _get_cur_packet_free_cnt(MDT_PACKET_KEYOFF);
	
	int ret = PACKET_RET_FAIL;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
	{
		//printf("%s %d line : Error(%d) \r\n","make_packet_keyoff_data_done", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
	
//
	LOGI(LOG_TARGET, " -- keyoff pkt done [%d]\r\n", packet_index);
	//printf("- %s : packet_index (%d)\r\n","make_packet_keyoff_data_done", packet_index);
	
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	if ( free_cnt > 0 )
	{
		ret = _make_mdt_keyoff_packet_frame__complete_packet(packet_index);
		
		if (ret == PACKET_RET_SUCCESS)
		{
			__set_increase_packet_front(MDT_PACKET_KEYOFF);
			__set_increase_use_cnt(MDT_PACKET_KEYOFF);
		}
	}
	else
	{
		ret = PACKET_RET_FAIL;
	}
#else


	ret = _make_mdt_keyoff_packet_frame__complete_packet(packet_index);
	
	if (ret == PACKET_RET_SUCCESS)
	{
		__set_increase_packet_front(MDT_PACKET_KEYOFF);
		__set_increase_use_cnt(MDT_PACKET_KEYOFF);
	}
	
#endif

	return ret;
	
}


int check_packet_keyoff_data_count()
{
	return _get_cur_packet_use_cnt(MDT_PACKET_KEYOFF);
}

int get_packet_keyoff_first(unsigned short *size, unsigned char **pbuf)
{
	unsigned int packet_index = __get_cur_packet_index_rear(MDT_PACKET_KEYOFF);
	int res = 0;
	
	if (packet_index > MDT_DATA_PACKET__MAX_FRAME)
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
		
	res = _make_mdt_keyoff_packet_frame__alloc_and_get_buff(packet_index, size, pbuf);
	
	
	if (res == PACKET_RET_SUCCESS)
	{
		__set_increase_packet_rear(MDT_PACKET_KEYOFF);
		__set_decrease_use_cnt(MDT_PACKET_KEYOFF);
	}
	
	return res;
}





int send_keyon_data(const int pkt_count)
{
	int res;
	int count = pkt_count;
	
	if (count <= 0)
		count = 0xffffffff;
		
	if (count > ONCE_SEND_PACKET_MAX_CNT)
		count = ONCE_SEND_PACKET_MAX_CNT;
	
	while (check_packet_keyon_data_count() && count--)
	{
		res = sender_add_data_to_buffer(REPORT_PERIOD_EVENT, NULL, ePIPE_1);
	}
	return res;
}

int send_keyoff_data(const int pkt_count)
{
	int res;
	int count = pkt_count;
	
	if (count <= 0)
		count = 0xffffffff;
		
	if (count > ONCE_SEND_PACKET_MAX_CNT)
		count = ONCE_SEND_PACKET_MAX_CNT;
		
	while (check_packet_keyoff_data_count() && count--)
	{
		res = sender_add_data_to_buffer(REPORT_TURNOFF_EVENT, NULL, ePIPE_1);
	}
	
	return res;
}
