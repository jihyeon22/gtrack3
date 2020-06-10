#include <pthread.h>
#include "data-list.h"
#include "debug.h"

listInstance_t body_buffer_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};


int get_body_data_count()
{
	return body_buffer_list.num;
}
