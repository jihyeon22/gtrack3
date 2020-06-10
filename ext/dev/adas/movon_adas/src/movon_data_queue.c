<<<<<<< HEAD

#include <stdio.h>
#include <stdlib.h>

#include <logd_rpc.h>

#include "movon_adas.h"
#include "movon_adas_protocol.h"
#include "movon_data_queue.h"

#define FULL_QUEUE_DELETE_AND_INSERT
#define LOG_TARGET eSVC_COMMON

char items[QUEUE_SIZE];

int front = -1, rear =-1;

static int _isFull()
{
    if( (front == rear + 1) || (front == 0 && rear == QUEUE_SIZE-1) ) 
		return 1;
	
    return 0;
}

static int _isEmpty()
{
    if( front == -1 ) 
		return 1;
    return 0;
}

static int _cntQueue()
{
	int cnt = 1;
	int i = 0;
	for( i = front; i!=rear; i=(i+1)%QUEUE_SIZE) 
		cnt++;
	
	return cnt;
}

int deQueue(char* data)
{
    char element;
	
    if( _isEmpty() ) 
	{
        //printf("\n Queue is empty !! \n");
        return QUEUE_RET_FAIL;
    }
		
	element = items[front];
	if (front == rear) {
		front = -1;
		rear = -1;
	} /* Q has only one element, so we reset the queue after dequeing it. ? */
	else {
		front = (front + 1) % QUEUE_SIZE;
		
	}
	//printf("\n Deleted element -> %d \n", element);
    *data = element;
    return QUEUE_RET_SUCCESS;
}


void enQueue(char element)
{
    char tmp_element;
    if( _isFull() ) 
	{
		//printf("\n Queue is full!! \n");
#ifdef FULL_QUEUE_DELETE_AND_INSERT
		deQueue(&tmp_element); // fource delete
#else
		return -1;
#endif
	}
	
	// FORCE insert..
	if (front == -1) 
		front = 0;
	
	rear = (rear + 1) % QUEUE_SIZE;
	items[rear] = element;
	//printf("\n Inserted -> %d", element);
    
}

void enQueue_size(char* element, int size)
{
    int i = 0;

    for ( i = 0 ; i < size ; i ++ )
        enQueue(element[i]);
}


