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
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	#include "vehicle_msg.h"
	#include <board/thermometer.h>
#endif

static unsigned long event_code = 26;
static unsigned long trip_refer = 0;
static unsigned long last_gps	 = 1;

int mdt_parsing(char *destbuf, tacom_std_data_t *srcbuf)
{
	DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
	char *phonenum = NULL;
	int tmp_int = 0;
	int i;
	int ret;
	int temper;
	THERMORMETER_DATA	thermometer;

	msg_mdt_t *mdt_data = (msg_mdt_t *) destbuf;

	mdt_data->protocol_id = 0x11;
	memset(mdt_data->terminal_id, 0x20, 15);
	phonenum  = atcmd_get_phonenum();
	memcpy(mdt_data->terminal_id, phonenum, strlen(phonenum));

	mdt_data->year = 2000 + char_mbtol(srcbuf->date_time, 2);
	mdt_data->month = char_mbtol(srcbuf->date_time+2, 2);
	mdt_data->day = char_mbtol(srcbuf->date_time+4, 2);
	mdt_data->hour = char_mbtol(srcbuf->date_time+6, 2);
	mdt_data->min = char_mbtol(srcbuf->date_time+8, 2);
	mdt_data->sec = char_mbtol(srcbuf->date_time+10, 2);

	mdt_data->position_type = 1;
	mdt_data->gps_x = char_mbtol(srcbuf->gps_x, 9)*10;
	mdt_data->gps_y = char_mbtol(srcbuf->gps_y, 9)*10;

	if (mdt_data->gps_x == 0 || mdt_data->gps_y == 0) {
		mdt_data->gps_x = 0;
		mdt_data->gps_y = 0;
		last_gps = 0;
	} else {
		if(last_gps == 0) {
			last_gps = 1;
			mdt_data->event_code = 202;
		}
	}

	tmp_int = char_mbtol(srcbuf->azimuth, 3);
	if (tmp_int >= 338 || tmp_int < 23)
		mdt_data->azimuth = 0;
	else if (tmp_int >= 23 && tmp_int < 68)
		mdt_data->azimuth = 1;
	else if (tmp_int >= 68 && tmp_int < 113)
		mdt_data->azimuth = 2;
	else if (tmp_int >= 113 && tmp_int < 158)
		mdt_data->azimuth = 3;
	else if (tmp_int >= 158 && tmp_int < 203)
		mdt_data->azimuth = 4;
	else if (tmp_int >= 203 && tmp_int < 248)
		mdt_data->azimuth = 5;
	else if (tmp_int >= 248 && tmp_int < 293)
		mdt_data->azimuth = 6;
	else if (tmp_int >= 293 && tmp_int < 338)
		mdt_data->azimuth = 7;

	mdt_data->speed = char_mbtol(srcbuf->speed, 3);
	if (event_code == 26) {
		trip_refer = char_mbtol(srcbuf->cumul_run_dist, 7);
		event_code = 5;
	}
	mdt_data->on_accumulative_distance = char_mbtol(srcbuf->cumul_run_dist, 7) * 1000;
	
	//default value setting
	mdt_data->temperature_a = -5555;//char_mbtol(srcbuf->temperature_A, 5);
	mdt_data->temperature_b = -5555;//char_mbtol(srcbuf->temperature_B, 5);
	mdt_data->temperature_c = -5555;

	ret = get_tempature(&thermometer);
	if(ret >= 0) {
		for(i = 0; i < thermometer.channel; i++)
		{
			switch(thermometer.temper[i].status)
			{
				case eOK:
					temper =  thermometer.temper[i].data;
					break;
				case eOPEN:
					temper = -3333;
					break;
				case eSHORT:
					temper = -4444;
					break;
				case eUNUSED:
				case eNOK:
				default:
					temper = -5555;

			}
			DTG_LOGD("CH-%d : %d C\n", i, temper);
			if(i == 0)
				mdt_data->temperature_a = temper;
			else if(i == 1)
				mdt_data->temperature_b = temper;
			else if(i == 2)
				mdt_data->temperature_c = temper;
		}
	}
	mdt_data->gpio_input = 0;
	mdt_data->power_type = 0;

	DTG_LOGD("Complete MDT message parsing.");
	return sizeof(msg_mdt_t);
}

