
/**
 *       @file  tacoc_clnt.c
 *      @brief  taco control service client program stub
 *
 * Detailed description starts here.
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013년 02월 27일
 *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technologt, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 * 20130612		yoonki		RPC Call 시 clnt NULL인지 검사 추가
 */

#include <memory.h>
#include <errno.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>

#include <tacoc/tacoc_local.h>

int tx_data_to_tacoc(int type, char *stream, int len);

int tacoc_sms_cb_data_call_wrapper(char **string, int *clnt_res)
{
	#if 0 // TODO: api fix
	if (!clnt) {
		errno = ECONNREFUSED;		
		return RPC_SYSTEMERROR;
	}

	return clnt_call(clnt, TACOC_SMS_CB_DATA,
		(xdrproc_t) xdr_wrapstring, (caddr_t) string,
		(xdrproc_t) xdr_void, (caddr_t) NULL,
		TIMEOUT);
	#else
	#endif
	return 1;
}

int tacoc_mdmc_power_off_call_wrapper()
{
	#if 0 // TODO: api fix
	if (!clnt) {
		errno = ECONNREFUSED;		
		return RPC_SYSTEMERROR;
	}

	return clnt_call(clnt, TACOC_MDMC_POWER_OFF,
		(xdrproc_t) xdr_void, (caddr_t) NULL,
		(xdrproc_t) xdr_void, (caddr_t) NULL,
		TIMEOUT);
	#else
	//tacoc_mdmc_power_off();
	#endif
	return 1;
}

int tacoc_breakdown_report_call_wrapper(int *integer, int *clnt_res)
{
	*clnt_res = tacoc_breakdown_report(integer);
	return 1;
}


/* Callback Call */
int tacoc_taco_cb_data_call_wrapper(tacoc_stream_t *frame, int *clnt_res)
{
	*clnt_res = tx_data_to_tacoc(frame->type, frame->data, frame->size);
	return 1;
}



