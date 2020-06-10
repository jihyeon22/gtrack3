#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>     // sleep()
#include <sys/ipc.h>
#include <sys/shm.h>

#include <pthread.h>
#include <semaphore.h>

#include <util/app_shm.h>

typedef struct shm_data
{
	int data_mode;
}shm_data;


int shmid = 0;
shm_data* shm_data_p = NULL;

void app_shm_init()
{
	key_t key = APP_SHM_KEY;

	shmid = shmget(key, sizeof(shm_data), IPC_CREAT | 0666);

	if (shmid < 0)
		printf("error : cannot creat shmrn\r\n");

	shm_data_p = (shm_data*)shmat(shmid, NULL, 0);

	if ( shm_data_p == NULL )
		printf("[writeop] error : failed attach memory\r\n");
	
	//data->data_mode = INIT_DATA ;
}


int app_shm_set_data(int data)
{
	/*
	int shmid;
	key_t key = APP_SHM_KEY;

	shm_data* data;
	shmid = shmget(key, sizeof(shm_data), IPC_CREAT | 0666);

	if (shmid < 0)
		printf("error : cannot creat shmrn\r\n");

	data = (shm_data*)shmat(shmid, NULL, 0);

	if ( data == -1 )
		printf("[writeop] error : failed attach memory\r\n");

	data->data_mode = 0 ;
	*/
	if (shm_data_p == NULL)
	{
		printf("shm data set fail...\r\n");
		return -1;
	}
	else
	{
		printf("shm data set \r\n");
		shm_data_p->data_mode = data;
		return 0;
	}
}

int app_shm_get_data(int* data)
{
	if (shm_data_p == NULL)
	{
		printf("shm data get fail...\r\n");
		return -1;
	}
	else
	{
		printf("shm data get \r\n");
		*data = shm_data_p->data_mode;
		return 0;
	}
}

