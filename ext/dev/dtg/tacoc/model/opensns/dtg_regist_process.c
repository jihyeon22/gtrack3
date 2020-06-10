
/*
 * dtg_regist_process.c
 *
 *  Created on: 2013. 4. 8.
 *      Author: ongten
 */


#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <util/crc16.h>
#include <wrapper/dtg_atcmd.h>
#include <dm/update.h>

#include <wrapper/dtg_log.h>
#include "dtg_type.h"
#include "dtg_packet.h"
#include "dtg_debug.h"
#include "dtg_regist_process.h"
#include "dtg_data_manage.h"
#include <base/dmmgr.h>

int g_error_report_count = 0;
static char dtg_env[DTG_ENV_SIZE + 1];
static char dtg_stat[DTG_STAT_SIZE + 1];
static char dtg_rl[DTG_RL_SIZE + 1];
static char dtg_vrn[DTG_VRN_SIZE + 1];

char* get_dtg_env(void)
{
	return dtg_env;
}

char* get_dtg_stat(void)
{
	return dtg_stat;
}

char* get_dtg_rl(void)
{
	return dtg_rl;
}

char* get_dtg_vrn(void)
{
	return dtg_vrn;
}

int create_dtg_data(void)
{
	int fd;
	char dtg_status[DTG_FILE_STATUS_SIZE] = {0};
	char *ptr;

	if (getenv(DTG_ENV_VAL))
		strncpy(dtg_env, getenv(DTG_ENV_VAL),2);
	else
		strncpy(dtg_env, "-9",2);

	do {
		fd = open(DTG_FILE_STATUS_PATH, O_RDONLY , 0644);
		sleep(1);
	} while (fd < 0 && errno != ENOENT);
	
	if(fd <= 0)
	{
		return -1;
	}
	
	read(fd, dtg_status, DTG_FILE_STATUS_SIZE);
	close(fd);
	
	ptr = strtok(dtg_status, "/");
	memset(dtg_stat, 0, sizeof(dtg_stat));
	strncpy(dtg_stat, ptr, DTG_STAT_SIZE);
	
	ptr = strtok(NULL, "/");
	memset(dtg_rl, 0, sizeof(dtg_rl));
	strncpy(dtg_rl, ptr, DTG_RL_SIZE);
	
	ptr = strtok(NULL, "/");
	
	if (ptr != NULL){
		memset(dtg_vrn, 0, sizeof(dtg_vrn));
		strncpy(dtg_vrn, ptr, DTG_VRN_SIZE);
	}
	
	return 0;
}

int send_to_reg_server(int type)
{
	return 0;
#if 0
	int err = 0;
	
	switch(type)
	{
		case MSG_TYPE_REGISTRATION:
		{
			err = dmmgr_send(eEVENT_KEY_ON, NULL, 0);
			break;
		}
		case MSG_TYPE_DE_REGISTRATION:
		{
			err = dmmgr_send(eEVENT_KEY_OFF, NULL, 0);			
			break;
		}
		case MSG_TYPE_DIAG_DATA:
		{
			//err = dmmgr_send(eEVENT_STATUS);
			break;
		}
	}

	return err;
#endif
}

void send_device_registration()
{
	g_error_report_count = 0;	
	send_to_reg_server(MSG_TYPE_REGISTRATION);
}

void send_device_de_registration()
{
	send_to_reg_server(MSG_TYPE_DE_REGISTRATION);
}

void check_update()
{
	dmmgr_send(eEVENT_UPDATE, NULL, 0);
}

void send_server_error_report(char *packet_type, int server_resp)
{
/*
	static int last_server_resp = 0;
	char log[1024] = {0, };
	char tmp[128] = {0, };
	char dtg_vrn[DTG_VRN_SIZE] = {0, };
	
	if(last_server_resp == server_resp)
		return;

	last_server_resp = server_resp;

	if(g_error_report_count > 5)
		return;

	g_error_report_count += 1;

	if(get_vrn_info(dtg_vrn, sizeof(dtg_vrn)) < 0)
		strncpy(dtg_vrn, "####00##0000", DTG_VRN_SIZE);

	memset(tmp, 0x00, sizeof(tmp));
	if (!strncmp(dtg_vrn, "####",4)) {
		memcpy(tmp, &dtg_vrn[4], 8);
	} else if (!strncmp(dtg_vrn, "0000",4)) {
		memcpy(tmp, &dtg_vrn[4], 8);
	} else if(!strncmp(dtg_vrn, "전국",4)) {
		memcpy(tmp, &dtg_vrn[4], 8);
	} else	{
		memcpy(tmp, dtg_vrn,12);
	}

	sprintf(log, "packet : %s, ip/port : %s/%d, server_resp %d", packet_type, get_server_ip_addr(), get_server_port(), server_resp);
	DTG_LOGD("warning msg %d: %s", strlen(log), log);
	dmmgr_send(eEVENT_WARNING, log, 0);	
*/
}
