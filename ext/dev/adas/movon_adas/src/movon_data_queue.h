<<<<<<< HEAD
#ifndef __MOVON_DATA_QUEUE_H__
#define __MOVON_DATA_QUEUE_H__

#define QUEUE_SIZE 2048
#define QUEUE_RET_SUCCESS   0
#define QUEUE_RET_FAIL      -1

int deQueue(char* data);
void enQueue(char element);
void enQueue_size(char* element, int size);
int get_dataframe_from_Queue(MOVON_DATA_FRAME_T* data);


#endif
=======
#ifndef __MOVON_DATA_QUEUE_H__
#define __MOVON_DATA_QUEUE_H__

#define QUEUE_SIZE 2048
#define QUEUE_RET_SUCCESS   0
#define QUEUE_RET_FAIL      -1

int deQueue(char* data);
void enQueue(char element);
void enQueue_size(char* element, int size);
int get_dataframe_from_Queue(MOVON_DATA_FRAME_T* data);


#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
