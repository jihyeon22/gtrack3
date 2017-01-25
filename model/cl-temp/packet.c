#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <base/gpstool.h>

#include <base/mileage.h>
#include <util/tools.h>

#include "config.h"
#include "packet.h"
#include "data-list.h"

#include "logd/logd_rpc.h"
#include <at/at_util.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

int _set_comm_head_data(CL_COMM_HEAD *phead, char *command)
{
	char phonenum[AT_LEN_PHONENUM_BUFF] = {0};

	at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);

	phead->stx = '[';

	strncpy(phead->command, command, sizeof(phead->command));

	int i, j;
	int unit_id_space = sizeof(phead->unit_id) - strlen(phonenum);
	for(i = 0, j =0; i < sizeof(phead->unit_id); i++) {
		if(i < unit_id_space) {
			phead->unit_id[i] = '@';
		} else {
			phead->unit_id[i] = phonenum[j++];
		}
	}

	return 0;
}

int _set_location_data(CL_LOCATION_BODY *pdata, locationData_t *loc)
{
	gpsData_t *gpsdata = &(loc->gpsdata);

	char temp_date[sizeof(pdata->date)+1] = {0};
	snprintf(temp_date, sizeof(pdata->date)+1, "%02d%02d%02d", gpsdata->year % 100, gpsdata->mon, gpsdata->day);
	memcpy(pdata->date, temp_date, sizeof(pdata->date));

	char temp_time[sizeof(pdata->time)+1] = {0};
	snprintf(temp_time, sizeof(pdata->time)+1, "%02d%02d%02d", gpsdata->hour, gpsdata->min, gpsdata->sec);
	memcpy(pdata->time, temp_time, sizeof(pdata->time));

	if(gpsdata->active == 1) {
		pdata->gps_status = 'A';
	} else {
		pdata->gps_status = 'V';
	}

	char temp_latitude[sizeof(pdata->latitude)+1] = {0};
	snprintf(temp_latitude, sizeof(pdata->latitude)+1, "%08.05f", gpsdata->lat);
	memcpy(pdata->latitude, temp_latitude, sizeof(pdata->latitude));

	char temp_longitude[sizeof(pdata->longitude)+1] = {0};
	snprintf(temp_longitude, sizeof(pdata->longitude)+1, "%09.05f", gpsdata->lon);
	memcpy(pdata->longitude, temp_longitude, sizeof(pdata->longitude));

	char temp_speed[sizeof(pdata->speed)+1] = {0};
	snprintf(temp_speed, sizeof(pdata->speed)+1, "%03d", gpsdata->speed);
	memcpy(pdata->speed, temp_speed, sizeof(pdata->speed));

	char temp_direction[sizeof(pdata->direction)+1] = {0};
	snprintf(temp_direction, sizeof(pdata->direction)+1, "%03d", (int)(gpsdata->angle));
	memcpy(pdata->direction, temp_direction, sizeof(pdata->direction));

	char temp_avg_speed[sizeof(pdata->avg_speed)+1] = {0};
	snprintf(temp_avg_speed, sizeof(pdata->avg_speed)+1, "%03d", loc->avg_speed);
	memcpy(pdata->avg_speed, temp_avg_speed, sizeof(pdata->avg_speed));

	if(loc->acc_status == 0)
	{
		pdata->acc_status = '0';
	}
	else
	{
		pdata->acc_status = '1';
	}

	char temp_accumul_dist[sizeof(pdata->accumul_dist)+1];
	snprintf(temp_accumul_dist, sizeof(pdata->accumul_dist)+1, "%06u", loc->mileage_m);
	memcpy(pdata->accumul_dist, temp_accumul_dist, sizeof(pdata->accumul_dist));

	char temp_event_code[sizeof(pdata->event_code)+1];
	snprintf(temp_event_code, sizeof(pdata->event_code)+1, "%02d", loc->event_code);
	memcpy(pdata->event_code, temp_event_code, sizeof(pdata->event_code));	

	return 0;
}

