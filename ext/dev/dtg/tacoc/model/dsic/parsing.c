/**
* @file parsing.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-08
*/
#include <string.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <standard_protocol.h>
#include <format_protocol.h>

static void make_mdt_pack
(mdt_packet_t *mdt_pack, tacom_std_data_t *p_std_data) {
	char *phonenum = NULL;
	int azimuth;

	mdt_pack->protocol_id = 0x11;
	mdt_pack->message_id = 0x64;
	memset(mdt_pack->terminal_id, 0x20, 15);
	phonenum  = atcmd_get_phonenum();
	memcpy(mdt_pack->terminal_id, phonenum, strlen(phonenum));

	mdt_pack->year = 2000 + char_mbtol(p_std_data->date_time, 2);
	mdt_pack->month = char_mbtol(p_std_data->date_time+2, 2);
	mdt_pack->day = char_mbtol(p_std_data->date_time+4, 2);
	mdt_pack->hour = char_mbtol(p_std_data->date_time+6, 2);
	mdt_pack->min = char_mbtol(p_std_data->date_time+8, 2);
	mdt_pack->sec = char_mbtol(p_std_data->date_time+10, 2);
	mdt_pack->position_type = 1;
	mdt_pack->gps_x = char_mbtol(p_std_data->gps_x, 9) * 10;
	mdt_pack->gps_y = char_mbtol(p_std_data->gps_y, 9) * 10;

	azimuth = char_mbtol(p_std_data->azimuth, 3);
	if (azimuth >= 338 || azimuth < 23)
		mdt_pack->azimuth = 0;
	else if (azimuth >= 23 && azimuth < 68)
		mdt_pack->azimuth = 1;
	else if (azimuth >= 68 && azimuth < 113)
		mdt_pack->azimuth = 2;
	else if (azimuth >= 113 && azimuth < 158)
		mdt_pack->azimuth = 3;
	else if (azimuth >= 158 && azimuth < 203)
		mdt_pack->azimuth = 4;
	else if (azimuth >= 203 && azimuth < 248)
		mdt_pack->azimuth = 5;
	else if (azimuth >= 248 && azimuth < 293)
		mdt_pack->azimuth = 6;
	else if (azimuth >= 293 && azimuth < 338)
		mdt_pack->azimuth = 7;

	mdt_pack->speed = char_mbtol(p_std_data->speed, 3);
	mdt_pack->cumul_dist = char_mbtol(p_std_data->day_run_dist, 4);
	mdt_pack->temperature_a = (long)(char_mbtod(p_std_data->temperature_A, 5)*10);
	mdt_pack->temperature_b = (long)(char_mbtod(p_std_data->temperature_B, 5)*10);
	mdt_pack->temperature_c = -5555;
	mdt_pack->gpio_input = 0;
	mdt_pack->power_type = 0;
	mdt_pack->terminate_char = 0x7E;
}

int dsic_ctrl_record_parsing(char *raw_buf, int raw_buf_len, char *dsicbuf)
{
	char *buf;
	int buf_len;
	int src_idx = 0;
	
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;

	buf = raw_buf;
	buf_len = raw_buf_len;

	int ret = 0;
	p_std_hdr = (tacom_std_hdr_t *)&buf[src_idx];
	src_idx += sizeof(tacom_std_hdr_t);

	p_std_data = (tacom_std_data_t *)&buf[src_idx];
	make_mdt_pack((mdt_packet_t *)dsicbuf, p_std_data);

	return sizeof(mdt_packet_t);
}

