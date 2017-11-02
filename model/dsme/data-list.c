#include <pthread.h>
#include "data-list.h"


listInstance_t welding_machine_buffer_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};


int get_data_count()
{
	return welding_machine_buffer_list.num;
}
