/**
* @file parsing.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-08
*/
#include <stdlib.h>
#include <string.h>

#include <board/power.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <standard_protocol.h>
#include <dtg_data_manage.h>
#include <wrapper/dtg_atcmd.h>
#include <wrapper/dtg_version.h>
#include <board/modem-time.h>
#include <board/board_system.h>
#include <time.h>
#include "dtg_data_manage.h"

int g_packet_id = 1;
void print_dtg_msg(char *title, char *msg, int len)
{
	char dmsg[256];
	memset(dmsg, 0x00, 256);
	memcpy(dmsg, msg, len);
	DTG_LOGI("%s : %s", title, dmsg);
}

unsigned char convert_angle(int bearing)
{
	if(bearing == 0) {
		return 0;
	} else if((bearing > 0) && (bearing < 90)) {
		return 1;
	} else if(bearing == 90) {
		return 2;
	} else if((bearing > 90) && (bearing < 180)) {
		return 3;
	} else if(bearing == 180) {
		return 4;
	} else if((bearing > 180) && (bearing < 270)) {
		return 5;
	} else if(bearing == 270) {
		return 6;
	} else if((bearing > 270) && (bearing <= 360)) {
		return 7;
	}

	return 0xff;
}

unsigned char mdt_crc16(unsigned short crc, const unsigned char *buf, int len)
{
	while(len--) {
		crc ^= *buf++;
	}

	return crc & 0xffff;
}

tacom_std_hdr_t g_current_std_hdr;
tacom_std_data_t g_current_std_data;
void set_current_dtg_data(unsigned char *std_buff, int std_buff_len)
{
	memcpy(&g_current_std_hdr, std_buff, sizeof(tacom_std_hdr_t));
printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ : set_current_dtg_data ++++\n");
	memcpy(&g_current_std_data, &std_buff[sizeof(tacom_std_hdr_t)], sizeof(tacom_std_data_t));
#if defined(DEVICE_MODEL_INNOCAR)
printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ : set_current_dtg_data-----\n");
printf("%d, %d, %d, %d\n", g_current_std_data.k_factor, g_current_std_data.rpm_factor, g_current_std_data.weight1, g_current_std_data.weight2);
	set_factor_value(g_current_std_data.k_factor, g_current_std_data.rpm_factor, g_current_std_data.weight1, g_current_std_data.weight2, g_current_std_hdr.dtg_fw_ver);
#endif

}

