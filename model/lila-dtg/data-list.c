#include <pthread.h>
#include "data-list.h"
#include "debug.h"

listInstance_t lila_dtg_data_buffer_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};


listInstance_t lila_adas_data_buffer_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};


int get_lila_dtg_data_count()
{
	return lila_dtg_data_buffer_list.num;
}


int get_lila_adas_data_count()
{
	return lila_adas_data_buffer_list.num;
}
