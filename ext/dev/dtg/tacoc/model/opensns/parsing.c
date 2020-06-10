/**
* @file parsing.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-08
*/
#include <stdlib.h>
#include <string.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <standard_protocol.h>
#include <dtg_data_manage.h>
#include <wrapper/dtg_atcmd.h>
#include <wrapper/dtg_version.h>
#include <board/modem-time.h>
#include <board/board_system.h>
#include <board/battery.h>
#include <time.h>

//#include <sys/stat.h>
//#include <sys/ioctl.h>
//#include <sys/socket.h>
//#include <net/if.h>
//#include <arpa/inet.h>
//#include <netinet/in.h>
//#include <netinet/ether.h>
//#include <netinet/if_ether.h>
//#include <sys/sysctl.h>
//#include <errno.h>
//#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>



#include "dtg_data_manage.h"
#include "server_packet.h"
#include "cvt_gps_pos.h"

static int g_dtg_key_status = eDTG_Key_Invalid;

void set_dtg_key_status(int flag)
{
	g_dtg_key_status = flag;
}

int get_dtg_key_status()
{
	return g_dtg_key_status;
}

//0 : ascii
//1 : binary(hexa)
void print_dtg_msg(unsigned char *msg, int len, int mode)
{
	int i;
	int j;

	for(i = 0, j = 1; i < len; i++, j++)
	{
		if(mode == 0)
		{
			printf("%c", msg[i]);
		}
		else
		{
			printf("%02x ", msg[i]);
			if( (j % 20) == 0)
				printf("\n");
		}
	}
	printf("\n");
}

//  ev
//	eDS_KeyOn			= 0x00,
//	eDS_Key_On_Running	= 0x01,
//	eDS_KeyOff			= 0x02,
//	eDS_Key_Off_Running	= 0x03
int status_data_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, int ev)
{
	tacom_std_hdr_t	        *p_std_hdr;
	tacom_std_data_t        *p_std_data;
	tacom_std_data_t        *p_start_std_data;
	message_define_t        *p_mdf;
	message_header_t        *p_mhdr;
	gps_track_info_pack_t   *p_gps_tip;
	gps_track_data_t        *p_gps_td;
	
	int dest_idx;
	int std_idx;
	unsigned long current_time;
	char *phonenum;
	int gps_count;
	struct tm gps_time;
	float batteryVoltage;
	char item_value[256];
	unsigned char tmp1;
	unsigned short tmp2;
	unsigned int tmp4;
	int gps_x, gps_y;
	unsigned long bx, by;
	unsigned long prev_bx, prev_by;
	double wgs84_x, wgs84_y;
	int first_active_gps = 0;;

	//gps_count  = (std_buff_len - sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t)) / sizeof(tacom_std_data_t);
	//printf("========> gps_count [%d]\n", gps_count);
	if(std_buff == NULL)
		return 0;

	dest_idx = std_idx = 0;

	p_std_hdr = (tacom_std_hdr_t *)std_buff;
	std_idx += sizeof(tacom_std_hdr_t);
	p_start_std_data = (tacom_std_data_t *)&std_buff[std_idx];
	//std_idx = sizeof(tacom_std_data_t);

	memset(dest, 0x00, dest_len);
	p_mdf = (message_define_t *)dest;
	dest_idx += sizeof(message_define_t);
	p_mhdr = (message_header_t *)&dest[dest_idx];
	dest_idx += sizeof(message_header_t);

	p_mdf->business_code = eBC_ASP;
	p_mdf->service_code = eSC_DTG_DATA;


	if(!strcmp(get_server_ip_addr(), "211.43.202.85")) //test server
	{
		p_mdf->dst_ip = inet_addr("211.200.15.205"); //hard coding
		p_mdf->dst_port = htons(8545);
	}
	else
	{
		p_mdf->dst_ip = inet_addr("211.200.12.168"); //hard coding
		p_mdf->dst_port = htons(8545);
	}


	p_mdf->packet_encrypt = ePE_NOT_APPLY;
	p_mdf->packet_compression = ePC_NO_ZIP;


	current_time = get_modem_time_utc_sec();
	p_mhdr->msg_date = htonl(current_time);

	p_mhdr->msg_flow = 0x01; //request
	phonenum = atcmd_get_phonenum(); 
	memset(p_mhdr->terminal_ID, 0x00, sizeof(p_mhdr->terminal_ID));
	if(phonenum != NULL) {
		strncpy(p_mhdr->terminal_ID, phonenum, sizeof(p_mhdr->terminal_ID));
	}

	p_mhdr->terminal_type = 20; //fixed (MDS Type=20)

	//protocol document refer 9 page.
	p_mhdr->operation_code = 0x03;
	p_mhdr->operation_flag = 0x00;

	memset(p_mhdr->work_id, 0x00, sizeof(p_mhdr->work_id));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "����",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_mhdr->work_id, p_std_hdr->registration_num, 12);
	}
	memset(item_value, 0x00, sizeof(item_value));
	memcpy(item_value, p_mhdr->work_id, sizeof(p_mhdr->work_id));
	DTG_LOGI("work_id : %s", item_value);

	p_mhdr->response_type = eRT_REQUEST_RESPOSE;

	p_mhdr->device_state = ev;
	p_mhdr->driver_id = htonl(0);
	p_mhdr->protocol_ver = 1;

	p_mhdr->return_code = htons(0);
	p_mhdr->terminal_ip = inet_addr("127.0.0.1");
	p_mhdr->port = htons(0);
	p_mhdr->gpsinfo_flag = 1;

	////////////////////////////////////////////
	//gps track
	////////////////////////////////////////////

	p_gps_tip = (gps_track_info_pack_t *)&dest[dest_idx];
	dest_idx += sizeof(gps_track_info_pack_t);

	p_gps_tip->collection_interval = get_mdt_create_period();;
	gps_count = 0;
