#ifndef __KT_FLOOD_UNIT_H__
#define __KT_FLOOD_UNIT_H__

#define UART_DEV_DEFAULT_PATH          "/dev/ttyHSL2"
#define UART_DEV_DEFAULT_BAUDRATE      115200

#define FILE_DIV_VAL		512

#define FLOOD_UNIT_START_FLAG	0x5b
#define FLOOD_UNIT_END_FLAG	    0x5d

#define MAX_SENSOR_CNT     6

char floodunit_sensor[MAX_SENSOR_CNT];
extern char g_sensor_state[MAX_SENSOR_CNT];

int kt_flood_unit_init();
int kt_flood_unit_close();

int kt_flood_unit_state_write();


#endif
