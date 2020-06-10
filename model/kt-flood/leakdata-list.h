#ifndef __MODEL_LEAKDATA_LIST_H__
#define __MODEL_LEAKDATA_LIST_H__

#include <util/list.h>

#define LEAKDATA_LIST_INIT "LEAKLISTCLR"

extern listInstance_t leak_data_list;

#define MAX_DEVICE_ID				11
#define INIT_DEVICE_ID				"00000000000"


#define FLOODUNIT_CMD_REQUEST 0x30
#define FLOODUNIT_CMD_LEAK    0x41
#define FLOODUNIT_CMD_EMPTY   0x40


typedef struct {
	unsigned char	cmd;
    unsigned char	deviceid[MAX_DEVICE_ID];
}__attribute__((packed))leak_data_t;

unsigned char	leakdeviceid[MAX_DEVICE_ID];

int get_leak_data_count();

// void * add_leak_empty_list();
void * add_leak_data_list(char * device_id);
void * add_leak_empty_list(unsigned char * pdata);
//void * add_leak_data_list(unsigned char * pdata, char * device_id);

#endif