//	printf("gtrack delta bebore dest_idx = [%d]\n", dest_idx);
	while ((std_buff_len - std_idx) > sizeof(tacom_std_data_t)) {
		p_std_data = (tacom_std_data_t *)&std_buff[std_idx];
		std_idx += sizeof(tacom_std_data_t);

		p_gps_td = (gps_track_data_t *)&dest[dest_idx];

		gps_x = char_mbtol(p_std_data->gps_x, 9);
		gps_y = char_mbtol(p_std_data->gps_y, 9);

		if(first_active_gps == 0)
		{
//printf("first_active_gps is zero\n");
			if( gps_x == 0 || gps_y == 0 )
			{
//printf("first_active_gps is zero : gpx is invalid\n");
				continue;
			}
			else
			{
				first_active_gps = 1;
				p_start_std_data = p_std_data;

				wgs84_x = gps_x/1000000.0;
				wgs84_y = gps_y/1000000.0;
				//printf("wgs84 gps_x, gps_y = [%.7f, %.7f]\n", wgs84_x, wgs84_y);
				wgs2bessel(wgs84_y, wgs84_x, &bx, &by);
				prev_bx = bx;
				prev_by = by;
				p_gps_tip->start_x = htonl(bx);
				p_gps_tip->start_y = htonl(by);
//printf("first_active_gps is zero : gpx is OK [%.6f, %.6f, %ld, %ld]\n", wgs84_x, wgs84_y, bx, by);


			}
		}
		else
		{
			if( gps_x == 0 || gps_y == 0 )
			{
				p_gps_td->dx = -128;
				p_gps_td->dy = -128;
				dest_idx += sizeof(gps_track_data_t);
				gps_count += 1;
//printf("invalid> dx, dy = [%d, %d]\n", p_gps_td->dx, p_gps_td->dy);
			}
			else
			{
				wgs84_x = gps_x/1000000.0;
				wgs84_y = gps_y/1000000.0;
				//printf("wgs84 gps_x, gps_y = [%.7f, %.7f]\n", wgs84_x, wgs84_y);
				wgs2bessel(wgs84_y, wgs84_x, &bx, &by);

				p_gps_td->dx = bx - prev_bx;
				p_gps_td->dy = by - prev_by;

//printf("OK!> dx, dy = [%.6f, %.6f, %ld, %ld, %ld, %ld, %ld, %ld]\n", wgs84_x, wgs84_y, bx, by, prev_bx, prev_by, (int)(bx - prev_bx), (int)(by - prev_by));

				dest_idx += sizeof(gps_track_data_t);
				gps_count += 1;
				prev_bx = bx;
				prev_by = by;

			}
		}

	} //end while

	//printf("gtrack delta after dest_idx = [%d]\n", dest_idx);

	if(first_active_gps == 0)
	{
		p_gps_tip->start_x = htonl(0);
		p_gps_tip->start_y = htonl(0);
	}

	gps_time.tm_year  = char_mbtol(p_start_std_data->date_time,   2) + 100;
	gps_time.tm_mon   = char_mbtol(p_start_std_data->date_time+2, 2) - 1;
	gps_time.tm_mday  = char_mbtol(p_start_std_data->date_time+4, 2);
	gps_time.tm_hour  = char_mbtol(p_start_std_data->date_time+6, 2);
	gps_time.tm_min   = char_mbtol(p_start_std_data->date_time+8, 2);
	gps_time.tm_sec   = char_mbtol(p_start_std_data->date_time+10,2);
	//printf("dat date : [%04d/%02d/%02d %02d:%02d:%02d]\n", gps_time.tm_year, gps_time.tm_mon , gps_time.tm_mday, gps_time.tm_hour, gps_time.tm_min, gps_time.tm_sec);
	p_gps_tip->gps_start_time = htonl(mktime(&gps_time));

	DTG_LOGI("========> gps_count [%d]", gps_count);
	p_gps_tip->data_cnt = htons(gps_count);

	////////////////////////////////////////////
	//status packet body
	////////////////////////////////////////////
