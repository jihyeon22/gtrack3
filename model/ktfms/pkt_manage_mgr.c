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

#include "logd/logd_rpc.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

// feature.
//#define MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT


// --------------------------------------------------------
// packet management
// --------------------------------------------------------
typedef struct {
	unsigned int front;
	unsigned int rear;
	unsigned int total_use_cnt;
}__MANAGE_MDS_PACKET_FRAME;

static __MANAGE_MDS_PACKET_FRAME manage_packet[e_MDS_PKT_MAX];

int __get_cur_packet_index_front(unsigned int packet)
{
	return manage_packet[packet].front;
}

int __get_cur_packet_index_rear(unsigned int packet)
{
	return manage_packet[packet].rear;
}

int __get_cur_packet_use_cnt(unsigned int packet)
{
	return manage_packet[packet].total_use_cnt;
}

int __get_cur_packet_free_cnt(unsigned int packet)
{
	int count = MDS_PACKET_MGR_MAX_CNT - manage_packet[packet].total_use_cnt;
	
	return count;
}

int __set_increase_packet_front(unsigned int packet)
{
	int index = manage_packet[packet].front;
	int next_index = ( (index + 1) % MDS_PACKET_MGR_MAX_CNT );

	
	if ( manage_packet[packet].rear == next_index )
	{
		//printf("%s %d line : Warnning Full packet chain  \r\n","__set_increase_packet_front", __LINE__);
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
		; // TODO NOTHING
#else
		manage_packet[packet].rear = ( (next_index + 1) % MDS_PACKET_MGR_MAX_CNT );
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
	int next_index = ( (index + 1) % MDS_PACKET_MGR_MAX_CNT );
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
		manage_packet.rear = (MDS_PACKET_MGR_MAX_CNT + index);
		
	return manage_packet.rear;
}*/

int __set_increase_use_cnt(int packet)
{
	int count = manage_packet[packet].total_use_cnt;
	
	count ++;

	if (count > MDS_PACKET_MGR_MAX_CNT)
	{
		//printf("%s %d line : Warnning Full packet chain  \r\n","__set_increase_use_cnt", __LINE__);
#ifdef MDS_DATA_PACKET__INSERT_OPT_FULL_PROTECT
		return PACKET_RET_FAIL;
#else
		count = MDS_PACKET_MGR_MAX_CNT;
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
