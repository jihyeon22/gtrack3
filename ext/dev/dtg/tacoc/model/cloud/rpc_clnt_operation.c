/*
 * rpc_clnt_operation.c
 *
 *  Created on: 2013. 3. 18.
 *      Author: ongten
 */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <rpc/pmap_clnt.h>

#include "wrapper/dtg_log.h"
//#include <wrapper/dtg_mdmc_rpc.h>
/* Insert Common Client Header */
#include "taco_rpc.h"
#include <wrapper/dtg_mdmc_rpc.h>

#include "dtg_debug.h"

extern CLIENT *clnt_mdmc;
extern CLIENT *clnt_taco;

int network_connect()
{
	return mdmc_api_net_conn_wrapper();
}

int net_time_sync()
{
	return mdmc_api_time_sync_wrapper();
}

int data_req_to_taco()
{
	return taco_request_call_wrapper();
}

int reboot(int delay)
{
	return mdmc_api_reset_wrapper();
}

