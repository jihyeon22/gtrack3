#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <float.h>

#include <base/config.h>
#include <base/error.h>
#include <base/devel.h>
#include <board/modem-time.h>
#include <board/crit-data.h>
#include <util/storage.h>
#include <util/tools.h>
#include <util/list.h>
#include <include/defines.h>
#include <config.h>

#include <logd_rpc.h>

#include <netcom.h>

#include <board/board_system.h>
#include <board/power.h>
#include <mdsapi/mds_api.h>

#include <base/gpstool.h>
#include <util/gps_fake.h>

static gpsData_t g_cur_gps_data;

pthread_mutex_t gps_fake_mutex = PTHREAD_MUTEX_INITIALIZER;


void gps_fake_mutex_lock()
{
    pthread_mutex_lock(&gps_fake_mutex);
}

void gps_fake_mutex_unlock()
{
    pthread_mutex_unlock(&gps_fake_mutex);
}

void gps_valid_data_get(gpsData_t *last)
{
    gps_fake_mutex_lock();
    memcpy(last, &g_cur_gps_data, sizeof(gpsData_t));
    gps_fake_mutex_unlock();
}


void gps_get_curr_data(gpsData_t* out)
{
    gps_fake_mutex_lock();
    memcpy(out, &g_cur_gps_data, sizeof(gpsData_t));
    gps_fake_mutex_unlock();
}

void gps_reset(int type)
{

}

int gps_valid_data_write(void)
{
    gps_fake_mutex_lock();
    gps_fake_mutex_unlock();
}


void gps_set_curr_data(gpsData_t* in)
{
    gps_fake_mutex_lock();
    memcpy(&g_cur_gps_data, in, sizeof(gpsData_t));
    gps_fake_mutex_unlock();
}
