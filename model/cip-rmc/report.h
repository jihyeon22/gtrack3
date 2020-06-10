#ifndef __MODEL_REPORT_H__
#define __MODEL_REPORT_H__

#include <base/gpstool.h>
#include <include/types.h>

#define REPORT_COMM_NAME	"report"

/**************************************************************
 * report
 **************************************************************/
#define REPORT_SET_IP			1
#define REPORT_SET_INTERVAL		2
#define REPORT_SET_MILEAGE		3
#define REPORT_STATUS			4
#define REPORT_PERIOD_EVENT	5
#define REPORT_POWER_EVENT	6
#define REPORT_TURNON_EVENT	26
#define REPORT_TURNOFF_EVENT	27
#define REPORT_RESET			69
#define REPORT_DEPARTURE_EVENT	90
#define REPORT_RETURN_EVENT	91
#define REPORT_ARRIVAL_EVENT	92
#define REPORT_WAITING_EVENT	93

struct report_config {
	int min_distance;
	int period_interval;
	int send_interval;
};

struct report_packet {
	struct report_body {
		BYTE   protocolId;
		BYTE   msgType;
		CHAR   termId[15];
		BYTE   eventCode;
		WORD   year;
		BYTE   month;
		BYTE   day;
		BYTE   gpsH;
		BYTE   gpsM;
		BYTE   gpsS;
		BYTE   locationType;
		INT32  wgs84Y;
		INT32  wgs84X;
		BYTE   angle;
		WORD   speed;
		UINT32 totalDistance;
		BYTE   powerType;
	} PACKED(body);
	WORD crc;
} PACKED();

int report_make_packet(uint8_t *encbuf, int eventCode, gpsData_t *gpsdata, int powerType);
void report_print_encoded_packet(const uint8_t *base, int len);
#endif /* TARGET_CIPRMC_REPORT_H */
