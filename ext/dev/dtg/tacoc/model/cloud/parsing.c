/**
* @file parsing.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-21
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <standard_protocol.h>
#include <dtg_data_manage.h>

unsigned long evt_code = 0;

int cld_record_parsing(char *buf, int buf_len, char *stdbuf)
{
	char tmp_buf[40];
	char tmp_buf_1[40];
	char tmp_buf_2[40];
	int src_idx = 0;
	int dest_idx = 0;
	int r_num = 0;

	int i;
	int len;
	unsigned char check_sum;
	unsigned char *p_packet_checksum;
	int packet_len;
	char *phonenum = NULL;
	
	tacom_std_data_t *std_data;

	CLOUD_SOFT_SERVER_PACKET_HEAD * cloud_hdr;
	CLOUD_SOFT_SERVER_PACKET_DATA * cloud_data;
	CLOUD_SOFT_SERVER_PACKET_TAIL * cloud_tail;

	int ret = 0;
	if(buf[0] != '>')
		return -1;

	memset(stdbuf, 0x00, sizeof(stdbuf));
	src_idx += 1;
	dest_idx = 0;

	cloud_hdr = (CLOUD_SOFT_SERVER_PACKET_HEAD *)&stdbuf[dest_idx];

    packet_len = sizeof(CLOUD_SOFT_SERVER_PACKET_HEAD) + 
    			sizeof(CLOUD_SOFT_SERVER_PACKET_DATA) + 
    			sizeof(CLOUD_SOFT_SERVER_PACKET_TAIL);
	   
	//packet Header ++
	memset(cloud_hdr, 0x00, sizeof(CLOUD_SOFT_SERVER_PACKET_HEAD));
	cloud_hdr->stx = '[';	
	memcpy(cloud_hdr->command, "MPR",sizeof(cloud_hdr->command));
	memset(cloud_hdr->unit_id, '#', sizeof(cloud_hdr->unit_id));
	phonenum = atcmd_get_phonenum();		// Check Yoonki 20130523
	len = strlen(phonenum);
	i = sizeof(cloud_hdr->unit_id) - len;
	memcpy(&cloud_hdr->unit_id[i], phonenum, len);


	sprintf(tmp_buf, "%03d", packet_len);
	memcpy(cloud_hdr->length, tmp_buf, sizeof(cloud_hdr->length));
	memcpy(cloud_hdr->data_count, "01", 2);

	src_idx += sizeof(tacom_std_hdr_t);
	dest_idx += sizeof(CLOUD_SOFT_SERVER_PACKET_HEAD);
	//packet Header --

	//packet Data ++
	std_data = (tacom_std_data_t *)&buf[src_idx];
	cloud_data = (CLOUD_SOFT_SERVER_PACKET_DATA *)&stdbuf[dest_idx];

	memcpy(cloud_data->date, std_data->date_time, sizeof(cloud_data->date));
	memcpy(cloud_data->time, &std_data->date_time[6], sizeof(cloud_data->time));

	long gps_x = char_mbtol(std_data->gps_x, 9);
	long gps_y = char_mbtol(std_data->gps_y, 9);
	if(gps_x == 0 || gps_y == 0)
		cloud_data->gps_status = 'V';
	else
		cloud_data->gps_status = 'A';
	sprintf(cloud_data->gps_latitude, "%02d.%06d", gps_x/1000000, gps_x%1000000);
	sprintf(cloud_data->gps_longitude, "%03d.%05d", gps_y/1000000, (gps_y%1000000)/10);

	memcpy(cloud_data->speed, std_data->speed, 3);
	memcpy(cloud_data->direction, std_data->azimuth, 3);

	memcpy(cloud_data->altitude, "0000", 4); 

	memset(cloud_data->accumu_dist, '0', 10);
	memcpy(&cloud_data->accumu_dist[3], std_data->cumul_run_dist, 7);

	cloud_data->acc_status = '1';
	cloud_data->battery_status = '0';
	cloud_data->area_no = '0';

	   
	if(evt_code == 0){
       	memcpy(cloud_data->event_code, "03", 2);
       	evt_code = 3;
	} else {
       	memcpy(cloud_data->event_code, "05", 2);
       	evt_code = 5;
	}
	long cumul_oil_usage = char_mbtol(std_data->cumulative_oil_usage, 9);
	memset(cloud_data->ext_inter_val, '0', sizeof(cloud_data->ext_inter_val));
	sprintf(cloud_data->ext_inter_val, "GStart%07d.%dGEnd", 
		cumul_oil_usage/10, cumul_oil_usage%10);

	src_idx += sizeof(tacom_std_data_t);
	dest_idx += sizeof(CLOUD_SOFT_SERVER_PACKET_DATA);
	//packet Data --

	//packet tail ++
	cloud_tail = (CLOUD_SOFT_SERVER_PACKET_TAIL *)&stdbuf[dest_idx];

	p_packet_checksum = (unsigned char *)cloud_data;
	check_sum = p_packet_checksum[0];
	for(i = 1; i < sizeof(CLOUD_SOFT_SERVER_PACKET_DATA); i++) {
		check_sum = check_sum ^ p_packet_checksum[i];
	}

	cloud_tail->check_sum = check_sum;
	cloud_tail->ext = ']';

	dest_idx += sizeof(CLOUD_SOFT_SERVER_PACKET_TAIL);
	//packet tail --

	r_num++;
	DTG_LOGD("Stream Size HDR[%d] + DATA[%d] + TAIL[%d] : [%d]", 
			sizeof(CLOUD_SOFT_SERVER_PACKET_HEAD),
			sizeof(CLOUD_SOFT_SERVER_PACKET_DATA) * r_num, 
			sizeof(CLOUD_SOFT_SERVER_PACKET_TAIL),
			dest_idx);

	return dest_idx;
}

