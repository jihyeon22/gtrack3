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

#include <base/gpstool.h>
#include <base/sender.h>
#include <board/battery.h>

#include "pkt_manage.h"
#include "netcom.h"

#include "include/defines.h"

#include "logd/logd_rpc.h"

//#include "kt_fms_packet.h"
// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

// --------------------------------------------------------
//  global variable
// --------------------------------------------------------
static MDS_PACKET_1_MGR 	mds_packet_1_mgr[MDS_PACKET_MGR_MAX_CNT];


int _mds_packet_1_mgr__clr(unsigned int packet_index)
{	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("%s %d line : Error(%d) \r\n","_mds_packet_1_mgr__clr", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
	
	memset(&mds_packet_1_mgr[packet_index], 0x00, sizeof(MDS_PACKET_1_MGR));
	
	mds_packet_1_mgr[packet_index].status = MDS_PACKET_STAT_CLR;
	mds_packet_1_mgr[packet_index].body_cnt = 0;
	
	return PACKET_RET_SUCCESS;
}

int _mds_packet_1_make_and_insert(unsigned int packet_index, char* input_buff, int input_size)
{
	int body_count = 0;
	int insert_body_index = 0;
//	int free_cnt = __get_cur_packet_free_cnt(e_MDS_PKT_1);
	
//	int i = 0;
	
	unsigned char * pdata;
	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("%s %d line : Error(%d) \r\n","_mds_packet_1_make_and_insert", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	body_count = mds_packet_1_mgr[packet_index].body_cnt;
	insert_body_index = body_count;
	
	if (body_count > MDS_PACKET_1_MAX_SAVE_CNT)
	{
		//printf("%s %d line : Error(%d) \r\n","_mds_packet_1_make_and_insert", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}

	pdata = &mds_packet_1_mgr[packet_index].body[insert_body_index];
	
	// -------------------------------------------------------------------------------------------
	// body buffer something to do...
	// -------------------------------------------------------------------------------------------
	memcpy(pdata, input_buff, input_size);
	mds_packet_1_mgr[packet_index].body[insert_body_index].buff_size = input_size;
	
	// increase body_index
	mds_packet_1_mgr[packet_index].body_cnt = ++body_count;
	//printf("%s(%d) body count [%d]\r\n", __func__, __LINE__, mds_packet_1_mgr[packet_index].body_cnt);
	return PACKET_RET_SUCCESS;
}


int _mds_packet_1_alloc_and_get_pkt(unsigned int packet_index, unsigned short *size, unsigned char **pbuf)
{	
	int body_count = 0;
	int packet_size = 0 ;

	int i = 0;
	int cpy_idx = 0;
	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	body_count = mds_packet_1_mgr[packet_index].body_cnt;
	//printf("%s(%d) idx [%d] / body count [%d]\r\n", __func__, __LINE__, packet_index, mds_packet_1_mgr[packet_index].body_cnt);
	//printf("%s(%d) body count [%d]\r\n", __func__, __LINE__, mds_packet_1_mgr[packet_index].body_cnt);
	
	if (body_count <= 0)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
		
	if (body_count > MDS_PACKET_1_MAX_SAVE_CNT)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
	
	if (mds_packet_1_mgr[packet_index].status != MDS_PACKET_STAT_PREPARE_COMPLETE)
	{
		//printf("alloc_and_get_buff : %d line : Error(%d) \r\n", __LINE__, PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE);
		return PACKET_RET_MDT_PAKCET_DATA_NOT_PREPARE;
	}

	// -------------------------------------------------------------------------------------------
	// body buffer something to do...
	// -------------------------------------------------------------------------------------------
	
	for(i = 0; i < body_count; i++)
	{
		packet_size += mds_packet_1_mgr[packet_index].body[i].buff_size;
		//printf("%s(%d) : 1 [%d] / bodycount [%d] \r\n",__func__,__LINE__,mds_packet_1_mgr[packet_index].body[i].buff_size, body_count);
	}
	
	//printf("%s(%d) : 2 [%d] / bodycount [%d] \r\n",__func__,__LINE__,mds_packet_1_mgr[packet_index].head.buff_size, body_count);
	packet_size += mds_packet_1_mgr[packet_index].head.buff_size;
	//printf("%s(%d) : packet_size [%d] \r\n",__func__,__LINE__);
	// 실제 패킷을 넣는곳..
	
	
	// -------------------------------------------------------------------------------------------
	// -------------------------------------------------------------------------------------------
	
	
	*size = packet_size;
	//printf("size is [%d]\r\n", *size);
	//phead->body_cnt = htons((unsigned short));
	
	*pbuf = malloc (packet_size);
	
	// 먼저, 해더정보를 카피하고..
	memcpy(*pbuf + cpy_idx, &mds_packet_1_mgr[packet_index].head.buff, mds_packet_1_mgr[packet_index].head.buff_size);
	
	// 인덱스를 카피한 만큼 옮긴다.
	cpy_idx += mds_packet_1_mgr[packet_index].head.buff_size;
	
	// 각 body 를 카피한다.
	for(i = 0; i < body_count; i++)
	{
		memcpy(*pbuf + cpy_idx, &mds_packet_1_mgr[packet_index].body[i].buff, mds_packet_1_mgr[packet_index].body[i].buff_size);
		cpy_idx += mds_packet_1_mgr[packet_index].body[i].buff_size;
	}
	
	
	LOGI(LOG_TARGET, "-- key on --> [%d] alloc buffer bodycnt [%d] / packet_size [%d]\r\n",packet_index, body_count , packet_size);
	
	//_mds_packet_1_mgr__clr(packet_index);
	
	return PACKET_RET_SUCCESS;
}

int _mds_packet_1_done(unsigned int packet_index, char* input_buff, int input_size)
{
	int body_count = 0;
	unsigned char * pdata;
	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		printf("%s %d line : Error(%d) \r\n","_mds_packet_1_done", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	body_count = mds_packet_1_mgr[packet_index].body_cnt;

	if (body_count <= 0)
	{
		printf("%s %d line : Error(%d) \r\n","_mds_packet_1_done", __LINE__, PACKET_RET_FAIL);
		return PACKET_RET_FAIL;
	}
		
	if (body_count > MDS_PACKET_1_MAX_SAVE_CNT)
	{
		printf("%s %d line : Error(%d) \r\n","_mds_packet_1_done", __LINE__, PACKET_RET_MDT_PAKCET_DATA_RANGE);
		return PACKET_RET_MDT_PAKCET_DATA_RANGE;
	}
	
	// -------------------------------------------------------------------------------------------
	// body buffer something to do...
	// -------------------------------------------------------------------------------------------
	
	// 실제 패킷을 핸들링
	pdata = &mds_packet_1_mgr[packet_index].head.buff;
	
	memcpy(pdata, input_buff, input_size);
	mds_packet_1_mgr[packet_index].head.buff_size = input_size;
	
	// -------------------------------------------------------------------------------------------
	// ---------------

	mds_packet_1_mgr[packet_index].status = MDS_PACKET_STAT_PREPARE_COMPLETE;
	
	//printf(" -- make Keyon Packet : Body count is [%d]\r\n", body_count);
	
	return PACKET_RET_SUCCESS;
}



// -----------------------------------------------------
// packet mgr wrapper.
// -----------------------------------------------------

int mds_packet_1_make_and_insert(char* input_buff, int input_size)
{
	unsigned int packet_index = __get_cur_packet_index_front(e_MDS_PKT_1);
	int free_cnt = __get_cur_packet_free_cnt(e_MDS_PKT_1);
	
	static int last_packet_index = 0;
	
	int ret = PACKET_RET_FAIL;
	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("%s %d line : Error(%d) \r\n","mds_packet_1_make_and_insert", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	if (last_packet_index != packet_index)
	{
		//printf("packet index is change... clear packet index\r\n");
		last_packet_index = packet_index;
		_mds_packet_1_mgr__clr(packet_index);
	}
	
	LOGD(LOG_TARGET, " -- mds_packet_1 : make packet index [%d] / free cnt [%d]\r\n", packet_index, free_cnt);
	
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	if ( free_cnt > 0 )
	{
		ret = _mds_packet_1_make_and_insert(packet_index, input_buff, input_size);
	}
	else
	{
		ret = PACKET_RET_FAIL;
	}
#else

	ret = _mds_packet_1_make_and_insert(packet_index, input_buff, input_size);
#endif
		
	return ret;
}

int mds_packet_1_make_done(char* input_buff, int input_size)
{
	unsigned int packet_index = __get_cur_packet_index_front(e_MDS_PKT_1);
	//int free_cnt = __get_cur_packet_free_cnt(e_MDS_PKT_1);
	
	int ret = PACKET_RET_FAIL;
	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("%s %d line : Error(%d) \r\n","\mds_packet_1_make_done", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}

	LOGI(LOG_TARGET, " -- keyon pkt done [%d]\r\n", packet_index);
	//printf("- %s : packet_index [%d]\r\n",__FUNCTION__, packet_index);
	
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
	if ( free_cnt > 0 )
	{
		ret = _mds_packet_1_done(packet_index);
		
		if (ret == PACKET_RET_SUCCESS)
		{
			__set_increase_packet_front(e_MDS_PKT_1);
			__set_increase_use_cnt(e_MDS_PKT_1);
		}
	}
	else
	{
		ret = PACKET_RET_FAIL;
	}
#else

	ret = _mds_packet_1_done(packet_index,input_buff, input_size);

	if (ret == PACKET_RET_SUCCESS)
	{
		__set_increase_packet_front(e_MDS_PKT_1);
		__set_increase_use_cnt(e_MDS_PKT_1);
	}
	else
	{
		printf("pkt done fail...\r\n");
	}
#endif

	return ret;
	
}

int mds_packet_1_get_use_cnt()
{
	return __get_cur_packet_use_cnt(e_MDS_PKT_1);
}

int mds_packet_1_get_cur_body_size()
{
	int body_count = 0;
	int packet_size = 0;
	int i = 0;
	
	unsigned int packet_index = __get_cur_packet_index_front(e_MDS_PKT_1);

	body_count = mds_packet_1_mgr[packet_index].body_cnt;

	for(i = 0; i < body_count; i++)
	{
		packet_size += mds_packet_1_mgr[packet_index].body[i].buff_size;
		//printf("%s(%d) : 1 [%d] / bodycount [%d] \r\n",__func__,__LINE__,mds_packet_1_mgr[packet_index].body[i].buff_size, body_count);
	}
	
	return packet_size;
}

int mds_packet_1_clear_rear()
{
	unsigned int packet_index = __get_cur_packet_index_rear(e_MDS_PKT_1);
	int ret = -1;
	int use_cnt = 0;
	
	use_cnt = __get_cur_packet_use_cnt(e_MDS_PKT_1);
	
	printf(" ====== pkt clear :: pkt 1 manage > use cnt is [%d]\r\n",use_cnt);
	
	if ( use_cnt > 0 )
	{
		_mds_packet_1_mgr__clr(packet_index);
	
		__set_increase_packet_rear(e_MDS_PKT_1);
		__set_decrease_use_cnt(e_MDS_PKT_1);
		ret = 0;
	}
	
	return ret;
}

int mds_packet_1_get_rear_idx()
{
	unsigned int packet_index = __get_cur_packet_index_rear(e_MDS_PKT_1);
	return packet_index;
}
 
int mds_packet_1_get_front_pkt(unsigned short *size, unsigned char **pbuf)
{
	unsigned int packet_index = __get_cur_packet_index_rear(e_MDS_PKT_1);
	int res = 0;
	
	if (packet_index > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("%s %d line : Error(%d) \r\n","mds_packet_1_get_front_pkt", __LINE__, PACKET_RET_MDT_PAKCET_FRAME_FULL);
		return PACKET_RET_MDT_PAKCET_FRAME_FULL;
	}
		
	//printf("- %s : packet_index (%d)\r\n","mds_packet_1_get_front_pkt", packet_index);
	//printf("- %s : use count (%d)\r\n","mds_packet_1_get_front_pkt", __get_cur_packet_use_cnt(e_MDS_PKT_1));
	
	res = _mds_packet_1_alloc_and_get_pkt(packet_index, size, pbuf);

	/*
	// move to packet clear func
	if (res == PACKET_RET_SUCCESS)
	{
		__set_increase_packet_rear(e_MDS_PKT_1);
		__set_decrease_use_cnt(e_MDS_PKT_1);
	}
	*/

	return res;
}




#if 1
int flush_mds_packet_1(const int pkt_count)
{
	int res = -1;
	int count = pkt_count;
	
	int use_count = mds_packet_1_get_use_cnt();
	
	// 네트워크 연결되어있을때만 쏘자
	if ( nettool_get_state() != DEFINES_MDS_OK )
	{
		printf("network is not ready.. skip send. \r\n");
		return 0;
	}
	
	if (mds_packet_1_get_send_stat() != PACKET_TRANSFER_STAT__COMPLETE )
	{
		printf("send pkt is not complete.. skip send. \r\n");
		return 0;
	}
	
	if (count <= 0)
		count = 0xffffffff;
		
	if (count > ONCE_SEND_PACKET_MAX_CNT)
		count = ONCE_SEND_PACKET_MAX_CNT;
	
	printf("%s() use count is [%d]\r\n", __func__, use_count);
	
	if ( use_count <= 0 )
		return res;
	
	// only 1 pkt support
	res = sender_add_data_to_buffer(MDS_PKT_1_EVENT, NULL, ePIPE_1);
	
	#if 0
	while ( use_count-- && count--)
	{
		printf("%s() flush 1 pkt [%d] / [%d]\r\n", __func__, use_count, count);
		res = sender_add_data_to_buffer(MDS_PKT_1_EVENT, NULL, ePIPE_1);
	}
	#endif
	
	return res;
}
#endif

static int g_send_stat = PACKET_TRANSFER_STAT__COMPLETE;
int mds_packet_1_set_send_stat(int stat)
{
	g_send_stat = stat;
	return g_send_stat;
}

int mds_packet_1_get_send_stat()
{
	return g_send_stat;
}


void kt_fms_send_sdr_packet()
{
	// maybe.. this function is called 1sec interval
	/*
	static int time_sec = 1;
	int server_stat = model_kt_fms_get_server_status() ;
	
	LOGD(LOG_TARGET, "PowerOn Routine - send pkt (%d) / %d / %d \r\n", get_ctx_keyon_send_to_data_interval(), server_stat, time_sec);
	
	if (time_sec > get_ctx_keyon_send_to_data_interval())
	{
		switch(server_stat)
		{
			case KT_FMS_UTIL_RET_SERVER_OK:
			{
				LOGI(LOG_TARGET, "send sdr Routine Routine send - server is ok : flush pkt / %d \r\n", time_sec);
				flush_mds_packet_1(0);
				time_sec = 0;
				break;
			}
			case KT_FMS_UTIL_RET_SERVER_ERR_AND_CHK:
			{ 
				LOGI(LOG_TARGET, "send sdr Routine - server is err : one pkt / %d \r\n", time_sec);
				flush_mds_packet_1(1);
				time_sec = 0;
				break;
			}
			default:
			{
				;
			}
		}
	}
	else
	{
		time_sec++;
	}*/
}


