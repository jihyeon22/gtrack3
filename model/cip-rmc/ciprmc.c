#include <include/types.h>
#include "request.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <base/mileage.h>

/**************************************************************
 * ciprmc
 **************************************************************/
#define RoundOff(x, dig)	\
	(floor((x) * pow(10,dig) + 0.5) / pow(10,dig))

INT32 ciprmc_convert_wgs84Y(float degree)
{
	return degree * 10000000;
}

INT32 ciprmc_convert_wgs84X(float degree)
{
	return degree * 10000000;
}

BYTE ciprmc_convert_angle(float azimuth)
{
	int bearing;

	bearing = RoundOff(azimuth, 0);

	if(bearing == 0) {
		return 0;
	} else if((bearing > 0) && (bearing < 90)) {
		return 1;
	} else if(bearing == 90) {
		return 2;
	} else if((bearing > 90) && (bearing < 180)) {
		return 3;
	} else if(bearing == 180) {
		return 4;
	} else if((bearing > 180) && (bearing < 270)) {
		return 5;
	} else if(bearing == 270) {
		return 6;
	} else if((bearing > 270) && (bearing <= 360)) {
		return 7;
	}

	return 0xff;
}

WORD ciprmc_convert_speed(float speed)
{
	return RoundOff(speed, 0);
}

UINT32 ciprmc_convert_distance(float mileage)
{
	return RoundOff(mileage, 0);
}

void ciprmc_set_mileage(void)
{
	struct response_packet packet;
	struct response_body *body;
	char errcode[sizeof(body->errorCode) + 1];
	char errmsg[sizeof(body->errorMsg) + 1];

	while((request_blocking(REQUEST_MILEAGE, &packet)) == -1);

	body = &(packet.body);

	memcpy(errcode, body->errorCode, sizeof(body->errorCode));
	errcode[sizeof(body->errorCode)] = '\0';

	memcpy(errmsg, body->errorMsg, sizeof(body->errorMsg));
	errmsg[sizeof(body->errorMsg)] = '\0';

	mileage_set_m(body->mileage.totalDistance);
	mileage_write();

#if 0
	if(!strncmp(errcode, "0000", 4)) {
		ciprmc_display_mileage();
	} else {
		ciprmc_display_error(errcode, errmsg);
	}
#endif

}
