<<<<<<< HEAD
/*
 * sms_msg_process.c
 *
 *  Created on: 2013. 3. 19.
 *      Author: ongten
 */

#include "stdio.h"
#include "stdlib.h"
#include "dtg_debug.h"

#include "dtg_ini_utill.h"
#include "dtg_data_manage.h"

int sms_set_svrip_info(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char ip[50] = {0,};
	char t_port[50] = {0,};
	int port = -1;
	char pwd[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(ip,psms);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_port,psms);
	port = atoi(t_port);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s %s %s %s\n", pwd, ip, t_port, resp);

	set_server_ip_addr(ip);
	set_server_pwd(pwd);
	set_server_port(port);


	save_ini_ip_setting_info();
	return 0;
}

int sms_set_report_period1(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_trans_period[50] = {0,};
	char t_create_period[50] = {0,};
	char resp[50] = {0,};
	char *psms;
	int trans_period = -1;
	int create_period = -1;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_trans_period,psms);
	trans_period = atoi(t_trans_period);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_create_period,psms);
	create_period = atoi(t_create_period);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s %s %s\n", t_trans_period, t_create_period, resp);

	if(trans_period <= create_period || trans_period%create_period!= 0)
	{
		set_normal_trans_period(trans_period);
		set_normal_create_period(trans_period);
	}
	else
	{
		set_normal_trans_period(trans_period);
		set_normal_create_period(create_period);
	}
	save_ini_report1_info();
	alarm(get_create_period());
	return 0;
}

int sms_set_report_period2(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_ntrans_period[50] = {0,};
	char t_ncreate_period[50] = {0,};
	char t_ptrans_period[50] = {0,};
	char t_pcreate_period[50] = {0,};
	char resp[50] = {0,};
	char *psms;
	int ntrans_period = -1;
	int ncreate_period = -1;
	int ptrans_period = -1;
	int pcreate_period = -1;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_ntrans_period,psms);
	ntrans_period = atoi(t_ntrans_period);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_ncreate_period,psms);
	ncreate_period = atoi(t_ncreate_period);

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_ptrans_period,psms);
	ptrans_period = atoi(t_ptrans_period);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_pcreate_period,psms);
	pcreate_period = atoi(t_pcreate_period);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s %s %s %s %s\n", t_ntrans_period, t_ncreate_period, t_ptrans_period, t_pcreate_period, resp);

	if(ntrans_period <= ncreate_period || ntrans_period%ncreate_period!= 0)
	{
		set_normal_trans_period(ntrans_period);
		set_normal_create_period(ntrans_period);
	}
	else
	{
		set_normal_trans_period(ntrans_period);
		set_normal_create_period(ncreate_period);
	}

	if(ptrans_period <= pcreate_period || ptrans_period%pcreate_period!= 0)
	{
		set_psave_trans_period(ptrans_period);
		set_psave_create_period(ptrans_period);
	}
	else
	{
		set_psave_trans_period(ptrans_period);
		set_psave_create_period(pcreate_period);
	}
	save_ini_report2_info();
	alarm(get_create_period());
	return 0;
}

int sms_set_cumulative_distance(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_cumul_dist[50] = {0,};
	int cumul_dist = -1;
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_cumul_dist,psms);
	cumul_dist=atoi(t_cumul_dist);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s", t_cumul_dist);

	set_cumulative_distance(cumul_dist);
	save_ini_cumulative_distance();

	return 0;
}

int sms_request_device_status(char* sms_msg)
{

	return 0;
}