int make_event_packet(unsigned char **pbuf, unsigned short *packet_len, locationData_t *loc)
{
	int res = 0;
	CL_PACKET_HEAD *phead;
	CL_LOCATION_BODY *pbody;
	CL_PACKET_TAIL *ptail;

	int data_len = sizeof(CL_LOCATION_BODY);
	int data_count = 1;

	unsigned short checksum_len = data_len;
	*packet_len = sizeof(CL_PACKET_HEAD) + data_len + sizeof(CL_PACKET_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		LOGE(LOG_TARGET, "malloc fail!\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_PACKET_HEAD *)packet_buf;
	pbody = (CL_LOCATION_BODY *)(packet_buf + sizeof(CL_PACKET_HEAD));
	ptail = (CL_PACKET_TAIL *)(packet_buf + sizeof(CL_PACKET_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MPL);

	int length = *packet_len;
	char temp_length[sizeof(phead->length)+1] = {0};
	snprintf(temp_length, sizeof(phead->length)+1, "%03d", length);
	memcpy(phead->length, temp_length, sizeof(phead->length));

	char temp_data_count[sizeof(phead->data_count)+1] = {0};
	snprintf(temp_data_count, sizeof(phead->data_count)+1, "%02d", data_count);
	memcpy(phead->data_count, temp_data_count, sizeof(phead->data_count));

	/* pbody */
	_set_location_data(pbody, loc); 

	/* ptail */
	ptail->check_sum = tools_checksum_xor(packet_buf + sizeof(CL_PACKET_HEAD), checksum_len);

	ptail->tail.etx = ']';

	int i;
	printf("size = %d\n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		if(i != *packet_len -2)
		{
			printf("%c", ((char *)phead)[i]);
		}
		else
		{
			printf("[%x]", ((char *)phead)[i]);
		}
	}
	printf("\n");

	return res;
}

int make_report_packet(unsigned char **pbuf, unsigned short *packet_len)
{
	int res = 0;
	int i = 0;

	CL_PACKET_HEAD *phead;
	CL_LOCATION_BODY *pbody;
	CL_PACKET_TAIL *ptail;

	int data_len = 0;
	int data_count = 0;

	locationData_t locData[MAX_DATA_COUNT];
	memset(locData, 0, sizeof(locData));
	
	while(data_count < MAX_DATA_COUNT)
	{
		locationData_t *temp_period = NULL;
		res = list_pop(&packet_list, (void *)(&temp_period));
		if(res < 0)
		{
			if(temp_period != NULL)
			{
				free(temp_period);
			}
			break;
		}
		
		if(temp_period == NULL)
		{
			break;
		}
		
		locData[data_count++] = *temp_period;

		if(temp_period != NULL)
		{
			free(temp_period);
		}
	}

	if(data_count == 0)
	{
		LOGE(LOG_TARGET, "no data to report!\n");
		return -2;
	}
	
	data_len = sizeof(CL_LOCATION_BODY) * data_count;

	unsigned short checksum_len = data_len;
	*packet_len = sizeof(CL_PACKET_HEAD) + data_len + sizeof(CL_PACKET_TAIL);

	printf("make [ %d ] packet size %d = phead(%d) + pdata(%d /%d) + ptail(%d) \n", data_count - 1, \
	       *packet_len, sizeof(CL_PACKET_HEAD), \
	       sizeof(CL_LOCATION_BODY), sizeof(CL_LOCATION_BODY)*data_count, \
	       sizeof(CL_PACKET_TAIL));

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		LOGE(LOG_TARGET, "malloc fail!\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_PACKET_HEAD *)packet_buf;
	pbody = (CL_LOCATION_BODY *)(packet_buf + sizeof(CL_PACKET_HEAD));
	ptail = (CL_PACKET_TAIL *)(packet_buf + sizeof(CL_PACKET_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MPL);

	int length = *packet_len;
	char temp_length[sizeof(phead->length)+1] = {0};
	snprintf(temp_length, sizeof(phead->length)+1, "%03d", length);
	memcpy(phead->length, temp_length, sizeof(phead->length));

	char temp_data_count[sizeof(phead->data_count)+1] = {0};
	snprintf(temp_data_count, sizeof(phead->data_count)+1, "%02d", data_count);
	memcpy(phead->data_count, temp_data_count, sizeof(phead->data_count));

	/* pbody */
	for(i = 0; i < data_count; i++) {
		_set_location_data(&pbody[i], &locData[i]);
	}

	/* ptail */
	ptail->check_sum = tools_checksum_xor(packet_buf + sizeof(CL_PACKET_HEAD), checksum_len);

	ptail->tail.etx = ']';

	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		if(i != *packet_len -2)
		{
			printf("%c", ((char *)phead)[i]);
		}
		else
		{
			printf("[%x]", ((char *)phead)[i]);
		}
	}
	printf("\n");

	return 0;
}

int make_rfid_packet(unsigned char **pbuf, unsigned short *packet_len, locationData_t *loc, rfidData_t *rfid)
{
	int res = 0;
	int i = 0;
	CL_PACKET_HEAD *phead;
	CL_RFID_BODY *pbody;
	CL_PACKET_TAIL *ptail;

	int data_len = sizeof(CL_RFID_BODY);
	int data_count = 1;

	unsigned short checksum_len = data_len;
	*packet_len = sizeof(CL_PACKET_HEAD) + data_len + sizeof(CL_PACKET_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_PACKET_HEAD *)packet_buf;
	pbody = (CL_RFID_BODY *)(packet_buf + sizeof(CL_PACKET_HEAD));
	ptail = (CL_PACKET_TAIL *)(packet_buf + sizeof(CL_PACKET_HEAD) + data_len);


	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MPF);

	int length = *packet_len;
	char temp_length[sizeof(phead->length)+1] = {0};
	snprintf(temp_length, sizeof(phead->length)+1, "%03d", length);
	memcpy(phead->length, temp_length, sizeof(phead->length));

	char temp_data_count[sizeof(phead->data_count)+1] = {0};
	snprintf(temp_data_count, sizeof(phead->data_count)+1, "%02d", data_count);
	memcpy(phead->data_count, temp_data_count, sizeof(phead->data_count));

	/* pbody */
	_set_location_data((CL_LOCATION_BODY *)pbody, loc);

	if(rfid->boarding == RFID_GET_ON)
	{
		pbody->boarding = 'Y';
	}
	else
	{
		pbody->boarding = 'N';
	}

	char rfid_uid[32+1] = {0};

	for(i = 0; i < rfid->len_uid; i++)
	{
		snprintf(&(rfid_uid[i * 2]), 3, "%02x", rfid->uid[i]);
	}
	memcpy(pbody->tag_id, rfid_uid, sizeof(pbody->tag_id));

#if 1
	for(i = 0; i < sizeof(pbody->tag_id); i++) {
		printf("%c ", pbody->tag_id[i]);
	}
	printf("\n");
#endif

	/* ptail */
	ptail->check_sum = tools_checksum_xor(packet_buf + sizeof(CL_PACKET_HEAD), checksum_len);
	ptail->tail.etx = ']';

	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		if(i != *packet_len -2)
		{
			printf("%c", ((char *)phead)[i]);
		}
		else
		{
			printf("[%x]", ((char *)phead)[i]);
		}
	}
	printf("\n");
	
	return res;
}

int make_msi_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned char *ip, unsigned int port)
{
	CL_COMM_HEAD *phead;
	CL_SERVER_BODY *pbody;
	CL_COMM_TAIL *ptail;

	int data_len = sizeof(CL_SERVER_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_SERVER_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);


	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MSI);

	/* pbody */
	pbody->status = 'Y'; //always Y

	char temp_server_ip[sizeof(pbody->server_ip)+1] = {0};
	snprintf(temp_server_ip, sizeof(pbody->server_ip)+1, "%.*s", sizeof(pbody->server_ip), ip);
	memcpy(pbody->server_ip, temp_server_ip, sizeof(pbody->server_ip));

	char temp_server_port[sizeof(pbody->server_port)+1] = {0};
	snprintf(temp_server_port, sizeof(pbody->server_port)+1, "%05u", port);
	memcpy(pbody->server_port, temp_server_port, sizeof(pbody->server_port));

	/* ptail */
	ptail->etx = ']';

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_mit_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int interval, unsigned int count)
{
	CL_COMM_HEAD *phead;
	CL_REPORT_BODY *pbody;
	CL_COMM_TAIL *ptail;

	int data_len = sizeof(CL_REPORT_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_REPORT_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MIT);

	/* pbody */
	pbody->status = 'Y'; //always Y

	char temp_interval_time[sizeof(pbody->interval_time)+1];
	snprintf(temp_interval_time, sizeof(pbody->interval_time)+1, "%05u", interval);
	memcpy(pbody->interval_time, temp_interval_time, sizeof(pbody->interval_time));

	char temp_max_packet[sizeof(pbody->max_packet)+1];
	snprintf(temp_max_packet, sizeof(pbody->max_packet)+1, "%02u", count);
	memcpy(pbody->max_packet, temp_max_packet, sizeof(pbody->max_packet));

	/* ptail */
	ptail->etx = ']';

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_gio_packet(unsigned char **pbuf, unsigned short *packet_len, CL_GIO_BODY *body)
{
	CL_COMM_HEAD *phead;
	CL_GIO_BODY *pbody;
	CL_COMM_TAIL *ptail;

	int data_len = sizeof(CL_GIO_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_GIO_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_GIO);

	/* pbody */
	memcpy(pbody, body, sizeof(CL_GIO_BODY));

	/* ptail */
	ptail->etx = ']';

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_mgz_packet(unsigned char **pbuf, unsigned short *packet_len, CL_GIO_BODY *body)
{
	CL_COMM_HEAD *phead;
	CL_GIO_BODY *pbody;
	CL_COMM_TAIL *ptail;

	int data_len = sizeof(CL_GIO_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_GIO_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MGZ);

	/* pbody */
	memcpy(pbody, body, sizeof(CL_GIO_BODY));

	/* ptail */
	ptail->etx = ']';

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_css_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int stop_time)
{
	CL_COMM_HEAD *phead;
	CL_STOP_TIME_BODY *pbody;
	CL_COMM_TAIL *ptail;

	int data_len = sizeof(CL_STOP_TIME_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_STOP_TIME_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_CSS);

	/* pbody */
	pbody->status = 'Y'; //always Y

	char temp_stop_time[sizeof(pbody->stop_time)+1];
	snprintf(temp_stop_time, sizeof(pbody->stop_time)+1, "%07u", stop_time);
	memcpy(pbody->stop_time, temp_stop_time, sizeof(pbody->stop_time));

	/* ptail */
	ptail->etx = ']';

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_mir_packet(unsigned char **pbuf, unsigned short *packet_len, unsigned int *m_kmh)
{
	CL_COMM_HEAD *phead;
	CL_MIR_BODY *pbody;
	CL_COMM_TAIL *ptail;
	int i = 0;

	int data_len = sizeof(CL_MIR_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_MIR_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_MIR);

	/* pbody */
	pbody->status = 'Y'; //always Y

	char tmp_buf[5] = {0};
	for(i=0; i<11; i++)
	{
		snprintf(tmp_buf, 5, "%04u", m_kmh[i]);
		memcpy(pbody->m_kmh[i], tmp_buf, 4);
	}

	/* ptail */
	ptail->etx = ']';

	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_mst_packet(unsigned char **pbuf, unsigned short *packet_len, CL_MST_BODY *body)
{
	CL_COMM_HEAD *phead;
	CL_MST_BODY *pbody;
	CL_COMM_TAIL *ptail;

	int data_len = sizeof(CL_MST_BODY);

	*packet_len = sizeof(CL_COMM_HEAD) + data_len + sizeof(CL_COMM_TAIL);

	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(*packet_len);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}
	memset(packet_buf, 0, *packet_len);
	*pbuf = packet_buf;

	phead = (CL_COMM_HEAD *)packet_buf;
	pbody = (CL_MST_BODY *)(packet_buf + sizeof(CL_COMM_HEAD));
	ptail = (CL_COMM_TAIL *)(packet_buf + sizeof(CL_COMM_HEAD) + data_len);

	/* phead */
	_set_comm_head_data((CL_COMM_HEAD *)phead, CMD_GIO);

	/* pbody */
	memcpy(pbody, body, sizeof(CL_MST_BODY));

	/* ptail */
	ptail->etx = ']';

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)phead)[i]);
	}
	printf("\n");

	return 0;
}