int term_info_parsing(char *destbuf, tacom_std_hdr_t *srcbuf)
{
	msg_hdr_t *header = (msg_hdr_t *)destbuf;
	msg_term_info_t *term_info = (msg_term_info_t *)(destbuf + sizeof(msg_hdr_t));
	char model_ver[256];
	char tmp_buf[20];

	char *phonenum  = atcmd_get_phonenum();

	header->prtc_id = 0x0003;
	header->msg_id = 0x01;
	header->svc_id = 0x02;
	int tmp_int = (sizeof(msg_term_info_t) << 1);
	header->msg_len_mark[0] = ((0x00ff0000&tmp_int) >> 16);
	header->msg_len_mark[1] = ((0x0000ff00&tmp_int) >> 8);
	header->msg_len_mark[2] = (0x000000ff&tmp_int);

	memset(term_info, 0, sizeof(msg_term_info_t));
	memcpy(term_info->dtg_model, srcbuf->vehicle_model, 20);
#if defined(BOARD_TL500S)
	strncpy(term_info->modem_model, "NEO-W200",8);
#elif defined(BOARD_TL500K)
	strncpy(term_info->modem_model, "NEO-W200K",9);
#else
	#error "Unkown Board Type!!"
#endif

	memcpy(term_info->mdn,  phonenum, 11);

	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	if (!strncmp(srcbuf->registration_num, "####",4)) {
		memset(term_info->vrn, 0, 12);
		memcpy(term_info->vrn, &srcbuf->registration_num[4], 8);
	} else if (!strncmp(srcbuf->registration_num, "0000",4)) {
		memset(term_info->vrn, 0, 12);
		memcpy(term_info->vrn, &srcbuf->registration_num[4], 8);
	} else if (!strncmp(srcbuf->registration_num, "����",4)) {
		memset(term_info->vrn, 0, 12);
		memcpy(term_info->vrn, &srcbuf->registration_num[4], 8);
	} else	{
		memcpy(term_info->vrn, srcbuf->registration_num,12);
	}
	memcpy(tmp_buf, term_info->vrn, 12);
	DTG_LOGT("TERM INFO VRN = [%s]", tmp_buf);

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	set_vrn_info(tmp_buf);
#endif

	//dtg_fw_ver
/*
	#define SVR_MODEL	"hnrt"
	#define DTG_MODEL	"cyng"
	#define SW_VERSION	"v1.00"
*/

	sprintf(model_ver, "%s-%s", DTG_MODEL, SW_VERSION);
	strncpy(term_info->modem_fw_ver, model_ver, sizeof(term_info->modem_fw_ver)-1);
	strncpy(term_info->modem_serial, atcmd_get_imei(), 15);
	DTG_LOGE("term_info->modem_fw_ver [%s]\n", term_info->modem_fw_ver);
	DTG_LOGE("term_info->modem_serial [%s]\n", term_info->modem_serial);
	
	

	DTG_LOGD("Complete Terminal infomation message parsing.");
	return (sizeof(msg_term_info_t) + sizeof(msg_hdr_t));
}

