#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


#include <wrapper/dtg_log.h>
#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>
#include "tacoc_local.h"
#include "dtg_type.h"

void tacoc_mdmc_power_off();
void mdmc_power_off_thread_wrapper(void * data)
{
	DTG_LOGW("////////////////////////////////////////");
	DTG_LOGW("/////////////POWER OFF THREAD///////////");
	DTG_LOGW("////////////////////////////////////////");

	/* Model Power Off Routine */
	mdmc_power_off();

	/* Unlock Reset */
	// reset code...
}

