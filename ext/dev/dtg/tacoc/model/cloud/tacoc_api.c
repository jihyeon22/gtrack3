<<<<<<< HEAD
/**
* @file tacoc_api.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-21
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "dtg_packet.h"
#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "dtg_utill.h"
#include "dtg_debug.h"

static char *last_stream = NULL;
static int last_stream_size = 0;

int tx_data_to_tacoc(int type, char *stream, int len)
{

	int retry_count = 0;
	int max_count = 3;
	int net_ret;

	set_working_action(1);

	DEBUG_INFO("Tacoc Data Reception Success\n");

	if (type == 0) {
		/* Summary */
		send_to_summary_server(stream, len);
		set_working_action(0);
		return 0;
	} else if (type == 1) {
		while(retry_count < max_count) {
			net_ret = send_to_server(stream, len);
			if(net_ret == NET_SUCCESS_OK || net_ret == RETURN_OK) {
				set_working_action(0);
				if (last_stream != NULL)
					free(last_stream);
				last_stream = (char *)malloc(len);
				memset(last_stream, 0, len);
				memcpy(last_stream, stream, len);
				last_stream_size = len;
				alarm(get_interval());
				return 0;
			} else {
				retry_count++;
			}
		}
	}
	set_working_action(0);
	alarm(get_interval());
	return -1;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
	common_sms_parser(smsdata, sender);
	return 1;
}

int breakdown_report(int integer)
{
	if(integer == 0) {
		printf("breakdw msg: dtg problem occur\n");
	} else if(integer == 1) {	
		printf("breakdw msg: dtg working\n");
	} else {
		printf("breakdw msg: incorrect code\n");
	}
	return 0;
}

void mdmc_power_off()
{
	if((last_stream != NULL) && (last_stream[0] == '[')) {
		last_stream[76] = '0';
		last_stream[77] = '4';
		tx_data_to_tacoc(1, last_stream, last_stream_size);
	}
}
=======
/**
* @file tacoc_api.c
* @brief 
* @author Jinwook Hong
* @version 
* @date 2013-11-21
*/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "dtg_packet.h"
#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "dtg_utill.h"
#include "dtg_debug.h"

static char *last_stream = NULL;
static int last_stream_size = 0;

int tx_data_to_tacoc(int type, char *stream, int len)
{

	int retry_count = 0;
	int max_count = 3;
	int net_ret;

	set_working_action(1);

	DEBUG_INFO("Tacoc Data Reception Success\n");

	if (type == 0) {
		/* Summary */
		send_to_summary_server(stream, len);
		set_working_action(0);
		return 0;
	} else if (type == 1) {
		while(retry_count < max_count) {
			net_ret = send_to_server(stream, len);
			if(net_ret == NET_SUCCESS_OK || net_ret == RETURN_OK) {
				set_working_action(0);
				if (last_stream != NULL)
					free(last_stream);
				last_stream = (char *)malloc(len);
				memset(last_stream, 0, len);
				memcpy(last_stream, stream, len);
				last_stream_size = len;
				alarm(get_interval());
				return 0;
			} else {
				retry_count++;
			}
		}
	}
	set_working_action(0);
	alarm(get_interval());
	return -1;
}

int tx_sms_to_tacoc(char *sender, char* smsdata)
{
	common_sms_parser(smsdata, sender);
	return 1;
}

int breakdown_report(int integer)
{
	if(integer == 0) {
		printf("breakdw msg: dtg problem occur\n");
	} else if(integer == 1) {	
		printf("breakdw msg: dtg working\n");
	} else {
		printf("breakdw msg: incorrect code\n");
	}
	return 0;
}

void mdmc_power_off()
{
	if((last_stream != NULL) && (last_stream[0] == '[')) {
		last_stream[76] = '0';
		last_stream[77] = '4';
		tx_data_to_tacoc(1, last_stream, last_stream_size);
	}
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