int bulk_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest)
{

	int src_idx = 0;
	int dest_idx = 0;
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;
	char *phonenum;
	struct tm cur_time;
	time_t system_time;
	struct tm *timeinfo;
	//unsigned short val_crc16 = 0;
	int gps_x, gps_y;
	int azimuth;
	//unsigned short speed;
	unsigned char device_type = eDTG_Device;
	unsigned char product_type = eChoyoung;


	gtrace_packet_body_t *p_gtrs_packet_body;
	gtrace_dtg_user_data_hdr_t *p_gtrs_hdr;
	gtrace_dtg_user_data_summary_t *p_gtrs_sum;
	gtrace_dtg_user_data_payload_t *p_gtrs_payload;

	p_std_hdr = (tacom_std_hdr_t *)&std_buff[src_idx];
	src_idx += sizeof(tacom_std_hdr_t);

	p_gtrs_packet_body = (gtrace_packet_body_t *)dest;
	dest_idx += sizeof(gtrace_packet_body_t);
	p_gtrs_hdr = (gtrace_dtg_user_data_hdr_t *)&dest[dest_idx];
	dest_idx += sizeof(gtrace_dtg_user_data_hdr_t);
	p_gtrs_sum = (gtrace_dtg_user_data_summary_t *)&dest[dest_idx];
	dest_idx += sizeof(gtrace_dtg_user_data_summary_t);
	//p_gtrs_payload = (gtrace_dtg_user_data_payload_t *)&dest[dest_idx];


	p_gtrs_packet_body->prot_id = 0x11;
	p_gtrs_packet_body->msg_id = 0x64;

	phonenum = atcmd_get_phonenum(); 
	memset(p_gtrs_packet_body->device_id, 0x20, sizeof(p_gtrs_packet_body->device_id));
	if(phonenum != NULL) {
		strncpy(p_gtrs_packet_body->device_id, phonenum, sizeof(p_gtrs_packet_body->device_id));
	}
	p_gtrs_packet_body->evt_code = ePeriod_Report;

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	p_gtrs_packet_body->year = timeinfo->tm_year+1900;
	p_gtrs_packet_body->month = timeinfo->tm_mon+1;
	p_gtrs_packet_body->day = timeinfo->tm_mday;
	p_gtrs_packet_body->hour = timeinfo->tm_hour;
	p_gtrs_packet_body->min = timeinfo->tm_min;
	p_gtrs_packet_body->sec = timeinfo->tm_sec;

	gps_x = char_mbtol(g_current_std_data.gps_x, 9)*10;
	gps_y = char_mbtol(g_current_std_data.gps_y, 9)*10;

	DTG_LOGI("%s> DTG SERVER gps_x, gps_y = [%d, %d]\n", __func__, gps_x, gps_y);

	if(gps_x == 0 && gps_y == 0)
		p_gtrs_packet_body->gps_status = 2;
	else
		p_gtrs_packet_body->gps_status = 1;

	p_gtrs_packet_body->gps_x = gps_x;
	p_gtrs_packet_body->gps_y = gps_y;
	azimuth = char_mbtol(g_current_std_data.azimuth, 3);
	p_gtrs_packet_body->bearing = convert_angle(azimuth);
	p_gtrs_packet_body->speed = char_mbtol(g_current_std_data.speed, 3);

	p_gtrs_packet_body->acc_dist = char_mbtol(g_current_std_data.cumul_run_dist, 7) * 1000;

	p_gtrs_packet_body->temp1 = -5555;
	p_gtrs_packet_body->temp2 = -5555;
	p_gtrs_packet_body->temp3 = -5555;
	p_gtrs_packet_body->report_period = get_dtg_report_period();
	p_gtrs_packet_body->gpio = 0;
	if(power_get_power_source() == POWER_SRC_DC)
		p_gtrs_packet_body->power = 0;
	else
		p_gtrs_packet_body->power = 1;

	p_gtrs_packet_body->create_period = 1;
	p_gtrs_packet_body->crc16 = mdt_crc16(0, (unsigned char *)p_gtrs_packet_body, sizeof(gtrace_packet_body_t)-3);
	p_gtrs_packet_body->term_char = 0x7e;


	p_gtrs_hdr->length = sizeof(gtrace_dtg_user_data_hdr_t) + sizeof(gtrace_dtg_user_data_summary_t);// + sizeof(gtrace_dtg_user_data_payload_t);

	if(g_packet_id++ > 0x0FFFFFFF)
		g_packet_id = 0;


	p_gtrs_hdr->packet_id = g_packet_id;
	memset(p_gtrs_hdr->cmd_id, '0', 4);


#if defined(DEVICE_MODEL_LOOP2)
	device_type = eDTG_Device;
	product_type = eLoop;
#elif defined(DEVICE_MODEL_UCAR)
	device_type = eDTG_Device;
	product_type = eUCAR;
#elif defined(DEVICE_MODEL_SINHUNG)
	device_type = eDTG_Device;
	product_type = eSinhung;
#elif defined(DEVICE_MODEL_LOOP)
	device_type = eDTG_Device;
	product_type = eLoop;
#elif defined(DEVICE_MODEL_CHOYOUNG)
	device_type = eDTG_Device;
	product_type = eChoyoung;
#elif defined(DEVICE_MODEL_KDT)
	device_type = eDTG_Device;
	product_type = eKDT;
#elif defined(DEVICE_MODEL_IREAL)
	device_type = eDTG_Device;
	product_type = eIREAL;
#elif defined(DEVICE_MODEL_INNOCAR)
	device_type = eDTG_Device;
	product_type = eINNOCAR;
#elif defined(DEVICE_MODEL_CJ)
	device_type = eDTG_Device;
	product_type = eCJ;
#elif defined(DEVICE_MODEL_DAESIN)
	device_type = eDTG_Device;
	product_type = eDAESIN;
#else
	#error "Unkown DTG Type"
#endif

	p_gtrs_hdr->dev_type = device_type;
	p_gtrs_hdr->product_type = product_type;

	//Summary Data
	//gtrace_dtg_user_data_summary_t *p_gtrs_sum;
	memset(p_gtrs_sum, 0x20, sizeof(gtrace_dtg_user_data_summary_t));
	strncpy(p_gtrs_sum->dtg_model, p_std_hdr->vehicle_model, sizeof(p_gtrs_sum->dtg_model));
	strncpy(p_gtrs_sum->vin, p_std_hdr->vehicle_id_num, sizeof(p_gtrs_sum->vin));
	
	p_gtrs_sum->vechicle_type[0] = p_std_hdr->vehicle_type[0];
	p_gtrs_sum->vechicle_type[1] = p_std_hdr->vehicle_type[1];
	
	memset(p_gtrs_sum->vrn, '#', sizeof(p_gtrs_sum->vrn));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_gtrs_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_gtrs_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "전국",4)) {
		memcpy(p_gtrs_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_gtrs_sum->vrn, p_std_hdr->registration_num, 12);
	}

	strncpy(p_gtrs_sum->brn, p_std_hdr->business_license_num, sizeof(p_gtrs_sum->brn));
	strncpy(p_gtrs_sum->driver_code, p_std_hdr->driver_code, sizeof(p_gtrs_sum->driver_code));
	
	/////////////////////////////////////////////////////////////////////////////
	//Payload
	/////////////////////////////////////////////////////////////////////////////
	while ((std_buff_len - src_idx) >= sizeof(tacom_std_data_t))
	{
		p_std_data = (tacom_std_data_t *)&std_buff[src_idx];
		p_gtrs_payload = (gtrace_dtg_user_data_payload_t *)&dest[dest_idx];

		p_gtrs_payload->distance_a_day = char_mbtol(p_std_data->day_run_dist, 4) * 1000;
		p_gtrs_payload->distance_all = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;


		p_gtrs_payload->year = char_mbtol(p_std_data->date_time,    2); //year
		p_gtrs_payload->month = char_mbtol(p_std_data->date_time+ 2, 2); //month
		p_gtrs_payload->day = char_mbtol(p_std_data->date_time+ 4, 2); //day

		p_gtrs_payload->hour  = char_mbtol(p_std_data->date_time+ 6, 2); //hour
		p_gtrs_payload->min = char_mbtol(p_std_data->date_time+ 8, 2); //min
		p_gtrs_payload->sec = char_mbtol(p_std_data->date_time+10, 2); //sec
		p_gtrs_payload->msec = char_mbtol(p_std_data->date_time+12, 2);

		//printf("gtrace data time : ");
		//printf("[%04d/%02d/%02d %02d:%02d:%02d]\n", p_gtrs_payload->year, p_gtrs_payload->month, p_gtrs_payload->day, 
		//											  p_gtrs_payload->hour, p_gtrs_payload->min, p_gtrs_payload->sec);

		p_gtrs_payload->speed = char_mbtol(p_std_data->speed, 3);
		p_gtrs_payload->rpm = char_mbtol(p_std_data->rpm, 4);
		p_gtrs_payload->bs = (p_std_data->bs << 7);
		p_gtrs_payload->gps_x = char_mbtol(p_std_data->gps_x, 9)*10;
		p_gtrs_payload->gps_y = char_mbtol(p_std_data->gps_y, 9)*10;
		p_gtrs_payload->azimuth = char_mbtol(p_std_data->azimuth, 3);
		p_gtrs_payload->accelation_x = (short)(char_mbtod(p_std_data->accelation_x, 6) * 10);
		p_gtrs_payload->accelation_y = (short)(char_mbtod(p_std_data->accelation_y, 6) * 10);
		p_gtrs_payload->status_code = char_mbtol(p_std_data->status, 2);
	//printf("gps_x, gps_y = [%d] [%d]\n", p_gtrs_payload->gps_x, p_gtrs_payload->gps_y);
	//printf("azimuth, speed, rpm = [%d] [%d] [%d]\n", p_gtrs_payload->azimuth, p_gtrs_payload->speed, p_gtrs_payload->rpm);

		p_gtrs_payload->rtUsedFuelAday = char_mbtol(p_std_data->day_oil_usage, 9);
		p_gtrs_payload->rtUsedFuelAll = char_mbtol(p_std_data->cumulative_oil_usage, 9);

		p_gtrs_hdr->length += sizeof(gtrace_dtg_user_data_payload_t);
		//data_size += sizeof(etrace_dtg_body_t);
		//p_etr_dtg_hdr->record_count += 1;
		dest_idx += sizeof(gtrace_dtg_user_data_payload_t);
		src_idx += sizeof(tacom_std_data_t);

	}

	return dest_idx;
}



