/**
 *       @file  taco.c
 *      @brief  taco deamon
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



/*******************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <execinfo.h>

//#include <tacom/taco_rpc_wrapper.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_atcmd.h>
#include <tacom/tacom_internal.h>
#include <wrapper/dtg_log.h>


static int g_taco_thread_check_flag = 0;
extern int tacom_data_type;
extern int tacom_data_build_period;

#define BURST_MODE		1
#define NORMAL_MODE		0

void taco_run(void)
{
	DTG_LOGD("RUN");

	int ret, result;
	int r_num;
	int processing_mode = NORMAL_MODE;  // 1   : BURST??송 0 : NORMAL??송
	int	breakdown = 1; 					// 1   : ??상??작  0 : ??상??작
	int ack_breakdown = 0; 				// 0 > : ??상??작  1 : ??상??작
	int ack_breakdown_flag = 1;
	int retry_cnt = 0;
	tacom_std_hdr_t hdr;
	tacoc_stream_t strm;
	char *phonenum;

	TACOM *tm = tacom_get_cur_context();
	g_taco_thread_check_flag = 1;

	/* Phone Number check */
	while (1) {
		if ((phonenum = atcmd_get_phonenum()) == NULL) {
			DTG_LOGE("TACO GET PHONE NUMBER FAILURE");
			sleep(1);
		}
		else {
			DTG_LOGD("TACO GET PHONE NUMBER : %s", phonenum);
			break;
		}
	}

	/* TACOM STANDARD HEADER ??어??*/
	ret = tacom_get_info_hdr(&hdr);
	if (ret < 0) {
		/* failure */
	}

	// while (1) 
	{
		retry_cnt = 0;
		DTG_LOGI("NORMAL MODE - WAITING REQUEST !!");
		/* Request 기다???*/
		//pthread_mutex_lock(&tm->sync_mutex);
		//pthread_cond_wait(&tm->sync_cond, &tm->sync_mutex);
		//pthread_mutex_unlock(&tm->sync_mutex);

		/* Summary Transfer Process */
		if (tm->tm_setup->conf_flag & (0x1 << SUMMARY_ENABLE_BIT)) { 
			r_num = tacom_read_summary();
			if (r_num < 0) {
				DTG_LOGE("READ SUMMARY ERROR!!");
			} else {
				strm.type = 0;	/* 0 : summary, 1 : normal data */
				strm.size = r_num;
				strm.data = tm->tm_strm.stream;

				DTG_LOGI("READED SUMMARY : %d bytes, SENDING TO TACOC", r_num);
				
				// TACOC_TACO_CB_DATA
				ret = tacoc_taco_cb_data_call_wrapper(&strm, &result); 

				if (tacom_ack_summary() < 0) {
					DTG_LOGE("SUMMARY ACK FAILURE");
				} else {
					tm->tm_setup->conf_flag &= ~(0x1 << SUMMARY_ENABLE_BIT);
					DTG_LOGI("SUMMARY ACK SUCCESS");
				}
			}
		}

		if (tm->tm_setup->conf_flag & (0x1 << READ_CURRENT_BIT)) {
retry_read_current:
			r_num = tacom_read_current(tm);
			if(r_num < 0) {
				DTG_LOGE("READ CURRENT ERROR!!");
				retry_cnt++;
				if (retry_cnt > 5) {
					breakdown = 0; // breakdown status
				} else {
					goto retry_read_current;
				}
			} else {
				retry_cnt = 0;

				strm.type = 1;	/* 0 : summary, 1 : normal data */
				strm.size = r_num;
				strm.data = tm->tm_strm.stream;

				DTG_LOGI("READED CURRENT DATA : %d bytes, SENDING TO TACOC", r_num);
				
				// TACOC_TACO_CB_DATA
				ret = tacoc_taco_cb_data_call_wrapper(&strm, &result); // 
				DTG_LOGI("READ CURRENT SUCCESS");
				
			}
		} 

		if (breakdown &&
			(tm->tm_setup->conf_flag & (0x1 << STANDARD_FLOW_BIT))) {

			/* Tacoc로????Request ??신 */
			/** 
			 * Taco burst xfer mode : Request ??이 바로바로 ??송 
			 */
			
			do {
retry_unreaded_records:
				DTG_LOGI("READING UNREADED RECORDS");
				r_num = tacom_unreaded_records_num();
				if (r_num < 0) {
					/* unreaded records number failure */
					DTG_LOGE("UNREAD RECORDS ERROR!! RETRY %d", retry_cnt);
					retry_cnt++;
					if (retry_cnt > 5) {
						breakdown = 0; // breakdown status
						break;
					}
					goto retry_unreaded_records;
				} else if(r_num == 0) {
					sleep(15);
					goto retry_unreaded_records;
				}
				retry_cnt = 0;
				
				DTG_LOGD("UNREADED RECORDS : %d", r_num);
#if 1
				processing_mode = NORMAL_MODE;
				DTG_LOGD("NORMAL MODE");
				if (r_num < 10)
					continue;
#else
				if (r_num < 180) {
					/* ??음 주기????송 */
					processing_mode = NORMAL_MODE;
					DTG_LOGD("NORMAL MODE");
				} else {
					processing_mode = BURST_MODE;
					DTG_LOGD("BURST MODE");
				}
#endif
retry_read_records:
				DTG_LOGD("READING RECORDS");
				r_num = tacom_read_records(0);
				if (r_num < 0) {
					/* failure */
					DTG_LOGE("READ RECORDS ERROR!! RETRY %d", retry_cnt + 1);
					retry_cnt++;
					
					if (retry_cnt > 5) {
						breakdown = 0; // breakdown status
						break;
					}
					goto retry_read_records;
				}
				retry_cnt = 0;

				if (r_num > 0) {
					strm.type = tacom_data_type;
					strm.size = r_num;
					strm.data = tm->tm_strm.stream;
					
					// TACOC_TACO_CB_DATA
					ret = tacoc_taco_cb_data_call_wrapper(&strm, &result); // TODO: api fix
				} else {
					result = 0;
				}

				if (result < 0) {
					/* send frame to server failure */
					DTG_LOGE("SERVER FAILURE!!");
					break;
				} else if (result == 100) {
					DTG_LOGI("Skip ACK-RQ Commmand : 100");
					processing_mode = NORMAL_MODE;
				} else if (result == 101) {
					DTG_LOGI("Skip ACK-RQ Commmand : 101");
					processing_mode = NORMAL_MODE;
				} else {
					if (tacom_ack_records(r_num) < 0) {
						/* ack failure */
						/** ack 명령??는 ??패????수?????? ??고
						  * 바로 ??패처리??다 */
						ack_breakdown++;	// ack breakdown status
						DTG_LOGE("ACK FAILURE!!");
						break;
					}
					DTG_LOGI("ACK SUCCESS");
				}

				/* ACK 고장 ??태 복구 */
				if (ack_breakdown) {
					if (processing_mode == NORMAL_MODE)
						tacom_set_status( TACOM_BUSY_NORMAL);
					else 
						tacom_set_status( TACOM_BUSY_BURST);
					
					/* ACK 고장 복구 ??공 */
					ack_breakdown = 0;	// normal
					ack_breakdown_flag = 1;	// ack normal status;
					tacoc_breakdown_report_call_wrapper(&ack_breakdown_flag,
								&result);
					DTG_LOGW("ACK BREAKDOWN RECOVERY SUCCESS");
				}
			} while (processing_mode == BURST_MODE);

			/* ACK BREAKDOWN */
			if (ack_breakdown) {
				/* ACK ??패??는 ??청????어????복구??야 ??서 IDLE ??태???만든??*/
				if (ack_breakdown == 1) {
					ack_breakdown_flag = 0; // ack breakdown status;
					tacoc_breakdown_report_call_wrapper(&ack_breakdown_flag, &result);
				}
				
				tacom_set_status( TACOM_IDLE);
				DTG_LOGW("CHANGE STATUS - TACOM_IDLE - CUZ ACK BREAKDOWN");
			}
		}

		if (!breakdown) {
			/* BREAKDOWN STATUS */
			tacom_set_status( TACOM_BREAKDOWN);
retry_breakdown_status:
			// TACOC_BREAKDOWN_REPORT
			ret = tacoc_breakdown_report_call_wrapper(&breakdown, &result);
			
			DTG_LOGW("CHANGE STATUS - TACOM_BREAKDOWN");

			/* recovery process */
			/** 
			 * check loop (read unreaded records -> read records)
			 **/
			while (1) {
				sleep(10);
				DTG_LOGD("AUTO RECOVERY");
				r_num = tacom_unreaded_records_num();
				if (r_num < 0) {
					DTG_LOGD("AUTO RECOVERY - UNREAD RECORD ERROR!!");
					continue;
				}
				
				sleep(10);
				r_num = tacom_read_records(0);
				if (r_num < 0) {
					DTG_LOGD("AUTO RECOVERY - READ RECORD ERROR!!");
					continue;
				}

				DTG_LOGD("AUTO RECOVERY SUCCESS");
				breakdown = 1; // normal status
retry_breakdown_success:
				// TACOC_BREAKDOWN_REPORT
				ret = tacoc_breakdown_report_call_wrapper(&breakdown, &result); // TODO: api fix

				tacom_set_status( TACOM_IDLE);
				break;
			}
		} else {
			/* NORMAL STATUS */
			tacom_set_status( TACOM_IDLE);
			DTG_LOGI("CHANGE STATUS - TACOM_IDLE");
		}
	}
}

