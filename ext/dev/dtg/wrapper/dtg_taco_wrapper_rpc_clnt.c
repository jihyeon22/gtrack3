<<<<<<< HEAD
/**
 *       @file  taco_clnt.c
 *      @brief  taco service client program stub
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

#include <memory.h>
#include <errno.h>
#include <wrapper/dtg_taco_wrapper_rpc_clnt.h>
#include <tacom/tacom_inc.h>

// warnnning fix
int taco_request(void);
int taco_set_info(int type, char *info);
int taco_command(int command, int config, int buf_size);

int taco_request_call_wrapper()
{
    // taco (taco_request) -> tacoc (
    return taco_request();
}

int taco_set_info_call_wrapper(tacom_info_t *info)
{
    // code : TACO_SET_INFO
    return taco_set_info(info->code, info->data);
}

/*
enum clnt_stat
taco_get_info_call(int *argp, char **clnt_res, CLIENT *clnt)
{
	enum clnt_stat ret;

	pthread_mutex_lock(&tacoc_rpc_cmd_mutex);
	ret = clnt_call(clnt, TACO_GET_INFO,
		(xdrproc_t) xdr_int, (caddr_t) argp,
		(xdrproc_t) xdr_wrapstring, (caddr_t) clnt_res,
		TIMEOUT);

	pthread_mutex_unlock(&tacoc_rpc_cmd_mutex);
	return ret;
}
*/

int taco_command_call_wrapper(int command, int config, int buf_size)
{
	// code : TACO_COMMAND
	taco_command(command, config, buf_size);
	return 1;
}

/*
enum clnt_stat
taco_set_factor_call(char **argp, char **clnt_res, CLIENT *clnt)
{
	struct timeval tout = { 300, 0 };
	enum clnt_stat ret;

	pthread_mutex_lock(&tacoc_rpc_cmd_mutex);
	fprintf(stderr, "taco_set_factor_call ++\n");

	ret = clnt_call(clnt, TACO_SET_FACTOR,
		(xdrproc_t) xdr_wrapstring, (caddr_t) argp,
		(xdrproc_t) xdr_wrapstring, (caddr_t) clnt_res,
		tout);

	fprintf(stderr, "taco_set_factor_call --\n");
	pthread_mutex_unlock(&tacoc_rpc_cmd_mutex);
	return ret;
}
*/
=======
/**
 *       @file  taco_clnt.c
 *      @brief  taco service client program stub
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

#include <memory.h>
#include <errno.h>
#include <wrapper/dtg_taco_wrapper_rpc_clnt.h>
#include <tacom/tacom_inc.h>

// warnnning fix
int taco_request(void);
int taco_set_info(int type, char *info);
int taco_command(int command, int config, int buf_size);

int taco_request_call_wrapper()
{
    // taco (taco_request) -> tacoc (
    return taco_request();
}

int taco_set_info_call_wrapper(tacom_info_t *info)
{
    // code : TACO_SET_INFO
    return taco_set_info(info->code, info->data);
}

/*
enum clnt_stat
taco_get_info_call(int *argp, char **clnt_res, CLIENT *clnt)
{
	enum clnt_stat ret;

	pthread_mutex_lock(&tacoc_rpc_cmd_mutex);
	ret = clnt_call(clnt, TACO_GET_INFO,
		(xdrproc_t) xdr_int, (caddr_t) argp,
		(xdrproc_t) xdr_wrapstring, (caddr_t) clnt_res,
		TIMEOUT);

	pthread_mutex_unlock(&tacoc_rpc_cmd_mutex);
	return ret;
}
*/

int taco_command_call_wrapper(int command, int config, int buf_size)
{
	// code : TACO_COMMAND
	taco_command(command, config, buf_size);
	return 1;
}

/*
enum clnt_stat
taco_set_factor_call(char **argp, char **clnt_res, CLIENT *clnt)
{
	struct timeval tout = { 300, 0 };
	enum clnt_stat ret;

	pthread_mutex_lock(&tacoc_rpc_cmd_mutex);
	fprintf(stderr, "taco_set_factor_call ++\n");

	ret = clnt_call(clnt, TACO_SET_FACTOR,
		(xdrproc_t) xdr_wrapstring, (caddr_t) argp,
		(xdrproc_t) xdr_wrapstring, (caddr_t) clnt_res,
		tout);

	fprintf(stderr, "taco_set_factor_call --\n");
	pthread_mutex_unlock(&tacoc_rpc_cmd_mutex);
	return ret;
}
*/
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
