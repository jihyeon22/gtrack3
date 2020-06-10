#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "ireal.h"
#include "uart.h"
#include "convtools.h"

#define INVALIED_REQCNT		0x10000
#define ERR_REQ	    	    0x20000
#define ERR_RESP 		 	0x40000

#define ERR_IREAL_HDR		0x100
#define ERR_IREAL_CMDTYPE	0x200
#define ERR_IREAL_ECMD		0x400
#define ERR_IREAL_BCC		0x800
#define ERR_IREAL_DATASIZE	0x1000

#define ERR_SYSTEM			0x010
#define ERR_TIMEOUT			0x020

#define SUCCESS				0x0

extern int dtg_uart_fd;
int ireal_errno = 0;

/* error 처리 해야됨 */
int ireal_find(IREAL_DATE *reqdate, IREAL_PACKET *packet)
{

	int ret, retry = IREAL_UART_RETRY;
	int retry_find = 1;
retry_req_find:
	ret = ireal_request(SCMD_SVC_STD, ECMD_FIND, 0x01, reqdate, 0);
	if (ret < 0) {
		ireal_errno |= ERR_REQ;
		return ret;
	}

retry_resp_find:
	ret = ireal_response(packet);
	if (ret < 0) {
		ireal_errno |= ERR_RESP;
		if (ireal_errno & ERR_TIMEOUT) {
			if (retry-- > 0) {
				fprintf(stderr, "retry %d\n", retry);
				goto retry_resp_find;
			} else {
				if (retry_find-- > 0) {
					fprintf(stderr, "timeout flush\n");
					uart_flush(dtg_uart_fd, IREAL_UART_FLUSH_TIME, 0);
					goto retry_req_find;
				}
			}
		} else {
			fprintf(stderr, "flush: %d\n", __LINE__);
			uart_flush(dtg_uart_fd, IREAL_UART_FLUSH_TIME, 0);
		}
		return -1;
	} 

		fprintf(stderr, "1 Reqdate:%02x/%02x/%02x %02x:%02x:%02x.%02x\n", 
				reqdate->Year, 
				reqdate->Mon, 
				reqdate->Day, 
				reqdate->Hour,
				reqdate->Min,
				reqdate->Sec,
				reqdate->mSec);
	if (is_ireal_reqdate_small(reqdate, packet)) {
		ireal_get_date(packet, reqdate);
		fprintf(stderr, "requet date update\n");
		fprintf(stderr, "2 Reqdate:%02x/%02x/%02x %02x:%02x:%02x.%02x\n", 
				reqdate->Year, 
				reqdate->Mon, 
				reqdate->Day, 
				reqdate->Hour,
				reqdate->Min,
				reqdate->Sec,
				reqdate->mSec);
	}
	
	return ret;
}

/* error 처리 해야됨 */
int ireal_read(IREAL_DATE *reqdate, 
		unsigned char reqcnt, IREAL_PACKET *packet)
{

	int ret, retry = IREAL_UART_RETRY;
	int retry_read = 1;

	if (reqcnt < 0 && reqcnt > 60) {
		ireal_errno |= INVALIED_REQCNT;
		return -1;
	}
retry_req_read:
	ret = ireal_request(SCMD_SVC_STD, ECMD_READ, 0x01, reqdate, reqcnt);
	if (ret < 0) {
		ireal_errno |= ERR_REQ;
		return ret;
	}

retry_resp_read:
	ret = ireal_response(packet);
	if (ret < 0) {
		ireal_errno |= ERR_RESP;
		if (ireal_errno & ERR_TIMEOUT) {
			if (retry-- > 0) {
				fprintf(stderr, "retry %d\n", retry);
				goto retry_resp_read;
			} else {
				if (retry_read > 0) {
					retry_read--;
					fprintf(stderr, "timeout flush\n");
					uart_flush(dtg_uart_fd, IREAL_UART_FLUSH_TIME, 0);
					goto retry_req_read;
				}
			}
		} else {
			fprintf(stderr, "flush: %d\n", __LINE__);
			uart_flush(dtg_uart_fd, IREAL_UART_FLUSH_TIME, 0);
		}
		return -1;
	} 

	return ret;
}