int hnrt_dtg_parsing
(char *buf, int buf_len, char *stdbuf, int num_dtg_data)
{
	DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
	int src_idx = 0;
	int dest_idx = 0;
	int r_num = 0;
	unsigned long tmp_long;
	char *phonenum = atcmd_get_phonenum(); 
	struct tm dtg_tm;
	char tmp_buf[20];
	int tmp_option_flag;

	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;

	msg_hdr_t *hnrt_msg_hdr;
	msg_dtg_t *hnrt_dtg_msg;
	dtg_data_t *hnrt_dtg_data;

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	vehicle_gps_t vgps_info;
#endif

	std_hdr = (tacom_std_hdr_t *) &buf[src_idx];
#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
	set_vrn_info(std_hdr->registration_num);
#endif
	src_idx += sizeof(tacom_std_hdr_t);

	while ((buf_len - src_idx) > sizeof(dtg_data_t)) {
		if ((r_num % num_dtg_data) == 0) {
			hnrt_msg_hdr = (msg_hdr_t *) &stdbuf[dest_idx];
			dest_idx += sizeof(msg_hdr_t);
			hnrt_dtg_msg = (msg_dtg_t *) &stdbuf[dest_idx];
			dest_idx += sizeof(msg_dtg_t);

			hnrt_msg_hdr->prtc_id = 0x0003;
			hnrt_msg_hdr->msg_id = 0x02;
			hnrt_msg_hdr->svc_id = 0x02;

			hnrt_dtg_msg->tid = strtol(phonenum, NULL, 10);
			memset(hnrt_dtg_msg->option_flag, 0, 3);
#if defined(DEVICE_MODEL_LOOP2)
			tmp_option_flag = eOptionUsedFuel;
			hnrt_dtg_msg->option_flag[0] = ((0x00ff0000&tmp_option_flag) >> 16);
			hnrt_dtg_msg->option_flag[1] = ((0x0000ff00&tmp_option_flag) >> 8);
			hnrt_dtg_msg->option_flag[2] = (0x000000ff&tmp_option_flag);
#endif

			memcpy(hnrt_dtg_msg->dtg_hdr.dtg_model, std_hdr->vehicle_model, 20);
			memcpy(hnrt_dtg_msg->dtg_hdr.vin, std_hdr->vehicle_id_num, 17);
			hnrt_dtg_msg->dtg_hdr.vechicle_type = char_mbtol(std_hdr->vehicle_type, 2);

			memset(tmp_buf, 0x00, sizeof(tmp_buf));
			if (!strncmp(std_hdr->registration_num, "####",4)) {
				memset(hnrt_dtg_msg->dtg_hdr.vrn, 0, 12);
				memcpy(hnrt_dtg_msg->dtg_hdr.vrn, &std_hdr->registration_num[4], 8);
			} else if (!strncmp(std_hdr->registration_num, "0000",4)) {
				memset(hnrt_dtg_msg->dtg_hdr.vrn, 0, 12);
				memcpy(hnrt_dtg_msg->dtg_hdr.vrn, &std_hdr->registration_num[4], 8);
			} else if (!strncmp(std_hdr->registration_num, "����",4)) {
				memset(hnrt_dtg_msg->dtg_hdr.vrn, 0, 12);
				memcpy(hnrt_dtg_msg->dtg_hdr.vrn, &std_hdr->registration_num[4], 8);
			} else {
				memcpy(hnrt_dtg_msg->dtg_hdr.vrn, std_hdr->registration_num, 12);
			}
			memcpy(tmp_buf, hnrt_dtg_msg->dtg_hdr.vrn, 12);
			DTG_LOGT("DTG VRN[%s]\n", tmp_buf);

			memcpy(hnrt_dtg_msg->dtg_hdr.brn, std_hdr->business_license_num, 10);
			memcpy(hnrt_dtg_msg->dtg_hdr.driver_code, std_hdr->driver_code, 18);
		}

		std_data = (tacom_std_data_t *) &buf[src_idx];
		hnrt_dtg_data = (dtg_data_t *) &stdbuf[dest_idx];

		dtg_tm.tm_year = 100 + char_mbtol(std_data->date_time, 2);
		dtg_tm.tm_mon = char_mbtol(std_data->date_time+2, 2) - 1;
		dtg_tm.tm_mday = char_mbtol(std_data->date_time+4, 2);
		dtg_tm.tm_hour = char_mbtol(std_data->date_time+6, 2);
		dtg_tm.tm_min = char_mbtol(std_data->date_time+8, 2);
		dtg_tm.tm_sec = char_mbtol(std_data->date_time+10, 2);
		hnrt_dtg_data->timestamp = mktime(&dtg_tm);
		hnrt_dtg_data->timestampmsec = char_mbtol(std_data->date_time+12, 2);
		hnrt_dtg_data->distance_a_day = char_mbtol(std_data->day_run_dist, 4) * 1000;
		hnrt_dtg_data->distance_all = char_mbtol(std_data->cumul_run_dist, 7) * 1000;

		hnrt_dtg_data->distance_trip = (char_mbtol(std_data->cumul_run_dist, 7) - trip_refer) * 1000;

		hnrt_dtg_data->speed = char_mbtol(std_data->speed, 3);
		hnrt_dtg_data->rpm = char_mbtol(std_data->rpm, 4);
		hnrt_dtg_data->bs = (std_data->bs << 7);
		hnrt_dtg_data->gps_x = char_mbtol(std_data->gps_x, 9) * 10;
		hnrt_dtg_data->gps_y = char_mbtol(std_data->gps_y, 9) * 10;
		hnrt_dtg_data->azimuth = char_mbtol(std_data->azimuth, 3);
		hnrt_dtg_data->accelation_x = (short)(char_mbtod(std_data->accelation_x, 6) * 10);
		hnrt_dtg_data->accelation_y = (short)(char_mbtod(std_data->accelation_y, 6) * 10);
		hnrt_dtg_data->status_code = char_mbtol(std_data->status, 2);

#if defined(DEVICE_MODEL_LOOP2)
		hnrt_dtg_data->rtUsedFuelAday = char_mbtol(std_data->day_oil_usage, 9);
		hnrt_dtg_data->rtUsedFuelAll = char_mbtol(std_data->cumulative_oil_usage, 9);
		hnrt_dtg_data->rtUsedFuelTrip = 0;
		//printf("rtUsedFuelAday[%d], rtUsedFuelAll[%d]\n", hnrt_dtg_data->rtUsedFuelAday, hnrt_dtg_data->rtUsedFuelAll);
#endif


#if defined(BOARD_TL500S) || defined(BOARD_TL500K)
		vgps_info.speed = hnrt_dtg_data->speed;
		vgps_info.distantall = hnrt_dtg_data->distance_all;
		vgps_info.azimuth = hnrt_dtg_data->azimuth;
		vgps_info.gps_x = hnrt_dtg_data->gps_x;
		vgps_info.gps_y = hnrt_dtg_data->gps_y;
		set_vgps_info_memory(vgps_info);
#endif

		src_idx += sizeof(tacom_std_data_t);
		dest_idx += sizeof(dtg_data_t);
		r_num++;
		if ((r_num % num_dtg_data) == 0) {
			hnrt_dtg_msg->dtg_num = num_dtg_data;
			tmp_long = (sizeof(msg_dtg_t) + 
					(sizeof(dtg_data_t) * num_dtg_data)) << 1;
			hnrt_msg_hdr->msg_len_mark[0] = (tmp_long & 0x00ff0000) >> 16;
			hnrt_msg_hdr->msg_len_mark[1] = (tmp_long & 0x0000ff00) >> 8;
			hnrt_msg_hdr->msg_len_mark[2] = 
					(tmp_long & 0x000000ff) & 0xfffffffe;
		}
	}
	if ((r_num % num_dtg_data) > 0) {
		hnrt_dtg_msg->dtg_num = (r_num % num_dtg_data);
		tmp_long = (sizeof(msg_dtg_t) + 
				(sizeof(dtg_data_t) * (r_num % num_dtg_data))) << 1;
		hnrt_msg_hdr->msg_len_mark[0] = (tmp_long & 0x00ff0000) >> 16;
		hnrt_msg_hdr->msg_len_mark[1] = (tmp_long & 0x0000ff00) >> 8;
		hnrt_msg_hdr->msg_len_mark[2] = 
				(tmp_long & 0x000000ff) & 0xfffffffe;
	} else {
		hnrt_msg_hdr->msg_len_mark[2] |= 0x1;
	}
	DTG_LOGD("Complete DTG message parsing.");
	DTG_LOGD("Source length [%d] | Destination length [%d] records [%d]", 
		src_idx, dest_idx, r_num);
	return dest_idx;
}
