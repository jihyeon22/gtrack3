#ifndef __BASE_MILEAGE_H__
#define __BASE_MILEAGE_H__

#include <base/gpstool.h>

#define MILEAGE_PATH "/data/mileage.dat"
#define MILEAGE_CHECK_INTERVAL_SECS 10

typedef struct mileageData mileageData_t;
struct mileageData {
	unsigned int distance_sum;
	int lastyear;
	int lastmon;
	int lastday;
	float lastlat;
	float lastlon;
};

int mileage_process(const gpsData_t *temp_gpsdata);
unsigned int mileage_get_m(void);
void mileage_set_m(const unsigned int val);
int mileage_write(void);
int mileage_read(void);
double get_distance_km(double tlat1, double tlat2, double tlon1, double tlon2);
double get_distance_m(double tlat1, double tlat2, double tlon1, double tlon2);

#endif