/* error 처리 해야됨 */
int ireal_real(IREAL_PACKET *packet)
{

	int ret, retry = IREAL_UART_RETRY;
	int retry_real = 1;

retry_req_real:
	ret = ireal_request(SCMD_SVC_STD, ECMD_REAL, 0x00, NULL, 0);
	if (ret < 0) {
		ireal_errno |= ERR_REQ;
		return ret;
	}

retry_resp_real:
	ret = ireal_response(packet);
	if (ret < 0) {
		ireal_errno |= ERR_RESP;
		if (ireal_errno & ERR_TIMEOUT) {
			if (retry-- > 0) {
				fprintf(stderr, "retry %d\n", retry);
				goto retry_resp_real;
			} else {
				if (retry_real-- > 0) {
					fprintf(stderr, "timeout flush\n");
					uart_flush(dtg_uart_fd, IREAL_UART_FLUSH_TIME, 0);
					goto retry_req_real;
				}
			}
		} else {
			fprintf(stderr, "flush: %d\n", __LINE__);
			uart_flush(dtg_uart_fd, IREAL_UART_FLUSH_TIME, 0);
		}
		return -1;
	} 

	return ret;
}

int ireal_request(uint8_t scmd, uint8_t ecmd, 
		int cdata, IREAL_DATE *date, uint8_t reqcnt)
{
	int i;
	int idx = 0;
	int nbytes;
	unsigned char buf[32] = {0};
	IREAL_HDR *Header;
	short bodylen;
	IREAL_BODYCMD *BodyCmd;
	unsigned char bcc = 0x0;

	/* ireal errno clear */
	ireal_errno = 0;

	/* create request packet buffer */
	/* header */
	Header 		= (IREAL_HDR *)&buf[idx];
	Header->stx 	= HDR_STX;
	Header->magic	= HDR_MAGIC;
	Header->ucode	= HDR_UCODE;
	idx += sizeof(IREAL_HDR);
	
	/* bodylen */
	bodylen = sizeof(IREAL_BODYCMD);
	if (date)
		bodylen += sizeof(IREAL_DATE);
	if (reqcnt)
		bodylen += sizeof(unsigned char);
	buf[idx++] = (char)(bodylen & 0xff);
	buf[idx++] = (char)(bodylen >> 8 & 0xff);
	
	/* body */
	BodyCmd 			= (IREAL_BODYCMD *)&buf[idx];
	BodyCmd->dir_sn 	= 0x00;
	BodyCmd->cmdtype 	= CMD_TYPE_CT_REQ;
	BodyCmd->scmd		= scmd;
	BodyCmd->ecmd		= ecmd;
	BodyCmd->cdata		= cdata;
	idx += sizeof(IREAL_BODYCMD);

	/* body data : date */
	if (date != NULL) {
		memcpy(&buf[idx], date, sizeof(IREAL_DATE));
		idx += sizeof(IREAL_DATE);
	}

	/* body data : reqcnt */
	if (reqcnt) {
		buf[idx++] = reqcnt;
	}

	bcc = buf[0];
	for (i = 1; i < idx; i++) {
		bcc = bcc ^ buf[i];
	}

	buf[idx++] = bcc;
	
	nbytes = uart_write(dtg_uart_fd, buf, idx);
	if (nbytes < 0) {
		ireal_errno = ERR_SYSTEM;
		return nbytes;
	}

	return nbytes;
}

