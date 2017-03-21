/**
 *       @file  tacoc.c
 *      @brief  taco control deamon
 *
 * TACOMETER? SERVER媛??곗씠???≪닔??諛?TACOMETER ?쒖뼱
 *
 *     @author  Yoonki (IoT), yoonki@mdstec.com
 *
 *   @internal
 *     Created  2013??02??27?? *    Revision  $Id: doxygen.templates,v 1.3 2010/07/06 09:20:12 mehner Exp $
 *    Compiler  gcc/g++
 *     Company  MDS Technology, R.Korea
 *   Copyright  Copyright (c) 2013, Yoonki
 *
 * This source code is released for free distribution under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * =====================================================================================
 */

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_tacoc_wrapper_rpc_clnt.h>

/* Insert Common Client Header */

#include <wrapper/dtg_mdmc_wrapper_rpc_clnt.h>
#include <wrapper/dtg_version.h>

#include <board/board_system.h>
/*******************************/
#include <stdio.h>
#include <stdlib.h>
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
#include <execinfo.h>

#include <unistd.h>


pthread_mutex_t tacoc_rpc_cmd_mutex; //jwrho
int main_process();
void tacoc_run(void)
{
	DTG_LOGI("RUN");

	while(1)
	{
		//?ㅽ뙣?대룄 ?ъ젒?띿쓣 ?꾪빐 臾댄븳 諛섎났?댁빞??
		if (power_get_power_source() == POWER_SRC_DC) {
			main_process();
		} else {
			DTG_LOGD("tacoc_run main power off status");
			DTG_LOGD("so, main_process can't run");
		}
		sleep(1);
	}
}


int tacoc_main()
{
	tacoc_run();
}
