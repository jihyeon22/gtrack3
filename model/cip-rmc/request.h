#ifndef __MODEL_REQUEST_H__
#define __MODEL_REQUEST_H__

#include <include/types.h>

#define REQUEST_COMM_NAME	"request"

/**************************************************************
 * request
 **************************************************************/
#define REQUEST_MILEAGE		21
#define REQUEST_FACTORY		22
#define REQUEST_PARTY		23
#define REQUEST_LOCUS		24

struct request_packet {
	struct request_body {
		BYTE   protocolId;
		BYTE   msgType;
		CHAR   termId[15];
		BYTE   eventCode;
		INT32  wgs84Y;
		INT32  wgs84X;
	} PACKED(body);
//    WORD crc;
} PACKED();

struct response_mileage {
	UINT32 totalDistance;
} PACKED();

struct response_factory {
	BYTE radius;
	INT32  wgs84X;
	INT32  wgs84Y;
} PACKED();

struct response_party {
	CHAR   termId[10];
	UINT32 distance;
} PACKED();

struct response_locus {
	BYTE   radius;
	INT32  wgs84X;
	INT32  wgs84Y;
} PACKED();

struct response_packet {
	struct response_body {
		BYTE   protocolId;
		BYTE   msgType;
		BYTE   eventCode;
		CHAR   errorCode[4];
		CHAR   errorMsg[32];
		union {
			struct response_mileage mileage;
			struct response_factory factory;
			struct response_party party;
			struct response_locus locus;
		};
	} PACKED(body);
//    WORD crc;
} PACKED();

int request_blocking(int eventCode, struct response_packet *packet);
#endif /* TARGET_CIPRMC_REQUEST_H */