int ireal_response(IREAL_PACKET *packet)
{
	int nbytes;
	int readcnt = 0, remaining = sizeof(IREAL_PACKET);
	char *temp = (char*)packet;

	fd_set reads;
	struct timeval tout = {15, 0};
	
	/* ireal errno clear */
	ireal_errno = 0;

	memset(packet, 0, sizeof(IREAL_PACKET));
retry_read:
	FD_ZERO(&reads);
	FD_SET(dtg_uart_fd, &reads);
	nbytes = select(dtg_uart_fd + 1, &reads, 0, 0, &tout);
	if (nbytes < 0) {
		ireal_errno = ERR_SYSTEM;
		return nbytes;
	} else if (nbytes == 0) {
		/* timeout */
		ireal_errno = ERR_TIMEOUT;
		return -1;
	} else {
		nbytes = read(dtg_uart_fd, temp + readcnt, remaining - readcnt);
		if(nbytes < 0)
			return -1;
	}

	readcnt += nbytes;

	/* we must read header and command field */
	if (readcnt < (sizeof(IREAL_HDR) + 
				   sizeof(short) + 
				   sizeof(IREAL_BODYCMD))) {
		fprintf(stderr, "READ CONTINUE\n");
		goto retry_read;
	}

	/* packet header check */
	if (!(packet->hdr.stx == HDR_STX &&
		  packet->hdr.magic == HDR_MAGIC &&
		  packet->hdr.ucode == HDR_UCODE)) {
		ireal_errno = ERR_IREAL_HDR;
		fprintf(stderr, "packet header error\n");
		return -1;
	}
	
	/* packet data check */
	if (packet->body.cmd.cmdtype == CMD_TYPE_CT_RSP) {
		switch (packet->body.cmd.ecmd)
		{
			case ECMD_TCHEAD: /* header data */
				if (readcnt < sizeof(IREAL_HDR) + 
						sizeof(short) +
						sizeof(IREAL_BODYCMD) + 
						sizeof(IREAL_DATE) + 
						sizeof(char))
				{
					FD_ZERO(&reads);
					FD_SET(dtg_uart_fd, &reads);
					tout.tv_sec = 15;
					tout.tv_usec = 0;
					nbytes = select(dtg_uart_fd + 1, &reads, 0, 0, &tout);
					if (nbytes < 0) {
						ireal_errno = ERR_SYSTEM;
						return nbytes;
					} else if (nbytes == 0) {
						/* timeout */
						ireal_errno = ERR_TIMEOUT;
						return -1;
					} else {
						nbytes = read(dtg_uart_fd, ((char*)packet) + readcnt, remaining - readcnt);
						if(nbytes < 0)
							return -1;
					}
					readcnt += nbytes;
				}
				break;
			case ECMD_TCDATA: /* tc data */
				while(readcnt < sizeof(IREAL_HDR) + 
						sizeof(short) + 
						sizeof(IREAL_BODYCMD) + 
						(sizeof(tacom_ireal_data_t) * packet->body.cmd.cdata) + 
						sizeof(char))
				{
					FD_ZERO(&reads);
					FD_SET(dtg_uart_fd, &reads);
					tout.tv_sec = 15;
					tout.tv_usec = 0;
					nbytes = select(dtg_uart_fd + 1, &reads, 0, 0, &tout);
					if (nbytes < 0) {
						ireal_errno = ERR_SYSTEM;
						return nbytes;
					} else if (nbytes == 0) {
						/* timeout */
						ireal_errno = ERR_IREAL_DATASIZE;
						return -1;
					} else {
						nbytes = read(dtg_uart_fd, ((char*)packet) + readcnt, remaining - readcnt);
						if(nbytes < 0)
							return -1;
					}
					readcnt += nbytes;
				}
				break;
			case ECMD_TCREAL:
				/* 구현해야됨 */
				while (readcnt < sizeof(IREAL_HDR) + 
						sizeof(short) + 
						sizeof(IREAL_BODYCMD) + 
						sizeof(IREAL_DTGREAL) + 
						sizeof(char))
				{
					fprintf(stderr, "REAL [%d/%d] BUFFER[%d/2048] CONTINUE..\n",
							readcnt,
							sizeof(IREAL_HDR) + sizeof(short) + sizeof(IREAL_BODYCMD) + 
							sizeof(IREAL_DTGREAL) + sizeof(char),
							remaining);
					FD_ZERO(&reads);
					FD_SET(dtg_uart_fd, &reads);
					tout.tv_sec = 15;
					tout.tv_usec = 0;
					nbytes = select(dtg_uart_fd + 1, &reads, 0, 0, &tout);
					if (nbytes < 0) {
						ireal_errno = ERR_SYSTEM;
						return nbytes;
					} else if (nbytes == 0) {
						/* timeout */
						ireal_errno = ERR_IREAL_DATASIZE;
						return -1;
					} else {
						nbytes = read(dtg_uart_fd, ((char*)packet) + readcnt, remaining - readcnt);
						if(nbytes < 0)
							return -1;
					}
					readcnt += nbytes;
				}
				break;
			case ECMD_TCNULL:
				/* 구현해야됨 */
				return -1;
			default:
				fprintf(stderr, "INVALIED ECMD\n");
				ireal_errno = ERR_IREAL_ECMD;
				return -1;
		}
	}
	else if (packet->body.cmd.cmdtype == CMD_TYPE_CT_NACK)
	{
		/* 구현해야됨 */
		fprintf(stderr, "CT_NACK\n");
		return -1;
	} 
	else 
	{
		fprintf(stderr, "packet cmdtype error\n");
		ireal_errno = ERR_IREAL_CMDTYPE;
		return -1;
	}
	
	packet->bcc = temp[readcnt - 1];
	temp[readcnt - 1] = 0x0;

	/* bcc check */
	if (bcc_check(packet->bcc, packet, readcnt) == false) {
		ireal_errno = ERR_IREAL_BCC;
		return -1;
	}

	return readcnt;
}

