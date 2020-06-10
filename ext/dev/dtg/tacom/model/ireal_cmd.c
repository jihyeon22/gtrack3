#include <stdio.h>
#include <unistd.h>
#include <time.h>

#include "ireal.h"
#include <wrapper/dtg_log.h>
#include <tacom_internal.h>
#include <tacom_protocol.h>
#include <convtools.h>

static IREAL_DATE reqdate, lastdate;

void set_reqdate_curr_time()
{
	time_t curr_time;
	struct tm *struct_time;

	time(&curr_time);
	while(curr_time < 0x52000000) {
		time(&curr_time);
		sleep(10);
	}
	struct_time = localtime( &curr_time);
	reqdate.Year	= itobcd(struct_time->tm_year - 100);
	reqdate.Mon		= itobcd(struct_time->tm_mon + 1);
	reqdate.Day		= itobcd(struct_time->tm_mday);
	reqdate.Hour	= itobcd(struct_time->tm_hour);
	reqdate.Min		= itobcd(struct_time->tm_min);
	reqdate.Sec		= itobcd(struct_time->tm_sec);
	reqdate.mSec	= 0x00;
}

int ireal_init_cmd()
{
	int ret;
	int size;
	int retry_read = 0;
	IREAL_PACKET packet;
	FILE *irealfd;

	recvbuf = malloc(1024*512);
	/* read last date file */
	irealfd = fopen("/var/irealreqdate", "rb");
	if (irealfd == NULL) {
		fprintf(stderr, "irealreqdate open failure\n");
		set_reqdate_curr_time();
	} else {
		fprintf(stderr, "irealreqdate open success\n");
		do {
			ret = fread(&reqdate, sizeof(IREAL_DATE), sizeof(IREAL_DATE), irealfd);
			sleep(2);
		} while(ret != sizeof(IREAL_DATE) && retry_read < 5);
		if (ret != sizeof(IREAL_DATE)) {
			set_reqdate_curr_time();
		}
		fclose(irealfd);
	}
	
	/* find last date */
	while (ret = ireal_find(&reqdate, &packet) < 0) {
		/* error */
		printf("find error\n");
		printf("ireal errno[0x%04x]\n", ireal_errno);
		sleep(5);
	}
	tacom_ireal_hdr_t *hdr = (tacom_ireal_hdr_t *)packet.body.data;

	pthread_mutex_lock(&ireal_hdr_mutex);
	memcpy(&ireal_header, hdr, sizeof(tacom_ireal_hdr_t));
	ireal_header_status = IREAL_HEADER_FULL;
	pthread_mutex_unlock(&ireal_hdr_mutex);
	memcpy(dtg_status.vrn, ireal_header.VRN, 12);
	store_dtg_status();

	return ret;
}

int wrap_ireal_rc()
{
	int ret;
	IREAL_PACKET packet;

	ret = ireal_real(&packet);
	if (ret < 0) {
		fprintf(stderr, "real error\n");
		printf("ireal errno[[0x%08x]\n", ireal_errno);
		return -1;
	}

	read_curr_buf = (tacom_ireal_data_t *)packet.body.data;
	return std_parsing(tm, 1, 0);
}

int wrap_ireal_rl()
{
	LOGW("LAST REQDATE:%02x/%02x/%02x %02x:%02x:%02x:%02x", 
			lastdate.Year, lastdate.Mon, lastdate.Day, 
			lastdate.Hour, lastdate.Min, lastdate.Sec, lastdate.mSec);
}

int wrap_ireal_rs(int r_num)
{
	int ret;
	IREAL_PACKET packet;
	IREAL_DATE tmpdate;
	tacom_ireal_data_t *DTGBody;
	unsigned char reqcnt;
	int respcnt = 0;
	int remaining = 3000;
	char *buf_offset = recvbuf;
	
	memset(recvbuf, 0, 1024*512);
	/* ireal dtg data read */
	if (r_num > 60) 
		reqcnt = 60;
	else
		reqcnt = r_num;

	/* find last date */
	ret = ireal_find(&reqdate, &packet);
	if (ret < 0) {
		/* error */
		printf("find error\n");
		printf("ireal errno[0x%04x]\n", ireal_errno);
		return ret;
	} else {
		tacom_ireal_hdr_t *hdr = (tacom_ireal_hdr_t *)packet.body.data;
		pthread_mutex_lock(&ireal_hdr_mutex);
		memcpy(&ireal_header, hdr, sizeof(tacom_ireal_hdr_t));
		ireal_header_status = IREAL_HEADER_FULL;
		pthread_mutex_unlock(&ireal_hdr_mutex);
		memcpy(dtg_status.vrn, ireal_header.VRN, 12);
		store_dtg_status();
	}

	tmpdate = reqdate;
	while (remaining > 0) {
		/* ireal_read 에서 데이터에 대한 에러처리(깨짐, 누락등)를 한다. */
		ret = ireal_read(&tmpdate, reqcnt, &packet);
		if (ret < 0) {
			fprintf(stderr, "read error\n");
			printf("ireal errno[[0x%08x]\n", ireal_errno);
			break;
		}
		ireal_get_date(&packet, &tmpdate);
		/* 읽은 데이터가 헤더이면 그 헤더의 데이터를 요청한다. */
		if (packet.body.cmd.ecmd == ECMD_TCHEAD) {
			fprintf(stderr, "HEADER\n");
			continue;
		} else if (packet.body.cmd.ecmd == ECMD_TCDATA) {
			DTGBody = (tacom_ireal_data_t *)packet.body.data;
			fprintf(stderr, "DATA\n");
			ireal_date_add_1sec(&tmpdate);
			memcpy((tacom_ireal_data_t *)buf_offset + respcnt, DTGBody, packet.body.cmd.cdata * sizeof(tacom_ireal_data_t));
			remaining -= packet.body.cmd.cdata;
			respcnt += packet.body.cmd.cdata;	// 수신된 DTG 데이터 갱신
			reqcnt = remaining > 60 ? 60 : remaining;
			lastdate = tmpdate;
			/* 읽어진 데이터의 최종 날짜 가져옴 : RQ용 */
			ireal_get_date(&packet, &lastdate);
		}
	}

	if (respcnt == 0) return -1;

	/* iread dtg data convert */
	return sizeof(tacom_ireal_data_t) * respcnt;
}

int wrap_ireal_rq()
{
	int ret;
	FILE *irealfd;
	/* RQ 수신시 요청할 다음 데이터의 시간정보 갱신 및 파일로 저장 */
	memcpy(&reqdate, &lastdate, sizeof(IREAL_DATE));
	ireal_date_add_1sec(&reqdate);
	fprintf(stderr, "Update Reqdate:%02x%02x%02x%02x%02x%02x%02x\n", 
			reqdate.Year, reqdate.Mon, reqdate.Day, 
			reqdate.Hour, reqdate.Min, reqdate.Sec, reqdate.mSec);
	
	irealfd = fopen("/var/irealreqdate", "wb");
	if (irealfd == NULL) {
		fprintf(stderr, "irealreqdate file create error\n");
	} else {
		ret = fwrite(&reqdate, sizeof(IREAL_DATE), sizeof(IREAL_DATE), irealfd);
		if (ret != sizeof(IREAL_DATE)) {
			fprintf(stderr, "irealreqdate file write error\n");
		}
		fclose(irealfd);
	}
}