//	printf("===> body start buff_idx [%d]\n", dest_idx);
	strncpy(&dest[dest_idx], "DEV 1.0", 10);
	dest_idx += 10;

	strncpy(&dest[dest_idx], "APP 1.0", 10	);
	dest_idx += 10;

	strncpy(&dest[dest_idx], "SYS 1.0", 10);
	dest_idx += 10;

	dest[dest_idx++] = RESOURCE_COUNT; //resource cnt(Vehicle Voltage, Speed, RPM, brake)

	
	//Resource Battery #1
	tmp2 = htons(eRI_VEHICLE_BATTERY); //item kind
	memcpy(&dest[dest_idx], &tmp2, 2);
	dest_idx += 2;

	batteryVoltage = battery_get_battlevel_car();
	batteryVoltage = batteryVoltage/1000.0;
printf("#2.batteryVoltage = [%.2f]\n", batteryVoltage);

	sprintf(item_value, "%.2f", batteryVoltage);
	tmp1 = strlen(item_value);
	memcpy(&dest[dest_idx++], &tmp1, 1); //item value length
	memcpy(&dest[dest_idx], item_value, tmp1); //item value
	dest_idx += tmp1;
	
	//Resource SPEED #2
	tmp2 = htons(eRI_SPEED);
	memcpy(&dest[dest_idx], &tmp2, 2); //item kind
	dest_idx += 2;

	sprintf(item_value, "%ld", char_mbtol(p_start_std_data->speed, 3));
	tmp1 = strlen(item_value);
	memcpy(&dest[dest_idx++], &tmp1, 1); //item value length
	memcpy(&dest[dest_idx], item_value, tmp1); //item value
	dest_idx += tmp1;

	//Resource RPM #3
	tmp2 = htons(eRI_RPM);
	memcpy(&dest[dest_idx], &tmp2, 2); //item kind
	dest_idx += 2;

	sprintf(item_value, "%ld", char_mbtol(p_start_std_data->rpm, 4));
	tmp1 = strlen(item_value);
	memcpy(&dest[dest_idx++], &tmp1, 1); //item value length
	memcpy(&dest[dest_idx], item_value, tmp1); //item value
	dest_idx += tmp1;

	//Resource BRAKE #4
	tmp2 = htons(eRI_BRAKE);
	memcpy(&dest[dest_idx], &tmp2, 2); //item kind
	dest_idx += 2;

	if(p_start_std_data->bs == 0x30)
		sprintf(item_value, "%d", 0);
	else
		sprintf(item_value, "%d", 1);

