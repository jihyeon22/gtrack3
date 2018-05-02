#include <stdlib.h>
#include <string.h>

#include <board/power.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <standard_protocol.h>

#include <wrapper/dtg_atcmd.h>
#include <wrapper/dtg_version.h>
#include <board/modem-time.h>
#include <board/board_system.h>

#include <time.h>

#include <dsic_dtg_data_manage.h>


tacom_std_hdr_t g_current_std_hdr;
tacom_std_data_t g_current_std_data;

#define DTG_MAX_DEV_ID_LED			15


int g_packet_id = 1;

void print_dtg_msg(char *title, char *msg, int len)
{
	char dmsg[256];
	memset(dmsg, 0x00, 256);
	memcpy(dmsg, msg, len);
	DTG_LOGI("%s : %s", title, dmsg);
}

unsigned char dtg_dsic__convert_angle(int bearing)
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

unsigned char dtg_dsic_crc16(unsigned short crc, const unsigned char *buf, int len)
{
	while(len--) {
		crc ^= *buf++;
	}

	return crc & 0xffff;
}



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





// ------------------------------------
// ------------------------------------
// ------------------------------------

int bulk_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest)
{
	int src_idx = 0;
	int dest_idx = 0;
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;

	struct tm cur_time;
	time_t system_time;
	struct tm *timeinfo;
	//unsigned short val_crc16 = 0;
	int gps_x, gps_y;
	int azimuth;
	//unsigned short speed;
	unsigned char device_type = eDTG_Device;
	unsigned char product_type = eChoyoung;

    char phonenum[DTG_MAX_DEV_ID_LED];

	dtg_dsic_packet_body_t *p_dtg_packet_body;
	dtg_dsic_user_data_hdr_t *p_dtg_packet_hdr;
	dtg_dsic_user_data_summary_t *p_dtg_packet_sum;
	dtg_dsic_user_data_payload_t *p_dtg_packet_payload;

	p_std_hdr = (tacom_std_hdr_t *)&std_buff[src_idx];
	src_idx += sizeof(tacom_std_hdr_t);

	p_dtg_packet_body = (dtg_dsic_packet_body_t *)dest;
	dest_idx += sizeof(dtg_dsic_packet_body_t);
	p_dtg_packet_hdr = (dtg_dsic_user_data_hdr_t *)&dest[dest_idx];
	dest_idx += sizeof(dtg_dsic_user_data_hdr_t);
	p_dtg_packet_sum = (dtg_dsic_user_data_summary_t *)&dest[dest_idx];
	dest_idx += sizeof(dtg_dsic_user_data_summary_t);
	//p_dtg_packet_payload = (dtg_dsic_user_data_payload_t *)&dest[dest_idx];

    at_get_phonenum(phonenum, DTG_MAX_DEV_ID_LED);
    
#if defined(SERVER_MODEL_MORAM)
	p_dtg_packet_body->SOH = 0x7B;
#endif

	p_dtg_packet_body->prot_id = 0x11;
	p_dtg_packet_body->msg_id = 0x64;

	memset(p_dtg_packet_body->device_id, 0x20, sizeof(p_dtg_packet_body->device_id));
	strncpy(p_dtg_packet_body->device_id, phonenum, sizeof(p_dtg_packet_body->device_id));

	p_dtg_packet_body->evt_code = ePeriod_Report;

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	p_dtg_packet_body->year = timeinfo->tm_year+1900;
	p_dtg_packet_body->month = timeinfo->tm_mon+1;
	p_dtg_packet_body->day = timeinfo->tm_mday;
	p_dtg_packet_body->hour = timeinfo->tm_hour;
	p_dtg_packet_body->min = timeinfo->tm_min;
	p_dtg_packet_body->sec = timeinfo->tm_sec;

	gps_x = char_mbtol(g_current_std_data.gps_x, 9)*10;
	gps_y = char_mbtol(g_current_std_data.gps_y, 9)*10;

	DTG_LOGI("%s> DTG SERVER gps_x, gps_y = [%d, %d]\n", __func__, gps_x, gps_y);

	if(gps_x == 0 && gps_y == 0)
		p_dtg_packet_body->gps_status = 2;
	else
		p_dtg_packet_body->gps_status = 1;

	p_dtg_packet_body->gps_x = gps_x;
	p_dtg_packet_body->gps_y = gps_y;
	azimuth = char_mbtol(g_current_std_data.azimuth, 3);
	p_dtg_packet_body->bearing = dtg_dsic__convert_angle(azimuth);
	p_dtg_packet_body->speed = char_mbtol(g_current_std_data.speed, 3);

	p_dtg_packet_body->acc_dist = char_mbtol(g_current_std_data.cumul_run_dist, 7) * 1000;

	p_dtg_packet_body->temp1 = -5555;
	p_dtg_packet_body->temp2 = -5555;
	p_dtg_packet_body->temp3 = -5555;
	p_dtg_packet_body->report_period = get_dtg_report_period();
	p_dtg_packet_body->gpio = 0;

	if (power_get_power_source() == POWER_SRC_DC)
		p_dtg_packet_body->power = 0;
	else
		p_dtg_packet_body->power = 1;

	p_dtg_packet_body->create_period = 1;
	p_dtg_packet_body->crc16 = dtg_dsic_crc16(0, (unsigned char *)p_dtg_packet_body, sizeof(dtg_dsic_packet_body_t)-3);
	p_dtg_packet_body->term_char = 0x7e;


	p_dtg_packet_hdr->length = sizeof(dtg_dsic_user_data_hdr_t) + sizeof(dtg_dsic_user_data_summary_t);// + sizeof(dtg_dsic_user_data_payload_t);

	if(g_packet_id++ > 0x0FFFFFFF)
		g_packet_id = 0;


	p_dtg_packet_hdr->packet_id = g_packet_id;
	memset(p_dtg_packet_hdr->cmd_id, '0', 4);


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

	p_dtg_packet_hdr->dev_type = device_type;
	p_dtg_packet_hdr->product_type = product_type;

	//Summary Data
	//dtg_dsic_user_data_summary_t *p_dtg_packet_sum;
	memset(p_dtg_packet_sum, 0x20, sizeof(dtg_dsic_user_data_summary_t));
	strncpy(p_dtg_packet_sum->dtg_model, p_std_hdr->vehicle_model, sizeof(p_dtg_packet_sum->dtg_model));
	strncpy(p_dtg_packet_sum->vin, p_std_hdr->vehicle_id_num, sizeof(p_dtg_packet_sum->vin));
	
	p_dtg_packet_sum->vechicle_type[0] = p_std_hdr->vehicle_type[0];
	p_dtg_packet_sum->vechicle_type[1] = p_std_hdr->vehicle_type[1];
	
	memset(p_dtg_packet_sum->vrn, '#', sizeof(p_dtg_packet_sum->vrn));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_dtg_packet_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_dtg_packet_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "����",4)) {
		memcpy(p_dtg_packet_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_dtg_packet_sum->vrn, p_std_hdr->registration_num, 12);
	}

	strncpy(p_dtg_packet_sum->brn, p_std_hdr->business_license_num, sizeof(p_dtg_packet_sum->brn));
	strncpy(p_dtg_packet_sum->driver_code, p_std_hdr->driver_code, sizeof(p_dtg_packet_sum->driver_code));
	
	/////////////////////////////////////////////////////////////////////////////
	//Payload
	/////////////////////////////////////////////////////////////////////////////
	while ((std_buff_len - src_idx) >= sizeof(tacom_std_data_t))
	{
		p_std_data = (tacom_std_data_t *)&std_buff[src_idx];
		p_dtg_packet_payload = (dtg_dsic_user_data_payload_t *)&dest[dest_idx];

		p_dtg_packet_payload->distance_a_day = char_mbtol(p_std_data->day_run_dist, 4) * 1000;
		p_dtg_packet_payload->distance_all = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;


		p_dtg_packet_payload->year = char_mbtol(p_std_data->date_time,    2); //year
		p_dtg_packet_payload->month = char_mbtol(p_std_data->date_time+ 2, 2); //month
		p_dtg_packet_payload->day = char_mbtol(p_std_data->date_time+ 4, 2); //day

		p_dtg_packet_payload->hour  = char_mbtol(p_std_data->date_time+ 6, 2); //hour
		p_dtg_packet_payload->min = char_mbtol(p_std_data->date_time+ 8, 2); //min
		p_dtg_packet_payload->sec = char_mbtol(p_std_data->date_time+10, 2); //sec
		p_dtg_packet_payload->msec = char_mbtol(p_std_data->date_time+12, 2);

		//printf("gtrace data time : ");
		//printf("[%04d/%02d/%02d %02d:%02d:%02d]\n", p_dtg_packet_payload->year, p_dtg_packet_payload->month, p_dtg_packet_payload->day, 
		//											  p_dtg_packet_payload->hour, p_dtg_packet_payload->min, p_dtg_packet_payload->sec);

		p_dtg_packet_payload->speed = char_mbtol(p_std_data->speed, 3);
		p_dtg_packet_payload->rpm = char_mbtol(p_std_data->rpm, 4);
		p_dtg_packet_payload->bs = (p_std_data->bs << 7);
		p_dtg_packet_payload->gps_x = char_mbtol(p_std_data->gps_x, 9)*10;
		p_dtg_packet_payload->gps_y = char_mbtol(p_std_data->gps_y, 9)*10;
		p_dtg_packet_payload->azimuth = char_mbtol(p_std_data->azimuth, 3);
		p_dtg_packet_payload->accelation_x = (short)(char_mbtod(p_std_data->accelation_x, 6) * 10);
		p_dtg_packet_payload->accelation_y = (short)(char_mbtod(p_std_data->accelation_y, 6) * 10);
		p_dtg_packet_payload->status_code = char_mbtol(p_std_data->status, 2);
	//printf("gps_x, gps_y = [%d] [%d]\n", p_dtg_packet_payload->gps_x, p_dtg_packet_payload->gps_y);
	//printf("azimuth, speed, rpm = [%d] [%d] [%d]\n", p_dtg_packet_payload->azimuth, p_dtg_packet_payload->speed, p_dtg_packet_payload->rpm);

		p_dtg_packet_payload->rtUsedFuelAday = char_mbtol(p_std_data->day_oil_usage, 9);
		p_dtg_packet_payload->rtUsedFuelAll = char_mbtol(p_std_data->cumulative_oil_usage, 9);

		p_dtg_packet_hdr->length += sizeof(dtg_dsic_user_data_payload_t);
		//data_size += sizeof(etrace_dtg_body_t);
		//p_etr_dtg_hdr->record_count += 1;
		dest_idx += sizeof(dtg_dsic_user_data_payload_t);
		src_idx += sizeof(tacom_std_data_t);

	}

	return dest_idx;
}



