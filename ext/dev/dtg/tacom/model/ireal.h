#ifndef __DTG_IREAL_DEFINE_HEADER__
#define __DTG_IREAL_DEFINE_HEADER__

#include <stdint.h>
#include <stdbool.h>
#include "tacom_ireal_protocol.h"

#define IREAL_UART_RETRY		1
#define IREAL_UART_FLUSH_TIME	5

/* request/response protocol */
#define HDR_STX				0xAA
#define HDR_MAGIC			0xA2
#define HDR_UCODE			0xD0
typedef struct {
	uint8_t stx;
	uint8_t magic;
	uint8_t ucode;
}__attribute__((packed))IREAL_HDR;

#define DIR_SN_INPUT		0x00	
#define DIR_SN_OUTPUT		0x80

#define	CMD_TYPE_CT_REQ		0x10	// request
#define	CMD_TYPE_CT_RSP		0x01	// response
#define	CMD_TYPE_CT_NACK	0xEE	// error response

#define SCMD_SVC_STD		0x30	// standard 1sec
#define SCMD_SVC_TEN		0x31	// 1/100 sec

#define ECMD_FIND			0x30
#define ECMD_READ			0x31	
#define ECMD_REAL			0x32
#define ECMD_TCHEAD			0x40
#define ECMD_TCDATA			0x41
#define ECMD_TCREAL			0x42
#define ECMD_TCNULL			0x00
#define ECMD_TCEMPTY		0x00

#define EDIR_SN				0x1001
#define ECMDTYPE			0x1002
#define ESCMD				0x1003
#define EECMD				0x1004
#define EDATA				0x1005
typedef struct {
	uint8_t dir_sn;
	uint8_t cmdtype;
	uint8_t scmd;
	uint8_t ecmd;
	uint8_t cdata;
}__attribute__((packed))IREAL_BODYCMD;

typedef struct {
	IREAL_BODYCMD cmd;
	uint8_t data[2048];	// ireal datafield 는 최대 60개의 운행기록데이타(30bytes * 60 = 1800 bytes) 까지 요청 가능하다.
} __attribute__((packed))IREAL_BODY;

typedef struct {
	IREAL_HDR 	hdr;
	uint16_t 	bodylen;
	IREAL_BODY 	body;
	uint8_t	    bcc;
}__attribute__((packed))IREAL_PACKET;

extern int ireal_errno;

int ireal_find(IREAL_DATE *reqdate, IREAL_PACKET *packet);
int ireal_read(IREAL_DATE *reqdate, 
		unsigned char reqcnt, IREAL_PACKET *packet);
int ireal_request(uint8_t scmd, uint8_t ecmd, 
		int cdata, IREAL_DATE *date, uint8_t reqcnt);
int ireal_response(IREAL_PACKET *packet);
void ireal_get_date(IREAL_PACKET *packet, IREAL_DATE *date);
void ireal_date_add_1sec(IREAL_DATE *date);
int is_ireal_reqdate_small(IREAL_DATE *date, IREAL_PACKET *packet);

#endif
