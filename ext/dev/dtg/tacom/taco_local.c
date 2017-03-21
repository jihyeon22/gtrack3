/**
 *       @file  taco_local.c
 *      @brief  taco service local procedure
 *
 * Detailed description starts here.
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013??02??27?? *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technologt, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <tacom_internal.h>



//#include <taco_local.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>

extern pthread_mutex_t cmd_mutex;
void taco_to_tacoc_clnt_reconnect();

int 
taco_request(void)
{
	DTG_LOGI("RECV REQUEST!!");
	//int unread_cnt;
	enum tacom_stat status;
	TACOM *tm = tacom_get_cur_context();
	/* ?�태 체크 */
	status = tacom_get_status();
	if (status != TACOM_IDLE) {
		return status;
	}

	tacom_set_status(TACOM_BUSY_NORMAL);
	DTG_LOGI("CHANGE STATUS - TACOM_BUSY_NORMAL");

//	pthread_mutex_lock(&tm->sync_mutex);
//	pthread_cond_signal(&tm->sync_cond);
//	pthread_mutex_unlock(&tm->sync_mutex);
	taco_run();

	return tacom_get_status();
}

int taco_set_info(int type, char *info)
{
	int ret;
	//enum tacom_stat status;

	pthread_mutex_lock(&cmd_mutex);
	ret = tacom_set_info(type, info);
	pthread_mutex_unlock(&cmd_mutex);
	if (ret < 0) {
		return -2;
	}
	
	return ret;
}

char * taco_get_info(int code)
{
	char *info;
	enum tacom_stat status;

	/* IDLE ?�태 체크 */
	status = tacom_get_status();
	if (status != TACOM_IDLE)
		return NULL;
	
	/* ?�패??"NOK" 리턴 */
	info = tacom_get_info(code);
	
	return strdup(info);
}


int taco_command(int command, int config, int buf_size)
{
	DTG_LOGI("RECV REQUEST!! %s : %d", __func__, command);
	int ret, result;
	//int retry = 0;
	//FILE *fptr = NULL;
	//char buf[1024];
	tacoc_stream_t strm;

	TACOM *tm = tacom_get_cur_context();

	if(tm == NULL) {
		printf("%s:%s> tm is NULL, yet net initialize??\n", __FILE__, __func__);
		return -1;
	}

		

	/* Summary Transfer Process */
	if (tm->tm_setup->conf_flag & (0x1 << SUMMARY_ENABLE_BIT)) { 
		int r_num = tacom_read_summary();
		if (r_num < 0) {
			DTG_LOGE("READ SUMMARY ERROR!!");
		} else {
			strm.type = 0;	/* 0 : summary, 1 : normal data */
			strm.size = r_num;
			strm.data = tm->tm_strm.stream;

			DTG_LOGI("READED SUMMARY : %d bytes, SENDING TO TACOC", r_num);
			ret = tacoc_taco_cb_data_call_wrapper(&strm, &result); 
			{
				if (tacom_ack_summary() < 0) {
					DTG_LOGE("SUMMARY ACK FAILURE");
				} else {
					tm->tm_setup->conf_flag &= ~(0x1 << SUMMARY_ENABLE_BIT);
					DTG_LOGI("SUMMARY ACK SUCCESS");
				}
			}
		}
	}

	if (tacom_get_status() != TACOM_IDLE) {
		DTG_LOGE("taco status is not TACOM_IDLE at %s. [%d]", __func__, tacom_get_status(tm));
		return -1;
	}

	tacom_set_status(TACOM_BUSY_NORMAL);
	DTG_LOGI("CHANGE STATUS - TACOM_BUSY_NORMAL");
	switch (command) {
		case CURRENT_DATA:
			ret = tacom_read_current();
			if (ret < 0) {
				result = ret;
				break;
			}
			strm.type = config;
			strm.size = ret;
			strm.data = tm->tm_strm.stream;
			
			ret = tacoc_taco_cb_data_call_wrapper(&strm, &result); 
			{
//				g_rpc_error_count = 0;
			}
			break;
		case REMAIN_DATA:
			result = tacom_unreaded_records_num();
			break;
		case ACCUMAL_MDT_DATA:
		case ACCUMAL_DATA:
			{
				tacom_data_build_period = (config & 0x0fffffff);	// FIXME
				tacom_data_type = 1;				// FIXME

				if(config >= 0x10000000) //file save request
					ret = tacom_read_records(config);
				//else if(config == 0x20000000) //abort test
				//	ret = tacom_read_records(tm, config);
				else
					ret = tacom_read_records(config & 0x0fffffff);

				if (ret < 0){
					result = ret;
					break;
				} else if (ret > 0){
					if(command == ACCUMAL_MDT_DATA)
						strm.type = 101; //fixed value
					else
						strm.type = 1; //fixed value

					strm.size = ret;
					strm.data = tm->tm_strm.stream;
				} else {
					if(command == ACCUMAL_MDT_DATA)
						tacom_ack_records(2);
					else
						tacom_ack_records(0);
					result = -1;
					break;
				}
				/*
				if (config & ~(0xefffffff)) { //when sig off, all data reading.
					do {
						fptr = fopen("/var/stored_records", "a" );
						sleep(1);
					} while (fptr == NULL);
					if (fptr != NULL) {
						fwrite(&strm.size, 1, 4, fptr);
						fwrite(strm.data, 1, strm.size, fptr);
						fflush(fptr);
						sync();
						fclose(fptr); fptr = NULL;
						sleep(10); //jwrho 2015.01.17
					}
					result = 1;
					break;
				}
				*/
				if (config & ~(0xefffffff)) { //when sig off, all data reading.
					break; //dont network transfer.
				}

				ret = tacoc_taco_cb_data_call_wrapper(&strm, &result); 
				{
//					g_rpc_error_count = 0;
				}
				break;
			}
		case CLEAR_DATA:
			result = tacom_ack_records(config);
			break;
		default:
			DTG_LOGE("unsupported command.");
			result = -1;
			break;
	}
	tacom_set_status(TACOM_IDLE);
	DTG_LOGI("CHANGE STATUS - TACOM_IDLE");
	return result;
}

char *taco_set_factor(char *command)
{
	char resp[128] = {0, };
	int retry = 0;
	//enum tacom_stat status;
	while (tacom_get_status() != TACOM_IDLE) {
		if (retry == 70)
			return NULL;
		sleep(3);
		retry++;
		fprintf(stderr, "%s>  tacom busy status..count:[%d]\n", __FILE__, retry);
	}

	tacom_set_status(TACOM_BUSY_NORMAL);
	DTG_LOGI("CHANGE STATUS - TACOM_BUSY_NORMAL");

	pthread_mutex_lock(&cmd_mutex);
	custom_command(command, resp, sizeof(resp));
	pthread_mutex_unlock(&cmd_mutex);

	tacom_set_status(TACOM_IDLE);
	DTG_LOGI("CHANGE STATUS - TACOM_IDLE resp: %s", resp);
	return resp;
}

