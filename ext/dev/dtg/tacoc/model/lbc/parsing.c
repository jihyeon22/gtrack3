#include <stdlib.h>
#include <string.h>

#include <wrapper/dtg_log.h>
#include <standard_protocol.h>
#include <dtg_data_manage.h>

int lbc_parsing(char *srcbuf, int src_len, char *dstbuf)
{
	DTG_LOGD("%s : %s +++\n", __FILE__, __func__);
	int src_idx = 0;
	int dst_idx = 0;
	int r_num = 0;
	char *phonenum = NULL;
	char *ptr;
	char tmp_data[20] = {0};

	tacom_std_hdr_t *std_hdr;
	tacom_std_data_t *std_data;

	tacom_lbc_hdr_t *lbc_hdr;
	tacom_lbc_data_t *lbc_data;

	std_hdr = (tacom_std_hdr_t *) &srcbuf[src_idx];
	lbc_hdr = (tacom_lbc_hdr_t *) &dstbuf[dst_idx];

	memcpy(lbc_hdr->vehicle_model, std_hdr->vehicle_model, 		
			sizeof(lbc_hdr->vehicle_model));		// MODEL
	memcpy(lbc_hdr->vehicle_id_num, std_hdr->vehicle_id_num, 
			sizeof(lbc_hdr->vehicle_id_num));		// CAR NUMBER
	memcpy(lbc_hdr->vehicle_type, std_hdr->vehicle_type,	
			sizeof(lbc_hdr->vehicle_type));			// TYPE
	memcpy(lbc_hdr->registration_num,	std_hdr->registration_num,	
			sizeof(lbc_hdr->registration_num));		// REG NUM
	memcpy(lbc_hdr->business_license_num, std_hdr->business_license_num, 
			sizeof(lbc_hdr->business_license_num));	// BUSINESS LICENCE NUM

	memset(lbc_hdr->driver_code, '#', sizeof(lbc_hdr->driver_code));
	phonenum = atcmd_get_phonenum();
	memcpy(lbc_hdr->driver_code, phonenum, strlen(phonenum));

	src_idx += sizeof(tacom_std_hdr_t);
	dst_idx += sizeof(tacom_lbc_hdr_t);

	while ((src_idx + sizeof(tacom_std_data_t)) <= src_len){
		std_data = (tacom_std_data_t *) &srcbuf[src_idx];
		lbc_data = (tacom_lbc_data_t *) &dstbuf[dst_idx];

		memset(lbc_data, 0, sizeof(tacom_lbc_data_t));

		memcpy(lbc_data->day_run_distance, std_data->day_run_dist, 4);
		memcpy(lbc_data->acumulative_run_distance, std_data->cumul_run_dist, 7);
		memcpy(lbc_data->date_time, std_data->date_time, 14);
		memcpy(lbc_data->speed, std_data->speed, 3);
		memcpy(lbc_data->rpm, std_data->rpm, 4);
		lbc_data->bs = std_data->bs;
		memcpy(lbc_data->gps_x, std_data->gps_y, 9);
		memcpy(lbc_data->gps_y, std_data->gps_x, 9);
		memcpy(lbc_data->azimuth, std_data->azimuth, 3);
		memcpy(lbc_data->accelation_x, std_data->accelation_x, 6);
		memcpy(lbc_data->accelation_y, std_data->accelation_y, 6);
		memcpy(lbc_data->status, std_data->status, 2);
		memcpy(lbc_data->acumulative_oil_usage, &std_data->cumulative_oil_usage[2], 7);
		memcpy(lbc_data->residual_oil, &std_data->residual_oil[4], 3);
		
		if (std_data->temperature_A[0] == '-')
			lbc_data->temperature_A[0] = '1';
		else
			lbc_data->temperature_A[0] = '0';
		lbc_data->temperature_A[1] = std_data->temperature_A[1];
		lbc_data->temperature_A[2] = std_data->temperature_A[2];
		lbc_data->temperature_A[3] = std_data->temperature_A[4];

		if (std_data->temperature_B[0] == '-')
			lbc_data->temperature_B[0] = '1';
		else
			lbc_data->temperature_B[0] = '0';
		lbc_data->temperature_B[1] = std_data->temperature_B[1];
		lbc_data->temperature_B[2] = std_data->temperature_B[2];
		lbc_data->temperature_B[3] = std_data->temperature_B[4];

		src_idx += sizeof(tacom_std_data_t);
		dst_idx += sizeof(tacom_lbc_data_t);
		r_num++;
	}

	DTG_LOGD("Stream Size HDR[%d] + DATA[%d] : [%d]", 
		sizeof(tacom_lbc_hdr_t), sizeof(tacom_lbc_data_t) * r_num, dst_idx);
	return dst_idx;
}

