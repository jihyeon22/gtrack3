<<<<<<< HEAD
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

#include <wrapper/dtg_atcmd.h>
#include <errno.h>
/* Insert Common Client Header */
#include <wrapper/dtg_log.h>
// #include "taco_rpc.h"
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>


void device_reset()
{
	return mdmc_api_reset_wrapper();
}

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

int data_req_to_taco_cmd(int command, int period, int size, int action_state)
{
	int ret;
	while(is_working_action())
		sleep(2);

	set_working_action(action_state);
	taco_command_call_wrapper(command, period, size);

	set_working_action(0);
	sleep(3);
	return result;
}

int reboot(int delay)
{
	return mdmc_api_reset_wrapper();
}

=======
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

#include <wrapper/dtg_atcmd.h>
#include <errno.h>
/* Insert Common Client Header */
#include <wrapper/dtg_log.h>
// #include "taco_rpc.h"
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>


void device_reset()
{
	return mdmc_api_reset_wrapper();
}

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

int data_req_to_taco_cmd(int command, int period, int size, int action_state)
{
	int ret;
	while(is_working_action())
		sleep(2);

	set_working_action(action_state);
	taco_command_call_wrapper(command, period, size);

	set_working_action(0);
	sleep(3);
	return result;
}

int reboot(int delay)
{
	return mdmc_api_reset_wrapper();
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
