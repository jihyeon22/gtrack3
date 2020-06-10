#include <pthread.h>
#include "data-list.h"

listInstance_t katech_packet_list_1 =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};

listInstance_t katech_packet_list_2 =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};

int katech_packet_list_cnt_1()
{
    return katech_packet_list_1.num;
}

int katech_packet_list_cnt_2()
{
    return katech_packet_list_2.num;
}