//	printf("status_data_parse> brake = [%s]\n", item_value);


	tmp1 = strlen(item_value);
	memcpy(&dest[dest_idx++], &tmp1, 1); //item value length
	memcpy(&dest[dest_idx], item_value, tmp1); //item value
	dest_idx += tmp1;

	memset(&dest[dest_idx], 0, 4); //reserved (4bytes)
	dest_idx += 4;

	p_mhdr->msg_body_len = htonl(dest_idx - sizeof(message_define_t));
	p_mdf->msg_len = htonl(dest_idx);

	return dest_idx;

}


int dtg_data_header_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, unsigned short *dtg_record_pack_cnt)
{
	tacom_std_hdr_t	        *p_std_hdr = NULL;
	message_define_t        *p_mdf = NULL;
	message_header_t        *p_mhdr = NULL;
	dtg_header_t			*p_dtg_hdr = NULL;

	
	int dest_idx = 0;
	int std_idx = 0;
	int record_cnt = 0;
	unsigned long current_time = 0;
	char *phonenum = NULL;
	char tmp_buf[20];

	dest_idx = std_idx = 0;

	p_std_hdr = (tacom_std_hdr_t *)std_buff;
	std_idx += sizeof(tacom_std_hdr_t);

	memset(dest, 0x00, dest_len);
	p_mdf = (message_define_t *)dest;
	dest_idx += sizeof(message_define_t);
	p_mhdr = (message_header_t *)&dest[dest_idx];
	dest_idx += sizeof(message_header_t);

	p_mdf->business_code = eBC_ASP;
	p_mdf->service_code = eSC_DTG_DATA;


	if(!strcmp(get_server_ip_addr(), "211.43.202.85")) //test server
	{
		p_mdf->dst_ip = inet_addr("211.200.15.205"); //hard coding
		p_mdf->dst_port = htons(8545);
	}
	else
	{
		p_mdf->dst_ip = inet_addr("211.200.12.168"); //hard coding
		p_mdf->dst_port = htons(8545);
	}


	p_mdf->packet_encrypt = ePE_NOT_APPLY;
	p_mdf->packet_compression = ePC_NO_ZIP;


	current_time = get_modem_time_utc_sec();
	p_mhdr->msg_date = htonl(current_time);


	p_mhdr->msg_flow = 0x01; //request
	phonenum = atcmd_get_phonenum(); 
	memset(p_mhdr->terminal_ID, 0x00, sizeof(p_mhdr->terminal_ID));
	if(phonenum != NULL) {
		strncpy(p_mhdr->terminal_ID, phonenum, sizeof(p_mhdr->terminal_ID));
		//DTG_LOGE("%s> terminal_ID need Modify, Now Hard Coding for Test\n", __func__);
		//strncpy(p_mhdr->terminal_ID, "01220551239", sizeof(p_mhdr->terminal_ID));
	}

	p_mhdr->terminal_type = 20; //fixed (MDS Type=20)

	//protocol document refer 9 page.
	p_mhdr->operation_code = 0xfe;
	p_mhdr->operation_flag = 0x01;

	memset(p_mhdr->work_id, 0x00, sizeof(p_mhdr->work_id));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "����",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_mhdr->work_id, p_std_hdr->registration_num, 12);
	}

	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	memcpy(tmp_buf, p_mhdr->work_id, sizeof(p_mhdr->work_id));
	DTG_LOGI("work_id : %s", tmp_buf);

	p_mhdr->response_type = eRT_REQUEST_RESPOSE;

	p_mhdr->device_state = eDS_Key_On_Running;
	p_mhdr->driver_id = htonl(0);

#if defined(FRAMEWORK_SIZE_44)
	p_mhdr->protocol_ver = 2;
#elif defined(FRAMEWORK_SIZE_48)
	p_mhdr->protocol_ver = 3;
#else
	p_mhdr->protocol_ver = 1;