int make_raw_packet(unsigned char **pbuf, unsigned short *packet_len, bufData_t *pbuf_data)
{
	unsigned char *packet_buf;
	packet_buf = (unsigned char *)malloc(pbuf_data->used_size);
	if(packet_buf == NULL)
	{
		printf("malloc fail\n");
		return -1;
	}

	if(pbuf_data->used_size <= 0)
	{
		printf("not exist data\n");
		free(packet_buf);
		return -1;
	}

	*packet_len = pbuf_data->used_size;

	memcpy(packet_buf, pbuf_data->buf, pbuf_data->used_size);

	*pbuf = packet_buf;

	int i;
	printf("size = %d \n", *packet_len);
	for(i = 0; i < *packet_len; i++)
	{
		printf("%c", ((char *)packet_buf)[i]);
	}
	printf("\n");

	return 0;
}

int cl_resp_packet_check(char result)
{
	int res = 0;
	switch(result)
	{
		case CL_SUC_ERROR_CODE:
			res = RESULT_OK;
			printf("CL RESULT_OK = %d \n", res);
			break;
		case CL_SET_ERROR_CODE:
			res = RESULT_RETRY;
			printf("CL RESULT_RETRY = %d \n", res);
			break;
		case CL_AUT_ERROR_CODE:
			res = RESULT_NA;
			printf("CL RESULT_NA = %d \n", res);
			break;
		case CL_CHK_ERROR_CODE:
			res = RESULT_ERR;
			printf("CL RESULT_ERR = %d \n", res);
			break;
		case CL_LEN_ERROR_CODE:
			res = RESULT_NA;
			printf("CL RESULT_NA = %d \n", res);
		default:
			;
	}
	if(result != CL_SUC_ERROR_CODE)
	{
		printf("CL RESPONSE ERROR : 0x%x \n", result);
	}
	return res;
}

