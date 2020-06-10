#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <include/defines.h>
#include <base/config.h>
#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/mileage.h>
#include <base/error.h>
#include <board/gpio.h>
#include "report.h"
#include "crc.h"
#include "ciprmc.h"
#include "hdlc_async.h"

void report_print_packet(struct report_packet *packet)
{
	struct report_body *body;
	char term[sizeof(body->termId) + 1];
	char *event, *location, *angle, *power;

	if(packet == NULL)
	{
		error_critical(eERROR_LOG, "report_print_packet Error");
	}

	body = &packet->body;

	memcpy(term, body->termId, sizeof(body->termId));
	term[sizeof(body->termId)] = '\0';

	switch(body->eventCode) {
		case REPORT_PERIOD_EVENT:
			event = "period";
			break;
		case REPORT_POWER_EVENT:
			event = "power";
			break;
		case REPORT_TURNON_EVENT:
			event = "turn-on";
			break;
		case REPORT_TURNOFF_EVENT:
			event = "turn-off";
			break;
		case REPORT_DEPARTURE_EVENT:
			event = "departure";
			break;
		case REPORT_RETURN_EVENT:
			event = "return";
			break;
		case REPORT_ARRIVAL_EVENT:
			event = "arrival";
			break;
		case REPORT_WAITING_EVENT:
			event = "waiting";
			break;
		default:
			event = "unknown";
			break;
	}

	location = (body->locationType) ? "GPS" : "Cell-based";

	switch(body->angle) {
		case 0:
			angle = "N-";
			break;
		case 1:
			angle = "NE";
			break;
		case 2:
			angle = "-E";
			break;
		case 3:
			angle = "SE";
			break;
		case 4:
			angle = "S-";
			break;
		case 5:
			angle = "SW";
			break;
		case 6:
			angle = "-W";
			break;
		case 7:
			angle = "NW";
			break;
		default:
			angle = "??";
			break;
	}

	power = (body->powerType) ? "Battery" : "External";

	printf("\t0x%02x 0x%02x [%s] %d(%s)\n",
	       body->protocolId, body->msgType, term,
	       body->eventCode, event);
	printf("\t%4d/%02d/%02d %02d:%02d:%02d %d(%s)\n",
	       body->year, body->month, body->day,
	       body->gpsH, body->gpsM, body->gpsS,
	       body->locationType, location);
	printf("\t[%i,%i] %d(%s) %dkm/h [%um] %d(%s)\n",
	       body->wgs84Y, body->wgs84X,
	       body->angle, angle, body->speed,
	       body->totalDistance, body->powerType, power);
	printf("\t(%f,%f)\n",
	       ((float)body->wgs84Y / 10000000),
	       ((float)body->wgs84X / 10000000));
}

void report_print_encoded_packet(const uint8_t *base, int len)
{
	struct report_packet packet;

	if(base == NULL)
	{
		error_critical(eERROR_LOG, "report_print_encoded_packet Error");
	}

	hdlc_async_decode((uint8_t *)&packet, base, len);

	hdlc_async_print_data(base, len);
	report_print_packet(&packet);
}
int report_make_packet(uint8_t *encbuf,
                       int eventCode, gpsData_t *gpsdata, int powerType)
{
	struct report_packet packet;
	struct report_body *body;
	int enclen;
	char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
	at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);

	if(encbuf == NULL || gpsdata == NULL)
	{
		error_critical(eERROR_LOG, "report_make_packet Error");
	}

	body = &packet.body;

	body->protocolId = 0x11;
	body->msgType = 0x64;
	memset(body->termId, ' ', sizeof(body->termId));
	memcpy(body->termId, phonenum, strnlen(phonenum, sizeof(body->termId)));
	body->eventCode = eventCode;

	body->year = gpsdata->year;
	body->month = gpsdata->mon;
	body->day = gpsdata->day;
	body->gpsH = gpsdata->hour;
	body->gpsM = gpsdata->min;
	body->gpsS = gpsdata->sec;

	if(gpsdata->active == 1)
	{
		body->locationType = 1;
	}
	else
	{
		body->locationType = 2;
	}

	body->wgs84Y = ciprmc_convert_wgs84Y(gpsdata->lat);
	body->wgs84X = ciprmc_convert_wgs84X(gpsdata->lon);
	body->angle = ciprmc_convert_angle(gpsdata->angle);
	body->speed = ciprmc_convert_speed(gpsdata->speed);

	body->totalDistance = ciprmc_convert_distance(mileage_get_m());

	body->powerType = powerType;

	packet.crc = crc16(0, (uint8_t *)body, sizeof(struct report_body));

	enclen = hdlc_async_encode(encbuf, (uint8_t *)&packet,
	                           sizeof(struct report_packet));
#if 0
	exutil_print_time();
	printf("%s():\n", __func__);
	report_print_packet(&packet);
	hdlc_async_print_data(encbuf, enclen);
#endif

	return enclen;
}

