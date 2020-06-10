#include <pthread.h>
#include "data-list.h"
//#include "debug.h"

listInstance_t cu_buffer_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};


int get_cu_data_count()
{
	return cu_buffer_list.num;
}