int get_dataframe_from_Queue(MOVON_DATA_FRAME_T* data)
{
    int frame_size = sizeof(MOVON_DATA_FRAME_T); // 26

    char data_buff[512] = {0,};
    char data_one;

    int queue_size = _cntQueue();
    int data_idx = 0;

    int found_data_prefix = 0 ;

    //printf("target frame size case 0 => [%d]\r\n", frame_size);

    //if ( queue_size < frame_size)
    //    printf(" >>> err get data one : [%d][%x][%c] - [%d]\r\n",data_one,data_one,data_one, __LINE__);

    while( queue_size > frame_size)
    {
        if ( deQueue(&data_one) == QUEUE_RET_FAIL )
            break;

        if ( data_one != MOVON_DATA_FRAME__PREFIX )
        {
            //LOGE(LOG_TARGET, " >>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
            queue_size = _cntQueue();
            data_idx = 0;
            continue;
        }
        //printf("00 :  get data one : [%d][%x]\r\n",data_one,data_one);

        data_buff[data_idx++] = data_one;

        if ( deQueue(&data_one) == QUEUE_RET_FAIL )
            break;
        
        if ( data_one != MOVON_DATA_FRAME__DATA1 )
        {
            //LOGE(LOG_TARGET, " >>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
            queue_size = _cntQueue();
            data_idx = 0;
            continue;
        }
        //printf("11 :  get data one : [%d][%x]\r\n",data_one,data_one);
        
        data_buff[data_idx++] = data_one;
        
        if ( deQueue(&data_one) == QUEUE_RET_FAIL )
            break;
        
        if ( data_one != MOVON_DATA_FRAME__DATA2 )
        {
            //LOGE(LOG_TARGET," >>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
            queue_size = _cntQueue();
            data_idx = 0;
            continue;
        }
        //printf("22 : get data one : [%d][%x][%c]\r\n",data_one,data_one,data_one);

        data_buff[data_idx++] = data_one;
        found_data_prefix = 1;
        // printf("get data one : chk success\r\n");
        break;
    }

    if( found_data_prefix == 0 )
    {
        // LOGE(LOG_TARGET, ">>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
        return QUEUE_RET_FAIL;
    }
    
    frame_size -= data_idx;
    //printf("target frame size case 1 => [%d]\r\n", frame_size);

    while(frame_size--)
    {
        deQueue(&data_one);
        data_buff[data_idx++] = data_one;
        //printf("get data one : [%d][%x]\r\n",data_one,data_one);
    }

    memcpy(data, data_buff, sizeof(MOVON_DATA_FRAME_T));

    return QUEUE_RET_SUCCESS;
    
=======

#include <stdio.h>
#include <stdlib.h>

#include <logd_rpc.h>

#include "movon_adas.h"
#include "movon_adas_protocol.h"
#include "movon_data_queue.h"

#define FULL_QUEUE_DELETE_AND_INSERT
#define LOG_TARGET eSVC_COMMON

char items[QUEUE_SIZE];

int front = -1, rear =-1;

static int _isFull()
{
    if( (front == rear + 1) || (front == 0 && rear == QUEUE_SIZE-1) ) 
		return 1;
	
    return 0;
}

static int _isEmpty()
{
    if( front == -1 ) 
		return 1;
    return 0;
}

static int _cntQueue()
{
	int cnt = 1;
	int i = 0;
	for( i = front; i!=rear; i=(i+1)%QUEUE_SIZE) 
		cnt++;
	
	return cnt;
}

int deQueue(char* data)
{
    char element;
	
    if( _isEmpty() ) 
	{
        //printf("\n Queue is empty !! \n");
        return QUEUE_RET_FAIL;
    }
		
	element = items[front];
	if (front == rear) {
		front = -1;
		rear = -1;
	} /* Q has only one element, so we reset the queue after dequeing it. ? */
	else {
		front = (front + 1) % QUEUE_SIZE;
		
	}
	//printf("\n Deleted element -> %d \n", element);
    *data = element;
    return QUEUE_RET_SUCCESS;
}


void enQueue(char element)
{
    char tmp_element;
    if( _isFull() ) 
	{
		//printf("\n Queue is full!! \n");
#ifdef FULL_QUEUE_DELETE_AND_INSERT
		deQueue(&tmp_element); // fource delete
#else
		return -1;
#endif
	}
	
	// FORCE insert..
	if (front == -1) 
		front = 0;
	
	rear = (rear + 1) % QUEUE_SIZE;
	items[rear] = element;
	//printf("\n Inserted -> %d", element);
    
}

void enQueue_size(char* element, int size)
{
    int i = 0;

    for ( i = 0 ; i < size ; i ++ )
        enQueue(element[i]);
}


int get_dataframe_from_Queue(MOVON_DATA_FRAME_T* data)
{
    int frame_size = sizeof(MOVON_DATA_FRAME_T); // 26

    char data_buff[512] = {0,};
    char data_one;

    int queue_size = _cntQueue();
    int data_idx = 0;

    int found_data_prefix = 0 ;

    //printf("target frame size case 0 => [%d]\r\n", frame_size);

    //if ( queue_size < frame_size)
    //    printf(" >>> err get data one : [%d][%x][%c] - [%d]\r\n",data_one,data_one,data_one, __LINE__);

    while( queue_size > frame_size)
    {
        if ( deQueue(&data_one) == QUEUE_RET_FAIL )
            break;

        if ( data_one != MOVON_DATA_FRAME__PREFIX )
        {
            //LOGE(LOG_TARGET, " >>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
            queue_size = _cntQueue();
            data_idx = 0;
            continue;
        }
        //printf("00 :  get data one : [%d][%x]\r\n",data_one,data_one);

        data_buff[data_idx++] = data_one;

        if ( deQueue(&data_one) == QUEUE_RET_FAIL )
            break;
        
        if ( data_one != MOVON_DATA_FRAME__DATA1 )
        {
            //LOGE(LOG_TARGET, " >>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
            queue_size = _cntQueue();
            data_idx = 0;
            continue;
        }
        //printf("11 :  get data one : [%d][%x]\r\n",data_one,data_one);
        
        data_buff[data_idx++] = data_one;
        
        if ( deQueue(&data_one) == QUEUE_RET_FAIL )
            break;
        
        if ( data_one != MOVON_DATA_FRAME__DATA2 )
        {
            //LOGE(LOG_TARGET," >>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
            queue_size = _cntQueue();
            data_idx = 0;
            continue;
        }
        //printf("22 : get data one : [%d][%x][%c]\r\n",data_one,data_one,data_one);

        data_buff[data_idx++] = data_one;
        found_data_prefix = 1;
        // printf("get data one : chk success\r\n");
        break;
    }

    if( found_data_prefix == 0 )
    {
        // LOGE(LOG_TARGET, ">>> err get data one : [%d][%x] - [%d]\r\n" ,data_one,data_one, __LINE__);
        return QUEUE_RET_FAIL;
    }
    
    frame_size -= data_idx;
    //printf("target frame size case 1 => [%d]\r\n", frame_size);

    while(frame_size--)
    {
        deQueue(&data_one);
        data_buff[data_idx++] = data_one;
        //printf("get data one : [%d][%x]\r\n",data_one,data_one);
    }

    memcpy(data, data_buff, sizeof(MOVON_DATA_FRAME_T));

    return QUEUE_RET_SUCCESS;
    
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}