int dsic_dtg_record_parsing(char *buf, int buf_len, char *dsicbuf, int num_of_dtg)
{
	int src_idx = 0;
	int dest_idx = 0;
	int r_num = 0;
	mdt_packet_t *dsic_point = NULL;
	
	tacom_std_hdr_t *p_std_hdr;
	tacom_std_data_t *p_std_data;

	dtg_hdr_t *dtg_hdr;
	dtg_data_t *dtg_data;
	user_hdr_t *user_hdr;

	memset(dsicbuf, 0x00, sizeof(dsicbuf));
	//src_idx += 1;
	p_std_hdr = (tacom_std_hdr_t *)&buf[src_idx];
	src_idx += sizeof(tacom_std_hdr_t);

	while ((buf_len - src_idx) >= sizeof(tacom_std_data_t)) {
		dsic_point = &dsicbuf[dest_idx];
		dest_idx += sizeof(mdt_packet_t);

		user_hdr = (user_hdr_t *)&dsicbuf[dest_idx];
		memcpy(user_hdr->req_cmd_id, "    ", 4);
		user_hdr->device_type = 2;		/* FIXME */
		user_hdr->product_type = 3;		/* FIXME */

		dest_idx += sizeof(user_hdr_t);
		dtg_hdr = (dtg_hdr_t *)&dsicbuf[dest_idx];

		memcpy(dtg_hdr->vehicle_model, p_std_hdr->vehicle_model, 20);
		memcpy(dtg_hdr->vehicle_id_num, p_std_hdr->vehicle_id_num, 17);
		memcpy(dtg_hdr->vehicle_type, p_std_hdr->vehicle_type, 2);
		memcpy(dtg_hdr->registration_num, p_std_hdr->registration_num, 12);
		memcpy(dtg_hdr->business_license_num, p_std_hdr->business_license_num, 10);
		memcpy(dtg_hdr->driver_code, p_std_hdr->driver_code, 18);

		dest_idx += sizeof(dtg_hdr_t);

		while ((buf_len - src_idx) >= sizeof(tacom_std_data_t)) {
			p_std_data = (tacom_std_data_t *)&buf[src_idx];
			dtg_data = (dtg_hdr_t *)&dsicbuf[dest_idx];

			dtg_data->day_run_dist = char_mbtol(p_std_data->day_run_dist,4);
			dtg_data->cumul_run_dist = char_mbtol(p_std_data->cumul_run_dist,7);
			dtg_data->year = 2000 + char_mbtol(p_std_data->date_time ,2);
			dtg_data->month = char_mbtol(p_std_data->date_time+2 ,2);
			dtg_data->day = char_mbtol(p_std_data->date_time+4 ,2);
			dtg_data->hour = char_mbtol(p_std_data->date_time+6 ,2);
			dtg_data->min = char_mbtol(p_std_data->date_time+8 ,2);
			dtg_data->sec = char_mbtol(p_std_data->date_time+10 ,2);
			dtg_data->c_sec = char_mbtol(p_std_data->date_time+12 ,2);
			dtg_data->speed = char_mbtol(p_std_data->speed, 3);
			dtg_data->rpm = char_mbtol(p_std_data->rpm, 4);
			dtg_data->bs = p_std_data->bs - 0x30;
			dtg_data->gps_x = char_mbtol(p_std_data->gps_x, 9) * 10;
			dtg_data->gps_y = char_mbtol(p_std_data->gps_y, 9) * 10;
			dtg_data->azimuth = char_mbtol(p_std_data->azimuth, 3);
			dtg_data->accelation_x = (short)(char_mbtod(p_std_data->accelation_x, 6) * 10);
			dtg_data->accelation_y = (short)(char_mbtod(p_std_data->accelation_y, 6) * 10);
			dtg_data->status = char_mbtol(p_std_data->status, 2);
			dtg_data->day_oil_usage = char_mbtol(p_std_data->day_oil_usage, 9);
			dtg_data->cumulative_oil_usage = char_mbtol(p_std_data->cumulative_oil_usage, 9);

			src_idx += sizeof(tacom_std_data_t);
			dest_idx += sizeof(dtg_data_t);
			r_num++;
			if((r_num % num_of_dtg) == 0){
				make_mdt_pack(dsic_point, p_std_data);
				user_hdr->length = 
					sizeof(user_hdr_t) + sizeof(dtg_hdr_t)
					+ (sizeof(dtg_data_t) * num_of_dtg);
				break;
			}
		}

		if ((r_num % num_of_dtg) > 0) {
			make_mdt_pack(dsic_point, p_std_data);
			user_hdr->length = 
				sizeof(user_hdr_t) + sizeof(dtg_hdr_t)
				+ (sizeof(dtg_data_t) * (r_num % num_of_dtg));
		}
	} //end while

	DTG_LOGD("Stream Size HDR[%d] + DATA[%d] : [%d]", sizeof(dtg_hdr_t),
		sizeof(dtg_data_t) * r_num, dest_idx);

	return dest_idx;
}
