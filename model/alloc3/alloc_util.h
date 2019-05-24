#ifndef __MODEL_ALLOC_UTIL_H__
#define __MODEL_ALLOC_UTIL_H__

#include <stdbool.h>

#define CIRCULATING_BUS_STATUS_FILE	     CONCAT_STR(USER_DATA_DIR, "/circulating_bus_status.dat")

int get_report_interval();
int RoundOff(float x, int dig);
int get_rssi_gps(void);
void getfilenameformat24(char *name, char *path);
int get_circulating_bus(void);
void set_circulating_bus(int circulating);
int load_circulating_bus_info();
int save_circulating_bus_info();


#endif