#endif
	

	p_mhdr->return_code = htons(0);
	p_mhdr->terminal_ip = inet_addr("127.0.0.1");
	p_mhdr->port = htons(0);
	p_mhdr->gpsinfo_flag = 0;

	//DTG HEADER++++
	p_dtg_hdr = (dtg_header_t *)&dest[dest_idx];
	dest_idx += sizeof(dtg_header_t);

	memcpy(p_dtg_hdr->vehicle_model, p_std_hdr->vehicle_model, 20);
	memcpy(p_dtg_hdr->vehicle_id_num, p_std_hdr->vehicle_id_num, 17);

	switch(char_mbtol(p_std_hdr->vehicle_type, 2))
	{
		case 11:
			p_dtg_hdr->vehicle_type = 0x00;
			break;
		case 12:
			p_dtg_hdr->vehicle_type = 0x01;
			break;
		case 13:
			p_dtg_hdr->vehicle_type = 0x02;
			break;
		case 14:
			p_dtg_hdr->vehicle_type = 0x03;
			break;
		case 15:
			p_dtg_hdr->vehicle_type = 0x04;
			break;
		case 16:
			p_dtg_hdr->vehicle_type = 0x05;
			break;
		case 17:
			p_dtg_hdr->vehicle_type = 0x06;
			break;
		case 21:
			p_dtg_hdr->vehicle_type = 0x07;
			break;
		case 22:
			p_dtg_hdr->vehicle_type = 0x08;
			break;
		case 31:
			p_dtg_hdr->vehicle_type = 0x09;
			break;
		case 32:
			p_dtg_hdr->vehicle_type = 0x0A;
			break;
		case 41:
			p_dtg_hdr->vehicle_type = 0x0B;
			break;
		default:
			p_dtg_hdr->vehicle_type = 0x00;
			break;
		
	}

	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memset(p_dtg_hdr->registration_num, 0, 12);
		memcpy(p_dtg_hdr->registration_num, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memset(p_dtg_hdr->registration_num, 0, 12);
		memcpy(p_dtg_hdr->registration_num, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "����",4)) {
		memset(p_dtg_hdr->registration_num, 0, 12);
		memcpy(p_dtg_hdr->registration_num, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_dtg_hdr->registration_num, p_std_hdr->registration_num, 12);
	}

	memcpy(p_dtg_hdr->business_license_num, p_std_hdr->business_license_num, 10);
	p_dtg_hdr->driver_idx = 0;
	memcpy(p_dtg_hdr->driver_code, p_std_hdr->driver_code, 7);

#if defined(FRAMEWORK_SIZE_44)
	p_dtg_hdr->reserved[0] = 44;
#elif defined(FRAMEWORK_SIZE_48)
	p_dtg_hdr->reserved[0] = 48;
#else
	p_dtg_hdr->reserved[0] = 0;
#endif


	record_cnt = 0;
	while ((std_buff_len - std_idx) >= sizeof(tacom_std_data_t)) {
		record_cnt += 1;
		std_idx += sizeof(tacom_std_data_t);
	}

	memcpy(tmp_buf, p_dtg_hdr->registration_num, 12);
	DTG_LOGT("VRN[%s], DTG Record Count[%d]\n", tmp_buf, record_cnt);

	//p_dtg_hdr
	*dtg_record_pack_cnt = record_cnt;
	record_cnt = htonl(record_cnt);
	memcpy(&dest[dest_idx], &record_cnt, 4);
	dest_idx += 4;
	dest_idx += 4; //reserved

	p_mhdr->msg_body_len = htonl(dest_idx - sizeof(message_define_t));
	p_mdf->msg_len = htonl(dest_idx);
	printf("total len = [%d]\n", dest_idx);

	return dest_idx;
}