int current_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int ev)
{
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;

	tacom_std_hdr_t tmp_std_hdr;
	tacom_std_data_t tmp_std_data;
    
	struct tm cur_time;
	time_t system_time;
	struct tm *timeinfo;
	int gps_x, gps_y;
	int azimuth;
	int dest_idx = 0;
	unsigned char device_type = eDTG_Device;
	unsigned char product_type = eChoyoung;

    char phonenum[DTG_MAX_DEV_ID_LED];

	dtg_dsic_packet_body_t *p_dtg_packet_body;
	dtg_dsic_user_data_hdr_t *p_dtg_packet_hdr;
	dtg_dsic_user_data_summary_t *p_dtg_packet_sum;
	dtg_dsic_user_data_payload_t *p_dtg_packet_payload;

    if ( std_buff == NULL)
    {
        memcpy(&tmp_std_hdr, &g_current_std_hdr , sizeof(tacom_std_hdr_t));
        memcpy(&tmp_std_data, &g_current_std_data , sizeof(tacom_std_data_t));

        p_std_hdr = &tmp_std_hdr;
        p_std_data = &tmp_std_data;
    }
    else
    {
	    p_std_hdr = (tacom_std_hdr_t *)std_buff;
	    p_std_data = (tacom_std_data_t *)&std_buff[sizeof(tacom_std_hdr_t)];
    }


	p_dtg_packet_body = (dtg_dsic_packet_body_t *)dest;
	dest_idx += sizeof(dtg_dsic_packet_body_t);
	p_dtg_packet_hdr = (dtg_dsic_user_data_hdr_t *)&dest[dest_idx];
	dest_idx += sizeof(dtg_dsic_user_data_hdr_t);
	p_dtg_packet_sum = (dtg_dsic_user_data_summary_t *)&dest[dest_idx];
	dest_idx += sizeof(dtg_dsic_user_data_summary_t);
	p_dtg_packet_payload = (dtg_dsic_user_data_payload_t *)&dest[dest_idx];


#if defined(SERVER_MODEL_MORAM)
	p_dtg_packet_body->SOH = 0x7B;
#endif

    at_get_phonenum(phonenum, DTG_MAX_DEV_ID_LED);

	p_dtg_packet_body->prot_id = 0x11;
	p_dtg_packet_body->msg_id = 0x64;

	memset(p_dtg_packet_body->device_id, 0x20, sizeof(p_dtg_packet_body->device_id));
	strncpy(p_dtg_packet_body->device_id, phonenum, sizeof(p_dtg_packet_body->device_id));

	p_dtg_packet_body->evt_code = ev;

	if(get_modem_time_tm(&cur_time) != MODEM_TIME_RET_SUCCESS) {
		time(&system_time);
		timeinfo = localtime ( &system_time );
	}
	else {
		timeinfo = (struct tm *)&cur_time;
	}

	p_dtg_packet_body->year = timeinfo->tm_year+1900;
	p_dtg_packet_body->month = timeinfo->tm_mon+1;
	p_dtg_packet_body->day = timeinfo->tm_mday;
	p_dtg_packet_body->hour = timeinfo->tm_hour;
	p_dtg_packet_body->min = timeinfo->tm_min;
	p_dtg_packet_body->sec = timeinfo->tm_sec;

    printf("time : [%02d/%02d/%02d %02d:%02d:%02d\n", p_dtg_packet_body->year, p_dtg_packet_body->month, p_dtg_packet_body->day, p_dtg_packet_body->hour, p_dtg_packet_body->min, p_dtg_packet_body->sec);

	gps_x = char_mbtol(p_std_data->gps_x, 9)*10;
	gps_y = char_mbtol(p_std_data->gps_y, 9)*10;

	DTG_LOGI("%s> DTG SERVER gps_x, gps_y = [%d, %d]\n", __func__, gps_x, gps_y);

	if(gps_x == 0 && gps_y == 0)
		p_dtg_packet_body->gps_status = 2;
	else
		p_dtg_packet_body->gps_status = 1;

	p_dtg_packet_body->gps_x = gps_x;
	p_dtg_packet_body->gps_y = gps_y;
	azimuth = char_mbtol(p_std_data->azimuth, 3);
	p_dtg_packet_body->bearing = dtg_dsic__convert_angle(azimuth);
	p_dtg_packet_body->speed = char_mbtol(p_std_data->speed, 3);

	p_dtg_packet_body->acc_dist = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;

	p_dtg_packet_body->temp1 = -5555;
	p_dtg_packet_body->temp2 = -5555;
	p_dtg_packet_body->temp3 = -5555;
	p_dtg_packet_body->report_period = get_dtg_report_period();
	p_dtg_packet_body->gpio = 0;
	if(power_get_power_source() == POWER_SRC_DC)
		p_dtg_packet_body->power = 0;
	else
		p_dtg_packet_body->power = 1;

	p_dtg_packet_body->create_period = 1;
	p_dtg_packet_body->crc16 = dtg_dsic_crc16(0, (unsigned char *)p_dtg_packet_body, sizeof(dtg_dsic_packet_body_t)-3);
	p_dtg_packet_body->term_char = 0x7e;


	p_dtg_packet_hdr->length = sizeof(dtg_dsic_user_data_hdr_t) + sizeof(dtg_dsic_user_data_summary_t) + sizeof(dtg_dsic_user_data_payload_t);

	if(g_packet_id++ > 0x0FFFFFFF)
		g_packet_id = 0;


	p_dtg_packet_hdr->packet_id = g_packet_id;
	memset(p_dtg_packet_hdr->cmd_id, '0', 4);


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

	p_dtg_packet_hdr->dev_type = device_type;
	p_dtg_packet_hdr->product_type = product_type;

	//Summary Data
	//dtg_dsic_user_data_summary_t *p_dtg_packet_sum;
	memset(p_dtg_packet_sum, 0x20, sizeof(dtg_dsic_user_data_summary_t));
	strncpy(p_dtg_packet_sum->dtg_model, p_std_hdr->vehicle_model, sizeof(p_dtg_packet_sum->dtg_model));
	strncpy(p_dtg_packet_sum->vin, p_std_hdr->vehicle_id_num, sizeof(p_dtg_packet_sum->vin));
	
	p_dtg_packet_sum->vechicle_type[0] = p_std_hdr->vehicle_type[0];
	p_dtg_packet_sum->vechicle_type[1] = p_std_hdr->vehicle_type[1];
	
	memset(p_dtg_packet_sum->vrn, '#', sizeof(p_dtg_packet_sum->vrn));
	if (!strncmp(p_std_hdr->registration_num, "####",4)) {
		memcpy(p_dtg_packet_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "0000",4)) {
		memcpy(p_dtg_packet_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else if (!strncmp(p_std_hdr->registration_num, "전국",4)) {
		memcpy(p_dtg_packet_sum->vrn, &p_std_hdr->registration_num[4], 8);
	} else {
		memcpy(p_dtg_packet_sum->vrn, p_std_hdr->registration_num, 12);
	}

	strncpy(p_dtg_packet_sum->brn, p_std_hdr->business_license_num, sizeof(p_dtg_packet_sum->brn));
	strncpy(p_dtg_packet_sum->driver_code, p_std_hdr->driver_code, sizeof(p_dtg_packet_sum->driver_code));
	
	/////////////////////////////////////////////////////////////////////////////
	//Payload
	/////////////////////////////////////////////////////////////////////////////
	p_dtg_packet_payload->distance_a_day = char_mbtol(p_std_data->day_run_dist, 4) * 1000;
	p_dtg_packet_payload->distance_all = char_mbtol(p_std_data->cumul_run_dist, 7) * 1000;


	p_dtg_packet_payload->year = char_mbtol(p_std_data->date_time,    2); //year
	p_dtg_packet_payload->month = char_mbtol(p_std_data->date_time+ 2, 2); //month
	p_dtg_packet_payload->day = char_mbtol(p_std_data->date_time+ 4, 2); //day

	p_dtg_packet_payload->hour  = char_mbtol(p_std_data->date_time+ 6, 2); //hour
	p_dtg_packet_payload->min = char_mbtol(p_std_data->date_time+ 8, 2); //min
	p_dtg_packet_payload->sec = char_mbtol(p_std_data->date_time+10, 2); //sec
	p_dtg_packet_payload->msec = char_mbtol(p_std_data->date_time+12, 2);

	//printf("gtrace data time : ");
	//printf("[%04d/%02d/%02d %02d:%02d:%02d]\n", p_dtg_packet_payload->year, p_dtg_packet_payload->month, p_dtg_packet_payload->day, 
	//											  p_dtg_packet_payload->hour, p_dtg_packet_payload->min, p_dtg_packet_payload->sec);

	p_dtg_packet_payload->speed = char_mbtol(p_std_data->speed, 3);
	p_dtg_packet_payload->rpm = char_mbtol(p_std_data->rpm, 4);
	p_dtg_packet_payload->bs = (p_std_data->bs << 7);
	p_dtg_packet_payload->gps_x = char_mbtol(p_std_data->gps_x, 9)*10;
	p_dtg_packet_payload->gps_y = char_mbtol(p_std_data->gps_y, 9)*10;
	p_dtg_packet_payload->azimuth = char_mbtol(p_std_data->azimuth, 3);
	p_dtg_packet_payload->accelation_x = (short)(char_mbtod(p_std_data->accelation_x, 6) * 10);
	p_dtg_packet_payload->accelation_y = (short)(char_mbtod(p_std_data->accelation_y, 6) * 10);
	p_dtg_packet_payload->status_code = char_mbtol(p_std_data->status, 2);
//printf("gps_x, gps_y = [%d] [%d]\n", p_dtg_packet_payload->gps_x, p_dtg_packet_payload->gps_y);
//printf("azimuth, speed, rpm = [%d] [%d] [%d]\n", p_dtg_packet_payload->azimuth, p_dtg_packet_payload->speed, p_dtg_packet_payload->rpm);

	p_dtg_packet_payload->rtUsedFuelAday = char_mbtol(p_std_data->day_oil_usage, 9);
	p_dtg_packet_payload->rtUsedFuelAll = char_mbtol(p_std_data->cumulative_oil_usage, 9);

#if defined(DEVICE_MODEL_INNOCAR)
	set_factor_value(p_std_data->k_factor, p_std_data->rpm_factor, p_std_data->weight1, p_std_data->weight2, p_std_hdr->dtg_fw_ver);
#endif
    dest_idx += sizeof(dtg_dsic_user_data_payload_t);

	return dest_idx;
}



int dtg_dsic__make_bulk_pkt(char* stream, int len, char** buf)
{
    int cnt = 0;
    int pack_buffer_size = 0;
    int send_pack_size = 0;
    unsigned char* dtg_pack_buf = NULL;
    
    cnt = (len - sizeof(tacom_std_hdr_t)) / sizeof(tacom_std_data_t) + 1;
	pack_buffer_size = sizeof(dtg_dsic_packet_body_t) + sizeof(dtg_dsic_user_data_hdr_t) + sizeof(dtg_dsic_user_data_summary_t) + (sizeof(dtg_dsic_user_data_payload_t) * (cnt+5)) + 2;

#ifdef SERVER_ABBR_BICD
    pack_buffer_size += sizeof(gtrace_dtg_pkt_suffix_t);
#endif
	//DTG_LOGD("pack_buffer malloc size = [%d]\n", pack_buffer_size);

	dtg_pack_buf = (unsigned char *)malloc(pack_buffer_size);
    
	if(dtg_pack_buf == NULL) {
		DTG_LOGE("dtg_buf mallock Error");
		return -1;
	}

	send_pack_size = bulk_dtg_parsing(stream, len, dtg_pack_buf);

#ifdef SERVER_ABBR_BICD
    dtg_pack_buf[pack_buffer_size - 4] = 0xff;
    dtg_pack_buf[pack_buffer_size - 3] = 0xff;
    dtg_pack_buf[pack_buffer_size - 2] = 0xff;
    dtg_pack_buf[pack_buffer_size - 1] = 0xff;
#endif

    *buf = dtg_pack_buf;



	return pack_buffer_size;
}


int dtg_dsic__make_evt_pkt(char* stream, int len, char** buf, int evt)
{
    int cnt = 0;
    int pack_buffer_size = 0;
    int send_pack_size = 0;
    unsigned char* dtg_pack_buf = NULL;
    
    pack_buffer_size = sizeof(dtg_dsic_packet_body_t) + sizeof(dtg_dsic_user_data_hdr_t) + sizeof(dtg_dsic_user_data_summary_t) + sizeof(dtg_dsic_user_data_payload_t);
    
#ifdef SERVER_ABBR_BICD
    pack_buffer_size += sizeof(gtrace_dtg_pkt_suffix_t);
#endif

	//DTG_LOGD("pack_buffer malloc size = [%d]\n", pack_buffer_size);

	dtg_pack_buf = (unsigned char *)malloc(pack_buffer_size);
    
	if(dtg_pack_buf == NULL) {
		DTG_LOGE("dtg_buf mallock Error");
		return -1;
	}

	send_pack_size = current_dtg_parsing(stream, len, dtg_pack_buf, evt);

#ifdef SERVER_ABBR_BICD
    dtg_pack_buf[pack_buffer_size - 4] = 0xff;
    dtg_pack_buf[pack_buffer_size - 3] = 0xff;
    dtg_pack_buf[pack_buffer_size - 2] = 0xff;
    dtg_pack_buf[pack_buffer_size - 1] = 0xff;
#endif

    *buf = dtg_pack_buf;

	return pack_buffer_size;
}


void dtg_dsic__send_key_evt(int power)
{
    int event_code = 0;
    if ( power )
        event_code = eKeyOn_Event;
    else
        event_code = eKeyOff_Event;

    //send_pack_size = current_dtg_parsing(stream, len, g_packet_buffer, event_code);
    
}

void dtg_dsic__send_power_evt(int power)
{
    int send_pack_size = 0;
    //send_pack_size = current_dtg_parsing(stream, len, g_packet_buffer, ePower_Event);
    //DTG_LOGD("trasfer packet size = [%d]\n", send_pack_size);
    //if(send_record_data_msg(g_packet_buffer, send_pack_size, __LINE__, "DTG Power Source") < 0) {
    //    return -2222; //network error
    //}	
}
