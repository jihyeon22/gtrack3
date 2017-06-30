#include <pthread.h>
#include "data-list.h"

listInstance_t packet_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};
