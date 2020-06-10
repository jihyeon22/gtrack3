#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <at/at_util.h>
#include <base/config.h>
#include <base/gpstool.h>
#include <base/error.h>
#include <util/transfer.h>
#include "crc.h"
#include "hdlc_async.h"
#include "request.h"
#include "ciprmc.h"

#include <netcom.h>

/**************************************************************
 * request
 **************************************************************/
void request_print_packet(struct request_packet *packet)
{
	struct request_body *body;
	char term[sizeof(body->termId) + 1];
	char *event;

	if(packet == NULL)
	{
		error_critical(eERROR_LOG, "request_print_packet Error");
	}

	body = &packet->body;

	memcpy(term, body->termId, sizeof(body->termId));
	term[sizeof(body->termId)] = '\0';

	switch(body->eventCode) {
		case REQUEST_MILEAGE:
			event = "mileage";
			break;
		case REQUEST_FACTORY:
			event = "factory";
			break;
		case REQUEST_PARTY:
			event = "party";
			break;
		case REQUEST_LOCUS:
			event = "locus";
			break;
		default:
			event = "unknown";
			break;
	}

	printf("\t0x%02x 0x%02x [%s] %d(%s)\n",
	       body->protocolId, body->msgType, term,
	       body->eventCode, event);
	printf("\t[%i,%i] (%f,%f)\n",
	       body->wgs84Y, body->wgs84X,
	       ((float)body->wgs84Y / 10000000),
	       ((float)body->wgs84X / 10000000));
}

void request_print_encoded_packet(const uint8_t *base, int len)
{
	struct request_packet packet;

	if(base == NULL)
	{
		error_critical(eERROR_LOG, "request_print_encoded_packet Error");
	}

	hdlc_async_decode((uint8_t *)&packet, base, len);

	hdlc_async_print_data(base, len);
	request_print_packet(&packet);
}

void response_print_mileage(struct response_mileage *mileage)
{
	printf("\t%um\n", mileage->totalDistance);
}

void response_print_factory(struct response_factory *factory)
{
	printf("\t%um, [%i,%i](%f,%f)\n",
	       factory->radius,
	       factory->wgs84Y, factory->wgs84X,
	       ((float)factory->wgs84Y / 10000000),
	       ((float)factory->wgs84X / 10000000));
}

void response_print_party(struct response_party *party)
{
	char termId[sizeof(party->termId) + 1];

	memcpy(termId, party->termId, sizeof(party->termId));
	termId[sizeof(party->termId)] = '\0';

	printf("\t[%s], %um\n", termId, party->distance);
}

void response_print_locus(struct response_locus *locus)
{
	printf("\t%um, [%i,%i](%f,%f)\n",
	       locus->radius,
	       locus->wgs84Y, locus->wgs84X,
	       ((float)locus->wgs84Y / 10000000),
	       ((float)locus->wgs84X / 10000000));
}

void response_print_packet(struct response_packet *packet)
{
	struct response_body *body;
	char *event;
	char errcode[sizeof(body->errorCode) + 1];
	char errmsg[sizeof(body->errorMsg) + 1];

	if(packet == NULL)
	{
		error_critical(eERROR_LOG, "response_print_packet Error");
	}

	body = &packet->body;

	switch(body->eventCode) {
		case REQUEST_MILEAGE:
			event = "mileage";
			break;
		case REQUEST_FACTORY:
			event = "factory";
			break;
		case REQUEST_PARTY:
			event = "party";
			break;
		case REQUEST_LOCUS:
			event = "locus";
			break;
		default:
			event = "unknown";
			break;
	}

	memcpy(errcode, body->errorCode, sizeof(body->errorCode));
	errcode[sizeof(body->errorCode)] = '\0';

	memcpy(errmsg, body->errorMsg, sizeof(body->errorMsg));
	errmsg[sizeof(body->errorMsg)] = '\0';

	printf("\t0x%02x 0x%02x %d(%s) %s[%s]\n",
	       body->protocolId, body->msgType,
	       body->eventCode, event, errcode, errmsg);
	switch(body->eventCode) {
		case REQUEST_MILEAGE:
			response_print_mileage(&body->mileage);
			break;
		case REQUEST_FACTORY:
			response_print_factory(&body->factory);
			break;
		case REQUEST_PARTY:
			response_print_party(&body->party);
			break;
		case REQUEST_LOCUS:
			response_print_locus(&body->locus);
			break;
		default:
			break;
	}
}

void response_print_encoded_packet(const uint8_t *base, int len)
{
	struct response_packet packet;

	if(base == NULL)
	{
		error_critical(eERROR_LOG, "response_print_encoded_packet Error");
	}

	hdlc_async_decode((uint8_t *)&packet, base, len);

	hdlc_async_print_data(base, len);
	response_print_packet(&packet);
}

int request_make_packet(uint8_t *encbuf,
                        int eventCode, gpsData_t *gpsdata)
{
	struct request_packet packet;
	struct request_body *body;
	int enclen;
	char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
	at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);

	if(encbuf == NULL || gpsdata == NULL)
	{
		error_critical(eERROR_LOG, "request_make_packet Error");
	}

	body = &packet.body;

	body->protocolId = 0x11;
	body->msgType = 0x64;
	memset(body->termId, ' ', sizeof(body->termId));
	memcpy(body->termId, phonenum, strlen(body->termId));
	body->eventCode = eventCode;

	body->wgs84Y = ciprmc_convert_wgs84Y(gpsdata->lat);
	body->wgs84X = ciprmc_convert_wgs84X(gpsdata->lon);

#if 0
	packet.crc = crc16(0, (uint8_t *)body, sizeof(struct request_body));
#endif

	enclen = hdlc_async_encode(encbuf, (uint8_t *)&packet,
	                           sizeof(struct request_packet));

#if 1
	printf("%s():\n", __func__);
	request_print_packet(&packet);
	hdlc_async_print_data(encbuf, enclen);
#endif

	return enclen;
}

int request_blocking(int eventCode, struct response_packet *packet)
{
	transferSetting_t setting = {0};
	gpsData_t gpsdata;
	uint8_t encbuf[128];
	int enclen;

	if((eventCode < 21) || (eventCode > 24)) {
		fprintf(stderr, "%s() error: unknown event code\n", __func__);
		return -1;
	}

	gps_get_curr_data(&gpsdata);
	if(gpsdata.active == 0 && eventCode == 23) {
		fprintf(stderr, "%s() error: no gpsdata\n", __func__);
		return -1;
	}

	enclen = request_make_packet(encbuf, eventCode, &gpsdata);

#if 0
	printf("%s() read:\n", __func__);
	response_print_encoded_packet(encbuf, enclen);
#endif

	transfer_packet_recv(&setting, encbuf, enclen, encbuf, enclen);
	hdlc_async_decode((uint8_t *)packet, encbuf, enclen);

	return 0;
}