int current_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int ev)
{
	
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;
	char *phonenum;
	struct tm cur_time;
	time_t system_time;
	struct tm *timeinfo;
	int gps_x, gps_y;
	int azimuth;
	int dest_idx = 0;
	unsigned char device_type = eDTG_Device;
	unsigned char product_type = eChoyoung;


	gtrace_packet_body_t *p_gtrs_packet_body;
	gtrace_dtg_user_data_hdr_t *p_gtrs_hdr;
	gtrace_dtg_user_data_summary_t *p_gtrs_sum;
	gtrace_dtg_user_data_payload_t *p_gtrs_payload;

	p_std_hdr = (tacom_std_hdr_t *)std_buff;
	p_std_data = (tacom_std_data_t *)&std_buff[sizeof(tacom_std_hdr_t)];

	p_gtrs_packet_body = (gtrace_packet_body_t *)dest;
	dest_idx += sizeof(gtrace_packet_body_t);
	p_gtrs_hdr = (gtrace_dtg_user_data_hdr_t *)&dest[dest_idx];
	dest_idx += sizeof(gtrace_dtg_user_data_hdr_t);
	p_gtrs_sum = (gtrace_dtg_user_data_summary_t *)&dest[dest_idx];
	dest_idx += sizeof(gtrace_dtg_user_data_summary_t);
	p_gtrs_payload = (gtrace_dtg_user_data_payload_t *)&dest[dest_idx];


	p_gtrs_packet_body->prot_id = 0x11;
	p_gtrs_packet_body->msg_id = 0x64;

	phonenum = atcmd_get_phonenum(); 
	memset(p_gtrs_packet_body->device_id, 0x20, sizeof(p_gtrs_packet_body->device_id));
	if(phonenum != NULL) {
		strncpy(p_gtrs_packet_body->device_id, phonenum, sizeof(p_gtrs_packet_body->device_id));
	}
	p_gtrs_packet_body->evt_code = ev;

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	p_gtrs_packet_body->year = timeinfo->tm_year+1900;
	p_gtrs_packet_body->month = timeinfo->tm_mon+1;
	p_gtrs_packet_body->day = timeinfo->tm_mday;
	p_gtrs_packet_body->hour = timeinfo->tm_hour;
	p_gtrs_packet_body->min = timeinfo->tm_min;
	p_gtrs_packet_body->sec = timeinfo->tm_sec;

printf("time : [%02d/%02d/%02d %02d:%02d:%02d\n", p_gtrs_packet_body->year, p_gtrs_packet_body->month, p_gtrs_packet_body->day, p_gtrs_packet_body->hour, p_gtrs_packet_body->min, p_gtrs_packet_body->sec);

	gps_x = char_mbtol(p_std_data->gps_x, 9)*10;
	gps_y = char_mbtol(p_std_data->gps_y, 9)*10;

	DTG_LOGI("%s> DTG SERVER gps_x, gps_y = [%d, %d]\n", __func__, gps_x, gps_y);

	if(gps_x == 0 && gps_y == 0)
		p_gtrs_packet_body->gps_status = 2;
	else
		p_gtrs_packet_body->gps_status = 1;

	p_gtrs_packet_body->gps_x = gps_x;
	p_gtrs_packet_body->gps_y = gps_y;
	azimuth = char_mbtol(p_std_data->azimuth, 3);
	p_gtrs_packet_body->bearing = convert_angle(azimuth);
	p_gtrs_packet_body->speed = char_mbtol(p_std_data->speed, 3);

	p_gtrs_packet_body->acc_dist = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;

	p_gtrs_packet_body->temp1 = -5555;
	p_gtrs_packet_body->temp2 = -5555;
	p_gtrs_packet_body->temp3 = -5555;
	p_gtrs_packet_body->report_period = get_dtg_report_period();
	p_gtrs_packet_body->gpio = 0;
	if(power_get_power_source() == POWER_SRC_DC)
		p_gtrs_packet_body->power = 0;
	else
		p_gtrs_packet_body->power = 1;

	p_gtrs_packet_body->create_period = 1;
	p_gtrs_packet_body->crc16 = mdt_crc16(0, (unsigned char *)p_gtrs_packet_body, sizeof(gtrace_packet_body_t)-3);
	p_gtrs_packet_body->term_char = 0x7e;


	p_gtrs_hdr->length = sizeof(gtrace_dtg_user_data_hdr_t) + sizeof(gtrace_dtg_user_data_summary_t) + sizeof(gtrace_dtg_user_data_payload_t);

	if(g_packet_id++ > 0x0FFFFFFF)
		g_packet_id = 0;


	p_gtrs_hdr->packet_id = g_packet_id;
	memset(p_gtrs_hdr->cmd_id, '0', 4);


#if defined(DEVICE_MODEL_LOOP2)
	device_type = eDTG_Device;
	product_type = eLoop;
#elif defined(DEVICE_MODEL_UCAR)
	device_type = eDTG_Device;
	product_type = eUCAR;
#elif defined(DEVICE_MODEL_SINHUNG)
	device_type = eDTG_Device;
	product_type = eSinhung;
#elif defined(DEVICE_MODEL_LOOP)
	device_type = eDTG_Device;
	product_type = eLoop;
#elif defined(DEVICE_MODEL_CHOYOUNG)
	device_type = eDTG_Device;
	product_type = eChoyoung;
#elif defined(DEVICE_MODEL_KDT)
	device_type = eDTG_Device;
	product_type = eKDT;
#elif defined(DEVICE_MODEL_IREAL)
	device_type = eDTG_Device;
	product_type = eIREAL;
#elif defined(DEVICE_MODEL_INNOCAR)
	device_type = eDTG_Device;
	product_type = eINNOCAR;
#elif defined(DEVICE_MODEL_CJ)
	device_type = eDTG_Device;
	product_type = eCJ;
#elif defined(DEVICE_MODEL_DAESIN)
	device_type = eDTG_Device;
	product_type = eDAESIN;
#else
	#error "Unkown DTG Type"
#endif

	p_gtrs_hdr->dev_type = device_type;
	p_gtrs_hdr->product_type = product_type;

	//Summary Data
	//gtrace_dtg_user_data_summary_t *p_gtrs_sum;
	memset(p_gtrs_sum, 0x20, sizeof(gtrace_dtg_user_data_summary_t));
	strncpy(p_gtrs_sum->dtg_model, p_std_hdr->vehicle_model, sizeof(p_gtrs_sum->dtg_model));
	strncpy(p_gtrs_sum->vin, p_std_hdr->vehicle_id_num, sizeof(p_gtrs_sum->vin));
	
	p_gtrs_sum->vechicle_type[0] = p_std_hdr->vehicle_type[0];
	p_gtrs_sum->vechicle_type[1] = p_std_hdr->vehicle_type[1];
	
	memset(p_gtrs_sum->vrn, '#', sizeof(p_gtrs_sum->vrn));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_gtrs_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_gtrs_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "전국",4)) {
		memcpy(p_gtrs_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_gtrs_sum->vrn, p_std_hdr->registration_num, 12);
	}

	strncpy(p_gtrs_sum->brn, p_std_hdr->business_license_num, sizeof(p_gtrs_sum->brn));
	strncpy(p_gtrs_sum->driver_code, p_std_hdr->driver_code, sizeof(p_gtrs_sum->driver_code));
	
	/////////////////////////////////////////////////////////////////////////////
	//Payload
	/////////////////////////////////////////////////////////////////////////////
	p_gtrs_payload->distance_a_day = char_mbtol(p_std_data->day_run_dist, 4) * 1000;
	p_gtrs_payload->distance_all = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;


	p_gtrs_payload->year = char_mbtol(p_std_data->date_time,    2); //year
	p_gtrs_payload->month = char_mbtol(p_std_data->date_time+ 2, 2); //month
	p_gtrs_payload->day = char_mbtol(p_std_data->date_time+ 4, 2); //day

	p_gtrs_payload->hour  = char_mbtol(p_std_data->date_time+ 6, 2); //hour
	p_gtrs_payload->min = char_mbtol(p_std_data->date_time+ 8, 2); //min
	p_gtrs_payload->sec = char_mbtol(p_std_data->date_time+10, 2); //sec
	p_gtrs_payload->msec = char_mbtol(p_std_data->date_time+12, 2);

	//printf("gtrace data time : ");
	//printf("[%04d/%02d/%02d %02d:%02d:%02d]\n", p_gtrs_payload->year, p_gtrs_payload->month, p_gtrs_payload->day, 
	//											  p_gtrs_payload->hour, p_gtrs_payload->min, p_gtrs_payload->sec);

	p_gtrs_payload->speed = char_mbtol(p_std_data->speed, 3);
	p_gtrs_payload->rpm = char_mbtol(p_std_data->rpm, 4);
	p_gtrs_payload->bs = (p_std_data->bs << 7);
	p_gtrs_payload->gps_x = char_mbtol(p_std_data->gps_x, 9)*10;
	p_gtrs_payload->gps_y = char_mbtol(p_std_data->gps_y, 9)*10;
	p_gtrs_payload->azimuth = char_mbtol(p_std_data->azimuth, 3);
	p_gtrs_payload->accelation_x = (short)(char_mbtod(p_std_data->accelation_x, 6) * 10);
	p_gtrs_payload->accelation_y = (short)(char_mbtod(p_std_data->accelation_y, 6) * 10);
	p_gtrs_payload->status_code = char_mbtol(p_std_data->status, 2);
//printf("gps_x, gps_y = [%d] [%d]\n", p_gtrs_payload->gps_x, p_gtrs_payload->gps_y);
//printf("azimuth, speed, rpm = [%d] [%d] [%d]\n", p_gtrs_payload->azimuth, p_gtrs_payload->speed, p_gtrs_payload->rpm);

	p_gtrs_payload->rtUsedFuelAday = char_mbtol(p_std_data->day_oil_usage, 9);
	p_gtrs_payload->rtUsedFuelAll = char_mbtol(p_std_data->cumulative_oil_usage, 9);

#if defined(DEVICE_MODEL_INNOCAR)
	set_factor_value(p_std_data->k_factor, p_std_data->rpm_factor, p_std_data->weight1, p_std_data->weight2, p_std_hdr->dtg_fw_ver);
#endif

	return sizeof(gtrace_packet_body_t) + sizeof(gtrace_dtg_user_data_hdr_t) + sizeof(gtrace_dtg_user_data_summary_t) + sizeof(gtrace_dtg_user_data_payload_t);
}