int dtg_data_pack_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, unsigned short dtg_record_pack_cnt)
{
	tacom_std_hdr_t	        *p_std_hdr;
	tacom_std_data_t        *p_std_data;
	message_define_t        *p_mdf;
	message_header_t        *p_mhdr;
	dtg_record_pack_t		*p_dtg_rec_pack;
	int key_flag = 0;

	
	int dest_idx;
	int std_idx;
	unsigned long current_time;
	char *phonenum;
	int gps_count;
	struct tm gps_time;
	float batteryVoltage;
	char item_value[256];
	unsigned char tmp1;
	unsigned short tmp2;
	unsigned int tmp4;
	int gps_x, gps_y;
	double wgs84_x, wgs84_y;

	//gps_count  = (std_buff_len - sizeof(tacom_std_hdr_t)+sizeof(tacom_std_data_t)) / sizeof(tacom_std_data_t);
	//printf("========> gps_count [%d]\n", gps_count);

	dest_idx = std_idx = 0;

	p_std_hdr = (tacom_std_hdr_t *)std_buff;
	std_idx += sizeof(tacom_std_hdr_t);

	memset(dest, 0x00, dest_len);
	p_mdf = (message_define_t *)dest;
	dest_idx += sizeof(message_define_t);
	p_mhdr = (message_header_t *)&dest[dest_idx];
	dest_idx += sizeof(message_header_t);


	p_mdf->business_code = eBC_ASP;
	p_mdf->service_code = eSC_DTG_DATA;


	if(!strcmp(get_server_ip_addr(), "211.43.202.85")) //test server
	{
		p_mdf->dst_ip = inet_addr("211.200.15.205"); //hard coding
		p_mdf->dst_port = htons(8545);
	}
	else
	{
		p_mdf->dst_ip = inet_addr("211.200.12.168"); //hard coding
		p_mdf->dst_port = htons(8545);
	}


	p_mdf->packet_encrypt = ePE_NOT_APPLY;
	p_mdf->packet_compression = ePC_NO_ZIP;


	current_time = get_modem_time_utc_sec();
	p_mhdr->msg_date = htonl(current_time);


	p_mhdr->msg_flow = 0x01; //request
	phonenum = atcmd_get_phonenum(); 
	memset(p_mhdr->terminal_ID, 0x00, sizeof(p_mhdr->terminal_ID));
	if(phonenum != NULL) {
		strncpy(p_mhdr->terminal_ID, phonenum, sizeof(p_mhdr->terminal_ID));

		//DTG_LOGE("%s> terminal_ID need Modify, Now Hard Coding for Test\n", __func__);
		//strncpy(p_mhdr->terminal_ID, "01220551239", sizeof(p_mhdr->terminal_ID));
	}

	p_mhdr->terminal_type = 20; //fixed (MDS Type=20)

	//protocol document refer 9 page.
	p_mhdr->operation_code = 0xfe;
	p_mhdr->operation_flag = 0x02;

	memset(p_mhdr->work_id, 0x00, sizeof(p_mhdr->work_id));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "����",4)) {
		memcpy(p_mhdr->work_id, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_mhdr->work_id, p_std_hdr->registration_num, 12);
	}

	memset(item_value, 0x00, sizeof(item_value));
	memcpy(item_value, p_mhdr->work_id, sizeof(p_mhdr->work_id));
	DTG_LOGI("work_id : %s", item_value);

	p_mhdr->response_type = eRT_REQUEST_RESPOSE;

	if(power_get_ignition_status() == POWER_IGNITION_ON)
		p_mhdr->device_state = eDS_Key_On_Running;
	else
		p_mhdr->device_state = eDS_KeyOff;	

	p_mhdr->driver_id = htonl(0);
#if defined(FRAMEWORK_SIZE_44)
	p_mhdr->protocol_ver = 2;
#elif defined(FRAMEWORK_SIZE_48)
	p_mhdr->protocol_ver = 3;
#else
	p_mhdr->protocol_ver = 1;
