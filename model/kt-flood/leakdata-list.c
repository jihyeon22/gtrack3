#include <stdio.h>
#include <pthread.h>

#include "leakdata-list.h"

listInstance_t leak_data_list =
{
	.head_list = NULL,
	.tail_list = NULL,
	.mutex = PTHREAD_MUTEX_INITIALIZER,
	.num = 0
};

int get_leak_data_count()
{
	return leak_data_list.num;
}

void print_list(void)
{
	printf("print list : %p %p %d\n", leak_data_list.head_list, leak_data_list.tail_list, leak_data_list.num);
}
void * add_leak_empty_list(unsigned char * pdata)
{
	pdata[0] = FLOODUNIT_CMD_EMPTY;

    sprintf(&pdata[1], "%s", INIT_DEVICE_ID);

    return pdata;
}

void * add_leak_data_list(char * device_id)
{
	leak_data_t *pdata = NULL;
    int device_id_len = 0;
 
	pdata = (char *)malloc(sizeof(leak_data_t));
	memset(pdata, 0x00, sizeof(leak_data_t));
	pdata->cmd = FLOODUNIT_CMD_LEAK;

	//strncpy(pdata->deviceid, device_id, INIT_DEVICE_ID);
	//strncpy(pdata->deviceid, device_id, MAX_DEVICE_ID);
	sprintf(pdata->deviceid, "%s", device_id);
	printf("device_id : %s\n", device_id );
	printf("warning_data->deviceid : %s\n", pdata->deviceid );

    return pdata;
}
