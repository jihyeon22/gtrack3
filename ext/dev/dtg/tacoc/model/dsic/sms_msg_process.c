/*
 * sms_msg_process.c
 *
 *  Created on: 2013. 3. 19.
 *      Author: gbuddha
 */

#include "stdio.h"
#include "stdlib.h"
#include "dtg_debug.h"

#include "dtg_ini_utill.h"
#include "dtg_data_manage.h"

int sms_set_ctrl_svrip_info(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char ip[64] = {0,};
	char t_port[50] = {0,};
	int port = -1;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(ip,psms);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(t_port,psms);
	port = strtol(t_port, NULL, 10);

	DTG_LOGD("%s %s\n", ip, t_port);

	set_ctrl_server_ip_addr(ip);
	set_ctrl_server_port(port);

	save_ini_ctrl_ip_setting_info();
	return 0;
}

int sms_set_dtg_svrip_info(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char ip[64] = {0,};
	char t_port[50] = {0,};
	int port = -1;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(ip,psms);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(t_port,psms);
	port = strtol(t_port, NULL, 10);

	DTG_LOGD("%s %s\n", ip, t_port);

	set_dtg_server_ip_addr(ip);
	set_dtg_server_port(port);

	save_ini_dtg_ip_setting_info();
	return 0;
}

int sms_set_ctrl_period(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_report_period[50] = {0,};
	char t_create_period[50] = {0,};
	int report_period = -1;
	int create_period = -1;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_report_period,psms);
	report_period = strtol(t_report_period, NULL, 10);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(t_create_period,psms);
	create_period = strtol(t_create_period, NULL, 10);

	DTG_LOGD("%s %s\n", t_report_period, t_create_period);

	if(report_period < create_period || report_period%create_period!= 0) {
		return -1;
	} else {
		set_ctrl_report_period(report_period);
		set_ctrl_create_period(create_period);
	}

	save_ini_ctrl_period_info();
	return 0;
}

int sms_set_dtg_period(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_report_period[50] = {0,};
	int report_period = -1;
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(t_report_period,psms);
	report_period = strtol(t_report_period, NULL, 10);

	DTG_LOGD("%s\n", t_report_period);

	if(report_period < 60) {
		return -1;
	} else {
		set_dtg_report_period(report_period);
	}

	save_ini_dtg_period_info();
	alarm(get_dtg_report_period());
	return 0;
}

int sms_set_device_reset(char* sms_msg)
{
	DTG_LOGD("sms_set_device_reset\n");

	char token_1[]=",";
	char token_2[]="\0";

	char pwd[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);

	if(strcmp(pwd, "mdt%reset#tngodgofk!") != 0)
		return -1;

	device_reset();
	return 0;
}

