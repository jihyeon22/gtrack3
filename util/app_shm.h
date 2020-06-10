#ifndef __APP_SHM_H__
#define __APP_SHM_H__

#define APP_SHM_KEY		0x1234

#define INIT_DATA		0xAABBCC

void app_shm_init();

int app_shm_set_data(int data);
int app_shm_get_data(int* data);


#endif




