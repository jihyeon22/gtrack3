#include <stdio.h>
#include <pthread.h>

#include "data-list.h"

listInstance_t tagging_data_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};

int get_tagging_data_count()
{
	return tagging_data_list.num;
}

void print_list(void)
{
	printf("print list : %p %p %d\n", tagging_data_list.head_list, tagging_data_list.tail_list, tagging_data_list.num);
}