int sms_set_gpio_mode(char* sms_msg)
{

	char token_1[]=",";
	char token_2[]="\0";

	char gpio_mode[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(gpio_mode,psms);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s", gpio_mode);

	set_gpio_mode(gpio_mode);
	save_ini_gpio_mode();

	return 0;
}

int sms_set_gpio_output(char* sms_msg)
{

	char token_1[]=",";
	char token_2[]="\0";

	char gpio_output[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(gpio_output,psms);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s", gpio_output);

	set_gpio_output(gpio_output);
	save_ini_gpio_output();

	return 0;
}

int sms_set_company_code(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char company_code[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(company_code,psms);

	DTG_LOGD("company_code: %s\n", company_code);

	set_company_code(company_code);
	save_ini_company_code();

	return 0;
}

int sms_device_status_req(char* sms_msg)
{
	//&ms,[sms resp=1, tcp=0],received phone number, 0
	//example> &ms,1,01089619571,0
	DTG_LOGD("sms_device_status_req");

	int serial_mode = get_serial_port_mode(); //UT_DEFAULT_MODE

	char token_1[]=",";
	char token_2[]="\0";

	int c_count = 0;
	char resp[50] = {0,};
	char dest_info[100] = {0,};
	char contents[100] = {0,};
	char *psms;
	char *phonenum;

	DTG_LOGD("sms_device_status_req = [%s]\n", sms_msg);

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(strcmp(resp, "0") == 0)
	{
		DTG_LOGD("resp: %s", resp);
		DTG_LOGD("SEND RESPONSE (TCP)");
		return 0;
	}

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(dest_info,psms);
		
	if(serial_mode == UT_DEFAULT_MODE)
		sprintf(contents, "1,%s,%d,%d,%d,%d,%d,3,0,9999,9999,-5555,0,%s,ver,7E", get_server_ip_addr(), get_server_port(), get_normal_trans_period(),get_normal_create_period(),get_psave_trans_period(),get_psave_create_period(),get_company_code());
	else
		sprintf(contents, "1,%s,%d,%d,%d,%d,%d,3,1,9999,9999,-5555,0,%s,ver,7E", get_server_ip_addr(), get_server_port(), get_normal_trans_period(),get_normal_create_period(),get_psave_trans_period(),get_psave_create_period(),get_company_code());

	int i;
	fprintf(stderr, "\n");
	for (i = 0; i < strlen(contents); i++)
		fprintf(stderr, "%02x ", contents[i]);
	fprintf(stderr, "\n");

	DTG_LOGD("sms llen: %d", strlen(contents));
	// send sms
	atcmd_send_sms(contents, NULL, dest_info);

	return 0;
}

int sms_set_device_reset(char* sms_msg)
{
	DTG_LOGD("sms_set_device_reset\n");

	char token_1[]=",";
	char token_2[]="\0";

	char pwd[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);

	if(strcmp(pwd, "m2m%flemgkwk*2006") != 0)
		return -1;

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}

	device_reset();
	// reset operation

	return 0;
}

int sms_set_geo_fence(char* sms_msg)
{
	DTG_LOGD("sms_set_geo_fence");
	return 0;
}
=======
/*
 * sms_msg_process.c
 *
 *  Created on: 2013. 3. 19.
 *      Author: ongten
 */

#include "stdio.h"
#include "stdlib.h"
#include "dtg_debug.h"

#include "dtg_ini_utill.h"
#include "dtg_data_manage.h"

int sms_set_svrip_info(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char ip[50] = {0,};
	char t_port[50] = {0,};
	int port = -1;
	char pwd[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(ip,psms);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_port,psms);
	port = atoi(t_port);
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s %s %s %s\n", pwd, ip, t_port, resp);

	set_server_ip_addr(ip);
	set_server_pwd(pwd);
	set_server_port(port);


	save_ini_ip_setting_info();
	return 0;
}

int sms_set_report_period1(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_trans_period[50] = {0,};
	char t_create_period[50] = {0,};
	char resp[50] = {0,};
	char *psms;
	int trans_period = -1;
	int create_period = -1;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_trans_period,psms);
	trans_period = atoi(t_trans_period);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_create_period,psms);
	create_period = atoi(t_create_period);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s %s %s\n", t_trans_period, t_create_period, resp);

	if(trans_period <= create_period || trans_period%create_period!= 0)
	{
		set_normal_trans_period(trans_period);
		set_normal_create_period(trans_period);
	}
	else
	{
		set_normal_trans_period(trans_period);
		set_normal_create_period(create_period);
	}
	save_ini_report1_info();
	alarm(get_create_period());
	return 0;
}

int sms_set_report_period2(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_ntrans_period[50] = {0,};
	char t_ncreate_period[50] = {0,};
	char t_ptrans_period[50] = {0,};
	char t_pcreate_period[50] = {0,};
	char resp[50] = {0,};
	char *psms;
	int ntrans_period = -1;
	int ncreate_period = -1;
	int ptrans_period = -1;
	int pcreate_period = -1;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_ntrans_period,psms);
	ntrans_period = atoi(t_ntrans_period);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_ncreate_period,psms);
	ncreate_period = atoi(t_ncreate_period);

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_ptrans_period,psms);
	ptrans_period = atoi(t_ptrans_period);
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_pcreate_period,psms);
	pcreate_period = atoi(t_pcreate_period);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s %s %s %s %s\n", t_ntrans_period, t_ncreate_period, t_ptrans_period, t_pcreate_period, resp);

	if(ntrans_period <= ncreate_period || ntrans_period%ncreate_period!= 0)
	{
		set_normal_trans_period(ntrans_period);
		set_normal_create_period(ntrans_period);
	}
	else
	{
		set_normal_trans_period(ntrans_period);
		set_normal_create_period(ncreate_period);
	}

	if(ptrans_period <= pcreate_period || ptrans_period%pcreate_period!= 0)
	{
		set_psave_trans_period(ptrans_period);
		set_psave_create_period(ptrans_period);
	}
	else
	{
		set_psave_trans_period(ptrans_period);
		set_psave_create_period(pcreate_period);
	}
	save_ini_report2_info();
	alarm(get_create_period());
	return 0;
}