void ireal_get_date(IREAL_PACKET *packet, IREAL_DATE *date)
{
	int i;
	tacom_ireal_hdr_t *tacom_hdr;
	tacom_ireal_data_t  *tacom_data;
	IREAL_DTGREAL *tacom_current_data;
	IREAL_DATE last_date;

	switch (packet->body.cmd.ecmd)
	{
		case ECMD_TCHEAD:
			tacom_hdr = (tacom_ireal_hdr_t *)packet->body.data;
			memcpy(date, &tacom_hdr->Date, sizeof(IREAL_DATE));
			break;

		case ECMD_TCDATA:
			tacom_data = ((tacom_ireal_data_t *)packet->body.data) + (packet->body.cmd.cdata - 1);
			memcpy(date, &tacom_data->Date, sizeof(IREAL_DATE));
			break;
		
		case ECMD_TCREAL:
			tacom_current_data = ((IREAL_DTGREAL *)packet->body.data);
			memcpy(date, &tacom_current_data->DTGBody.Date, sizeof(IREAL_DATE));
			break;

		default:
			break;
	}
}

void ireal_date_add_1sec(IREAL_DATE *date)
{
	unsigned char lastday[] = {31,28,31,30,31,30,31,31,30,31,30,31};
	unsigned char Year = bcdtoi(date->Year);
	unsigned char Mon = bcdtoi(date->Mon);
	unsigned char Day = bcdtoi(date->Day);
	unsigned char Hour = bcdtoi(date->Hour);
	unsigned char Min = bcdtoi(date->Min);
	unsigned char Sec = bcdtoi(date->Sec);
	unsigned char mSec = bcdtoi(date->mSec);

	Sec += 1;
	if (Sec > 59) {	Sec = 0; Min += 1; }
	if (Min > 59) {	Min = 0; Hour += 1; }
	if (Hour > 23) { Hour = 0; Day += 1; }
	if (Day > lastday[Mon - 1]) {	Day = 1; Mon += 1; }
	if (Mon > 12) { Mon = 1; Year += 1; }

	date->Year = itobcd(Year);
	date->Mon = itobcd(Mon);
	date->Day = itobcd(Day);
	date->Hour = itobcd(Hour);
	date->Min = itobcd(Min);
	date->Sec = itobcd(Sec);
	date->mSec = itobcd(mSec);
}

int is_ireal_reqdate_small(IREAL_DATE *date, IREAL_PACKET *packet)
{
	tacom_ireal_hdr_t *phdr = (IREAL_HDR *)packet->body.data;
	IREAL_DATE *pdate = &phdr->Date;
		fprintf(stderr, "packet date:%02x/%02x/%02x %02x:%02x:%02x.%02x\n", 
				pdate->Year, 
				pdate->Mon, 
				pdate->Day, 
				pdate->Hour,
				pdate->Min,
				pdate->Sec,
				pdate->mSec);
	int pdate_ymd = (pdate->Year << 16) + (pdate->Mon  << 8) + pdate->Day;
	int date_ymd = (date->Year << 16) + (date->Mon  << 8) + date->Day;
	if(pdate_ymd < date_ymd)
		return 0;
	else if(pdate_ymd > date_ymd)
		return 1;
	else {
		int pdate_hms = (pdate->Hour << 24) + (pdate->Min  << 16) + 
						(pdate->Sec  << 8) + pdate->mSec;
		int date_hms = (date->Hour << 24) + (date->Min  << 16) + 
						(date->Sec  << 8) + date->mSec;
		if(pdate_hms < date_hms)
			return 0;
		else if(pdate_hms > date_hms)
			return 1;
		else 
			return 0;
	}
}

void ireal_packet_dump(IREAL_PACKET *packet)
{
	int i;
	tacom_ireal_hdr_t  *tacom_hdr;
	tacom_ireal_data_t *tacom_data;
	IREAL_DTGREAL *tacom_real;
	switch (packet->body.cmd.ecmd) 
	{
		case ECMD_TCHEAD:
			tacom_hdr = (tacom_ireal_hdr_t *)packet->body.data;
			break;
		case ECMD_TCDATA:
			for (i = 0; i < packet->body.cmd.cdata; i++) {
				tacom_data = ((tacom_ireal_data_t *)packet->body.data) + i;
			}
			break;
		case ECMD_TCREAL:
			tacom_real = (IREAL_DTGREAL *)packet->body.data;
			break;
		case ECMD_TCNULL:
			break;
		default:
			break;
	}
	fprintf(stderr, "BCC     => [0x%02x]\n", packet->bcc);

}
