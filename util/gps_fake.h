<<<<<<< HEAD
#ifndef __BASE_GPSTOOL_FAKE_H__
#define __BASE_GPSTOOL_FAKE_H__

#include <time.h>

void gps_valid_data_get(gpsData_t *last);

int gps_valid_data_write(void);
void gps_valid_data_clear(void);
int gps_valid_data_read(void);
void gps_valid_data_set(gpsData_t *last);
void gps_set_curr_data(gpsData_t* in);

#endif // __BASE_GPSTOOL_FAKE_H__

=======
#ifndef __BASE_GPSTOOL_FAKE_H__
#define __BASE_GPSTOOL_FAKE_H__

#include <time.h>

void gps_valid_data_get(gpsData_t *last);

int gps_valid_data_write(void);
void gps_valid_data_clear(void);
int gps_valid_data_read(void);
void gps_valid_data_set(gpsData_t *last);
void gps_set_curr_data(gpsData_t* in);

#endif // __BASE_GPSTOOL_FAKE_H__

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