int sms_set_cumulative_distance(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char t_cumul_dist[50] = {0,};
	int cumul_dist = -1;
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(t_cumul_dist,psms);
	cumul_dist=atoi(t_cumul_dist);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s", t_cumul_dist);

	set_cumulative_distance(cumul_dist);
	save_ini_cumulative_distance();

	return 0;
}

int sms_request_device_status(char* sms_msg)
{

	return 0;
}

int sms_set_gpio_mode(char* sms_msg)
{

	char token_1[]=",";
	char token_2[]="\0";

	char gpio_mode[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(gpio_mode,psms);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s", gpio_mode);

	set_gpio_mode(gpio_mode);
	save_ini_gpio_mode();

	return 0;
}

int sms_set_gpio_output(char* sms_msg)
{

	char token_1[]=",";
	char token_2[]="\0";

	char gpio_output[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(gpio_output,psms);

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}
	else if(resp[0] != '0')
	{
		return -1;
	}

	DTG_LOGD("%s", gpio_output);

	set_gpio_output(gpio_output);
	save_ini_gpio_output();

	return 0;
}

int sms_set_company_code(char* sms_msg)
{
	char token_1[]=",";
	char token_2[]="\0";

	char company_code[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(company_code,psms);

	DTG_LOGD("company_code: %s\n", company_code);

	set_company_code(company_code);
	save_ini_company_code();

	return 0;
}

int sms_device_status_req(char* sms_msg)
{
	//&ms,[sms resp=1, tcp=0],received phone number, 0
	//example> &ms,1,01089619571,0
	DTG_LOGD("sms_device_status_req");

	int serial_mode = get_serial_port_mode(); //UT_DEFAULT_MODE

	char token_1[]=",";
	char token_2[]="\0";

	int c_count = 0;
	char resp[50] = {0,};
	char dest_info[100] = {0,};
	char contents[100] = {0,};
	char *psms;
	char *phonenum;

	DTG_LOGD("sms_device_status_req = [%s]\n", sms_msg);

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(strcmp(resp, "0") == 0)
	{
		DTG_LOGD("resp: %s", resp);
		DTG_LOGD("SEND RESPONSE (TCP)");
		return 0;
	}

	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(dest_info,psms);
		
	if(serial_mode == UT_DEFAULT_MODE)
		sprintf(contents, "1,%s,%d,%d,%d,%d,%d,3,0,9999,9999,-5555,0,%s,ver,7E", get_server_ip_addr(), get_server_port(), get_normal_trans_period(),get_normal_create_period(),get_psave_trans_period(),get_psave_create_period(),get_company_code());
	else
		sprintf(contents, "1,%s,%d,%d,%d,%d,%d,3,1,9999,9999,-5555,0,%s,ver,7E", get_server_ip_addr(), get_server_port(), get_normal_trans_period(),get_normal_create_period(),get_psave_trans_period(),get_psave_create_period(),get_company_code());

	int i;
	fprintf(stderr, "\n");
	for (i = 0; i < strlen(contents); i++)
		fprintf(stderr, "%02x ", contents[i]);
	fprintf(stderr, "\n");

	DTG_LOGD("sms llen: %d", strlen(contents));
	// send sms
	atcmd_send_sms(contents, NULL, dest_info);

	return 0;
}

int sms_set_device_reset(char* sms_msg)
{
	DTG_LOGD("sms_set_device_reset\n");

	char token_1[]=",";
	char token_2[]="\0";

	char pwd[50] = {0,};
	char resp[50] = {0,};
	char *psms;

	psms=strtok(sms_msg,token_1);
	if(psms==0) return -1;
	psms=strtok(0,token_1);
	if(psms==0) return -1;
	strcpy(pwd,psms);

	if(strcmp(pwd, "m2m%flemgkwk*2006") != 0)
		return -1;

	psms=strtok(0,token_2);
	if(psms==0) return -1;
	strcpy(resp,psms);

	if(resp[0] == '1')
	{
		DTG_LOGD("SEND RESPONSE (TCP)");
	}

	device_reset();
	// reset operation

	return 0;
}

int sms_set_geo_fence(char* sms_msg)
{
	DTG_LOGD("sms_set_geo_fence");
	return 0;
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
