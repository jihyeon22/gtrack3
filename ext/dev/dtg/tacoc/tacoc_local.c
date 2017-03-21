/**
 *       @file  tacoc_local.c
 *      @brief  toco control service local procedure
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

#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <pthread.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include "tacoc_local.h"


void *tacoc_sms_thread(char *string)
{

	char *token = ",";
	char *endoftoken = "\r\n";
	char *tr;
	char sender[32] = { 0 };
	
	char *message = strdup(string);
	char *content = message;

	printf("%s:%d> msg [%s]\n", __func__, __LINE__, content);

#if defined(BOARD_TL500K)	
	//char msg[128];
	char *temp_bp = NULL;
	char token_1[ ] = ",";
	char token_2[ ] = "\r";
#endif
	pthread_detach(pthread_self());

#if defined(BOARD_TL500K)	
	if(!strcmp("KT_DEV_FOTA_SVC_FOTA_REQ_PUSH", message))//
	{
		tx_sms_to_tacoc(NULL, message);
		free(message);
		return NULL;
	}
	else if(!strcmp("KT_DEV_FOTA_SVC_DEVICE_RESET", message))
	{
		tx_sms_to_tacoc(NULL, message);
		free(message);
		return NULL;
	}
	else if(!strcmp("KT_DEV_FOTA_SVC_MODEM_INDIRECT_RESET", message))
	{
		tx_sms_to_tacoc(NULL, message);
		free(message);
		return NULL;
	}
	else if(!strcmp("KT_DEV_FOTA_SVC_DEVICE_QTY", message))
	{
		tx_sms_to_tacoc(NULL, message);
		free(message);
		return NULL;
	}
	else if(!strcmp("KT_DEV_FOTA_SVC_MODEM_QTY", message))
	{
		tx_sms_to_tacoc(NULL, message);
		free(message);
		return NULL;
	}
#endif

#if defined(BOARD_NEO_W100)	|| defined(BOARD_TL500S)
	tr = strtok(content, token);		// NEWMT:DATE
	tr = strtok(NULL, token);		// sender phone number
	if (tr != NULL)
		strcpy(sender, tr);
	tr = strtok(NULL, token);		// TID
	tr = strtok(NULL, endoftoken);	// SMS Message

	if (tr != NULL) {
		/* Call Model SMS Produce */
		tx_sms_to_tacoc(sender, tr);
	}
#elif defined(BOARD_TL500K)
	memset(sender, 0x00, sizeof(sender));
	//memset(msg, 0x00, sizeof(msg));

	tr = strtok_r(content, token_1, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_1, &temp_bp);
	if(tr == NULL) return -1;
	memcpy(sender, &tr[1], strlen(tr)-2);

	tr = strtok_r(NULL, token_2, &temp_bp);
	if(tr == NULL) return -1;
	tr = strtok_r(NULL, token_2, &temp_bp);
	if(tr == NULL) return -1;
	//memcpy(msg, &tr[1], strlen(tr)-1);
	
	if (tr != NULL) {
		/* Call Model SMS Produce */
		tx_sms_to_tacoc(sender, &tr[1]);
	}
#endif

printf("tacoc_sms_thread --\n");
	free(message);
}

int power_proc_flags = 0;
void tacoc_mdmc_power_off()
{
	if(power_proc_flags == 1)
		return;

	DTG_LOGW("////////////////////////////////////////");
	DTG_LOGW("////////////////////////////////////////");
	DTG_LOGW("/////////////POWER OFF//////////////////");
	DTG_LOGW("////////////////////////////////////////");
	DTG_LOGW("////////////////////////////////////////");

	mdmc_power_off_thread_wrapper(NULL);

	power_proc_flags = 1;
}

int tacoc_breakdown_report(int* integer)
{
	int result;
	result = breakdown_report(*integer);
	return result;
	
}

int tacoc_taco_cb_data(tacoc_stream_t *strm)
{
	int result = 0;

	result = tx_data_to_tacoc(strm->type, strm->data, strm->size);
	DTG_LOGD("%s: %s >> ret:%d", __FILE__, __func__, result);
	return result;
}
