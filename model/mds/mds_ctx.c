#include<sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "packet.h"
#include <base/gpstool.h>
#include "mds_ctx.h"

static RUN_CONTEXT_T run_ctx;

int set_ctx_network(int status)
{
	run_ctx.network_stat = status;
	return PACKET_RET_SUCCESS;
}

int get_ctx_network(void)
{
	return run_ctx.network_stat;
}

int set_ctx_power(int status)
{
	run_ctx.last_power_status = run_ctx.power_status;
	run_ctx.power_status = status;
	return PACKET_RET_SUCCESS;
}

int get_ctx_power(void)
{
	return run_ctx.power_status;
}

int get_ctx_power_is_changed(void)
{
	if (run_ctx.last_power_status != run_ctx.power_status)
	{
		run_ctx.last_power_status = run_ctx.power_status;
		return PACKET_RET_SUCCESS;
	}
	else
		return PACKET_RET_FAIL;
}


int set_ctx_deivce_phone_num(char* phone_num, int size)
{
	// argument TODO Error 처리
	strncpy(run_ctx.device_phone_num, phone_num, size);
	return PACKET_RET_SUCCESS;
}


int set_ctx_keyon_time(int year, int month, int day, int hour, int min, int sec)
{
	run_ctx.keyon_time.time_YY = (year - 1900) % 100;
	run_ctx.keyon_time.time_MM = month ;
	run_ctx.keyon_time.time_DD = day;
	run_ctx.keyon_time.time_hour = hour;
	run_ctx.keyon_time.time_min = min;
	run_ctx.keyon_time.time_sec = sec;
	run_ctx.keyon_time.time_mil_sec = 0;
	
	return PACKET_RET_SUCCESS ;
}

int set_ctx_keyoff_time(int year, int month, int day, int hour, int min, int sec)
{
	run_ctx.keyoff_time.time_YY = (year - 1900) % 100;
	run_ctx.keyoff_time.time_MM = month ;
	run_ctx.keyoff_time.time_DD = day;
	run_ctx.keyoff_time.time_hour = hour;
	run_ctx.keyoff_time.time_min = min;
	run_ctx.keyoff_time.time_sec = sec;
	run_ctx.keyoff_time.time_mil_sec = 0;
	
	return PACKET_RET_SUCCESS;
}

int set_ctx_keyon_gather_data_interval(int sec)
{
	run_ctx.keyon_gather_data_interval_sec = sec;
	return PACKET_RET_SUCCESS;
}


int get_ctx_keyon_gather_data_interval(void)
{
	return run_ctx.keyon_gather_data_interval_sec;	
}

int set_ctx_keyon_send_to_data_interval(int sec)
{
	run_ctx.keyon_send_to_data_interval_sec = sec;
	return PACKET_RET_SUCCESS;
}

int get_ctx_keyon_send_to_data_interval(void)
{
	return run_ctx.keyon_send_to_data_interval_sec;
	
}

int set_ctx_keyoff_gather_data_interval(int sec)
{
	run_ctx.keyoff_gather_data_interval_sec = sec;
	return PACKET_RET_SUCCESS;
}


int get_ctx_keyoff_gather_data_interval(void)
{
	return run_ctx.keyoff_gather_data_interval_sec;
}


int set_ctx_keyoff_send_to_data_interval(int sec)
{
	run_ctx.keyoff_send_to_data_interval_sec = sec;
	return PACKET_RET_SUCCESS;
}

int get_ctx_keyoff_send_to_data_interval(void)
{
	return run_ctx.keyoff_send_to_data_interval_sec;
}


int set_ctx_server_stat(int status)
{
	run_ctx.server_stat = status;
	return PACKET_RET_SUCCESS;
}

int get_ctx_server_stat(void)
{
	return run_ctx.server_stat;
}

// server sleep........ info..

int decrease_ctx_server_sleep(void)
{
	if (run_ctx.server_sleep_time > 0 )
		run_ctx.server_sleep_time--;
		
	return run_ctx.server_sleep_time;
}

int set_ctx_server_sleep(int sec)
{
	run_ctx.server_sleep_time = sec;
	return run_ctx.server_sleep_time;
}

int get_ctx_server_sleep(void)
{
	return run_ctx.server_sleep_time;
}