#endif

	p_mhdr->return_code = htons(0);
	p_mhdr->terminal_ip = inet_addr("127.0.0.1");
	p_mhdr->port = htons(0);
	p_mhdr->gpsinfo_flag = 0;

	tmp2 = htons(dtg_record_pack_cnt);
	memcpy(&dest[dest_idx], &tmp2, 2);
	dest_idx += 2;


	while ((std_buff_len - std_idx) >= sizeof(tacom_std_data_t)) {
		p_std_data = (tacom_std_data_t *)&std_buff[std_idx];
		p_dtg_rec_pack = (dtg_record_pack_t *)&dest[dest_idx];
	
#ifdef DTG_RECORD_BIG_ENDIAN
		p_dtg_rec_pack->cumul_run_dist = htonl(char_mbtol(p_std_data->cumul_run_dist, 7)); //Km
#else
		p_dtg_rec_pack->cumul_run_dist = char_mbtol(p_std_data->cumul_run_dist, 7); //Km
#endif

		gps_time.tm_year  = char_mbtol(p_std_data->date_time,   2);
		gps_time.tm_mon   = char_mbtol(p_std_data->date_time+2, 2);
		gps_time.tm_mday  = char_mbtol(p_std_data->date_time+4, 2);
		gps_time.tm_hour  = char_mbtol(p_std_data->date_time+6, 2);
		gps_time.tm_min   = char_mbtol(p_std_data->date_time+8, 2);
		gps_time.tm_sec   = char_mbtol(p_std_data->date_time+10,2);
		sprintf(item_value, "%04d%02d%02d%02d%02d%02d", gps_time.tm_year+2000, gps_time.tm_mon, gps_time.tm_mday, gps_time.tm_hour, gps_time.tm_min, gps_time.tm_sec);
		ConvertStringTime2DwordTime(item_value, &tmp4);
		//printf("DTG DATE : [%04d%02d%02d%02d%02d%02d] => [%u]\n", gps_time.tm_year+2000, gps_time.tm_mon, gps_time.tm_mday, gps_time.tm_hour, gps_time.tm_min, gps_time.tm_sec, tmp4);
#ifdef DTG_RECORD_BIG_ENDIAN
		p_dtg_rec_pack->date_time = htonl(tmp4);
#else
		p_dtg_rec_pack->date_time = tmp4;
#endif
		

#ifdef DTG_RECORD_BIG_ENDIAN
		p_dtg_rec_pack->gps_x = htonl(char_mbtol(p_std_data->gps_x, 9));
		p_dtg_rec_pack->gps_y = htonl(char_mbtol(p_std_data->gps_y, 9));
		p_dtg_rec_pack->accelation_x = htons((short)(char_mbtod(p_std_data->accelation_x, 6) * 10));
		p_dtg_rec_pack->accelation_y = htons((short)(char_mbtod(p_std_data->accelation_y, 6) * 10));
		p_dtg_rec_pack->day_run_dist = htons(char_mbtol(p_std_data->day_run_dist, 4)); //Km
		p_dtg_rec_pack->azimuth = htons(char_mbtol(p_std_data->azimuth, 3));
		p_dtg_rec_pack->rpm = htons(char_mbtol(p_std_data->rpm, 4));
		p_dtg_rec_pack->speed = char_mbtol(p_std_data->speed, 3);
		p_dtg_rec_pack->msec = 0x00;
		if(p_std_data->bs == 0x30)
			p_dtg_rec_pack->brake = 0;
		else
			p_dtg_rec_pack->brake = 1;
		
		p_dtg_rec_pack->gps_status = 0;
		if(p_std_data->gps_x == 0 || p_std_data->gps_y == 0)
			p_dtg_rec_pack->gps_status = 11;

		p_dtg_rec_pack->dummy1 = 0;
		p_dtg_rec_pack->dummy2 = 0;

#if defined(FRAMEWORK_SIZE_44) || defined(FRAMEWORK_SIZE_48)
		p_dtg_rec_pack->day_oil_usage = htonl(char_mbtol(p_std_data->day_oil_usage, 9));
		p_dtg_rec_pack->cumulative_oil_usage = htonl(char_mbtol(p_std_data->cumulative_oil_usage, 9));

		p_dtg_rec_pack->temperature_A = htons(0xA000);
		p_dtg_rec_pack->temperature_B = htons(0xA000);
#endif

#if defined(FRAMEWORK_SIZE_48)
		p_dtg_rec_pack->residual_oil = htonl(char_mbtol(p_std_data->residual_oil, 7));
		p_dtg_rec_pack->dummy3 = 0;
#endif


#else //No define DTG_RECORD_BIG_ENDIAN
		p_dtg_rec_pack->gps_x = char_mbtol(p_std_data->gps_x, 9);
		p_dtg_rec_pack->gps_y = char_mbtol(p_std_data->gps_y, 9);
		p_dtg_rec_pack->accelation_x = char_mbtod(p_std_data->accelation_x, 6) * 10;
		p_dtg_rec_pack->accelation_y = char_mbtod(p_std_data->accelation_y, 6) * 10;
		p_dtg_rec_pack->day_run_dist = char_mbtol(p_std_data->day_run_dist, 4); //Km
		p_dtg_rec_pack->azimuth = char_mbtol(p_std_data->azimuth, 3);
		p_dtg_rec_pack->rpm = char_mbtol(p_std_data->rpm, 4);
		p_dtg_rec_pack->speed = char_mbtol(p_std_data->speed, 3);
		p_dtg_rec_pack->msec = 0x00;
		if(p_std_data->bs == 0x30)
			p_dtg_rec_pack->brake = 0;
		else
			p_dtg_rec_pack->brake = 1;

		//printf("p_dtg_rec_pack->brake = [0x%02x]\n", p_dtg_rec_pack->brake);

		p_dtg_rec_pack->gps_status = 0;
		if(p_std_data->gps_x == 0 || p_std_data->gps_y == 0)
			p_dtg_rec_pack->gps_status = 11;

		if(power_get_ignition_status() == POWER_IGNITION_ON) 
		{
			//get_dtg_key_status
			if(g_dtg_key_status != eDTG_KeyOn && key_flag == 0)
			{
				p_dtg_rec_pack->dummy1 = eDTG_KeyOn;
				key_flag = 1;
			}
			else
			{
				p_dtg_rec_pack->dummy1 = eDTG_Key_On_Running;
				//g_dtg_key_status = eDTG_Key_On_Running;
			}
		}
		else
		{
			if( std_buff_len - (std_idx +sizeof(tacom_std_data_t)) >= sizeof(tacom_std_data_t))
			{
				p_dtg_rec_pack->dummy1 = eDTG_Key_On_Running;
				g_dtg_key_status = eDTG_Key_On_Running;
			}
			else
			{
				//if(g_dtg_key_status != eDTG_KeyOff)
				//{
					p_dtg_rec_pack->dummy1 = eDTG_KeyOff;
					g_dtg_key_status = eDTG_KeyOff;
				//}
				//else
				//{
				//	p_dtg_rec_pack->dummy1 = eDTG_Key_Off_Running;
				//	g_dtg_key_status = eDTG_Key_Off_Running;
				//}
			}
		}

		printf("p_dtg_rec_pack->dummy1 = [0x%02x]\n", p_dtg_rec_pack->dummy1);

		p_dtg_rec_pack->dummy2 = 0;

#if defined(FRAMEWORK_SIZE_44) || defined(FRAMEWORK_SIZE_48)
		p_dtg_rec_pack->day_oil_usage = char_mbtol(p_std_data->day_oil_usage, 9);
		p_dtg_rec_pack->cumulative_oil_usage = char_mbtol(p_std_data->cumulative_oil_usage, 9);

		p_dtg_rec_pack->temperature_A = 0xA000;
		p_dtg_rec_pack->temperature_B = 0xA000;
#endif

#if defined(FRAMEWORK_SIZE_48)
		p_dtg_rec_pack->residual_oil =char_mbtol(p_std_data->residual_oil, 7);
		p_dtg_rec_pack->dummy3 = 0;
#endif

#endif
		dest_idx += sizeof(dtg_record_pack_t);
		std_idx += sizeof(tacom_std_data_t);
	}

	dest_idx += 4; //reserved

	p_mhdr->msg_body_len = htonl(dest_idx - sizeof(message_define_t));
	p_mdf->msg_len = htonl(dest_idx);
	printf("total len = [%d]\n", dest_idx);
 
	return dest_idx;

}
