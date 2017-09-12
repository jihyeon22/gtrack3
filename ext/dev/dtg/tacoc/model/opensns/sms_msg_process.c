/*
 * sms_msg_process.c
 *
 *  Created on: 2013. 3. 19.
 *      Author: gbuddha
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <wrapper/dtg_atcmd.h>
#include "dtg_debug.h"

#include "dtg_ini_utill.h"
#include "dtg_data_manage.h"
#include "rpc_clnt_operation.h"

#include <wrapper/dtg_version.h>

int sms_set_mdt_svrip_info(char* sms_msg)
{
	char *temp_bp;
	char token_1[]=",";
	char token_2[]="\0";

	char ip[64] = {0,};
	char tmp_str[50] = {0,};
	int port = -1;
	char *psms;

	psms=strtok_r(sms_msg,token_1, &temp_bp); //ip
	if(psms==0) return -1;
	strcpy(ip,psms);

	psms=strtok_r(NULL,token_2, &temp_bp); //port
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	port = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%s %d\n", ip, port);

	set_mdt_server_ip_addr(ip);
	set_mdt_server_port(port);

	save_ini_mdt_server_setting_info();
	return 0;

}
int sms_set_dtg_svrip_info(char* sms_msg)
{
	char *temp_bp;
	char token_1[]=",";
	char token_2[]="\0";

	char ip[64] = {0,};
	char tmp_str[50] = {0,};
	int port = -1;
	char *psms;

	psms=strtok_r(sms_msg,token_1, &temp_bp); //ip
	if(psms==0) return -1;
	strcpy(ip,psms);

	psms=strtok_r(NULL,token_2, &temp_bp); //port
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	port = strtol(tmp_str, NULL, 10);

	DTG_LOGD("%s %d\n", ip, port);

	set_server_ip_addr(ip);
	set_server_port(port);

	save_ini_dtg_server_setting_info();
	return 0;
}

int sms_set_mdt_period(char* sms_msg)
{
	char *temp_bp;
	char token_1[]=",";
	char token_2[]="\0";

	char tmp_str[50] = {0,};
	int report_time = 0;
	int create_time = 0;
	char *psms;

	psms=strtok_r(sms_msg,token_1, &temp_bp); //report period
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	report_time = strtol(tmp_str, NULL, 10);

	psms=strtok_r(NULL,token_2, &temp_bp); //create period
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	create_time = strtol(tmp_str, NULL, 10);

	DTG_LOGD("SMS MDT PERIOD SET : %d, %d\n", report_time, create_time);

	set_mdt_report_period(report_time);
	set_mdt_create_period(create_time);

	save_ini_mdt_period_info();

	return 0;
}

int sms_set_dtg_period(char* sms_msg)
{
	char *temp_bp;
	char token_2[]="\0";

	char tmp_str[50] = {0,};
	int report_time = 0;
	char *psms;

	psms=strtok_r(sms_msg,token_2, &temp_bp); //report period
	if(psms==0) return -1;
	strcpy(tmp_str,psms);
	report_time = strtol(tmp_str, NULL, 10);

	DTG_LOGD("SMS DTG PERIOD SET : %d\n", report_time);

	set_dtg_report_period(report_time);

	save_ini_dtg_period_info();
	return 0;
}

int sms_set_device_reset(char* sms_msg)
{
	char *temp_bp;
	char token_2[]="\0";
	char *psms;

	psms=strtok_r(sms_msg,token_2, &temp_bp); //report period
	if(psms==0) return -1;
	if(!strcmp(psms, "mdt%reset#tngodgofk!"))
	{
		while(1) {
			DTG_LOGE("wait sms device reset...");
			//system("poweroff");
			poweroff(NULL,0);
			sleep(3);
		}
	}

	return 0;
}

int set_dtg_value(unsigned char *buf, int nbytes)
{
	int fd;
	struct termios newtio, oldtio;
	int i;

	fd = open("/dev/ttyMSM", O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0) {
		printf("uart open fail\n");
		return -1;
	}

	memset(&newtio, 0, sizeof(newtio));
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_cflag = CS8 | CLOCAL | CREAD;

	newtio.c_cflag |= B115200;
	newtio.c_lflag = 0;		 // non-canonical ?�력 모드
	newtio.c_cc[VTIME] = 0;	 // 무제???���?	newtio.c_cc[VMIN] = 1;	 
	
	tcgetattr(fd, &oldtio);
	tcflush (fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	int ret = write(fd, buf, nbytes);
	printf("write len = [%d]\n", ret);

	close(fd);

	return 0;
}

int sms_set_dtg_setting_value1(char* sms_msg)
{

	int i;
	int total_length = 0;
	char tmp_str[80] = {0,};
	int tmp_str_idx = 0;
	int tmp_value;

	char cmd_buf[256];
	unsigned short data_len = 103;
	unsigned short write_bit_flag = 0;
	int idx = 0;
	innocar_dtg_set_value_t ino_dtg_val;

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(&ino_dtg_val, 0, sizeof(innocar_dtg_set_value_t));
	cmd_buf[0] = 0x02;
	cmd_buf[1] = 0x50;
	memcpy(&cmd_buf[2], &data_len, 2);
	strncpy(&cmd_buf[4], "RW", 2);

	int step = 0;

	//printf("sizeof(innocar_dtg_set_value_t) = [%d]\n", sizeof(innocar_dtg_set_value_t));
	//sizeof(innocar_dtg_set_value_t) --> length : 97
	total_length = strlen(sms_msg);
	memset(tmp_str, 0x00, sizeof(tmp_str));
	tmp_str_idx = 0;
	for(i = 0; i <= total_length; i++) {
		if(sms_msg[i] == ',' || i == total_length)
		{
			switch(step)
			{
				case 0: //registration_num
					if(strlen(tmp_str) > 0) {
						strncpy(ino_dtg_val.registration_num, tmp_str, 12);
						write_bit_flag |= SET_FLAG_RN;
						//printf("registration_num = [%s]\n", ino_dtg_val.registration_num);
					}
					break;
				case 1: //vehicle_type
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.vehicle_type = tmp_value;
						write_bit_flag |= SET_FLAG_VT;
						//printf("vehicle_type = [%d]\n", ino_dtg_val.vehicle_type);
					}
					break;
				case 2: //driver_name
					if(strlen(tmp_str) > 0) {
						strncpy(ino_dtg_val.driver_name, tmp_str, 10);
						write_bit_flag |= SET_FLAG_DRIVER_NAME;
						//printf("driver_name = [%s]\n", ino_dtg_val.driver_name);
					}
					break;
				case 3: //driver_code
					if(strlen(tmp_str) > 0) {
						strncpy(ino_dtg_val.driver_code, tmp_str, 18);
						write_bit_flag |= SET_FLAG_DRIVER_CODE;
						//printf("driver_code = [%s]\n", ino_dtg_val.driver_code);
					}
					break;
				case 4: //vehicle_id_num
					if(strlen(tmp_str) > 0) {
						strncpy(ino_dtg_val.vehicle_id_num, tmp_str, 17);
						write_bit_flag |= SET_FLAG_VIN;
						//printf("vehicle_id_num = [%s]\n", ino_dtg_val.vehicle_id_num);
					}
					break;
				case 5: //business_license_num
				
					if(strlen(tmp_str) > 0) {
						strncpy(ino_dtg_val.business_license_num, tmp_str, 10);
						write_bit_flag |= SET_FLAG_BLN;
						//printf("business_license_num = [%s]\n", ino_dtg_val.business_license_num);
					}
					break;
			}
			step += 1;
			memset(tmp_str, 0x00, sizeof(tmp_str));
			tmp_str_idx = 0;
		}
		else
		{
			tmp_str[tmp_str_idx++] = sms_msg[i];
		}
	}

	printf("write_bit_flag = [0x%08x]\n", write_bit_flag);
	memcpy(&cmd_buf[6], &write_bit_flag, 2); 
	memcpy(&cmd_buf[8], &ino_dtg_val, sizeof(innocar_dtg_set_value_t));
	
	for(i = 0; i < 8+sizeof(innocar_dtg_set_value_t); i++){
		cmd_buf[8+sizeof(innocar_dtg_set_value_t)] += cmd_buf[i];
	}

	cmd_buf[8+sizeof(innocar_dtg_set_value_t)+1] = 0x03;
	printf("packet end idx = [%d]\n", 8+sizeof(innocar_dtg_set_value_t)+1);

	set_dtg_value(cmd_buf, data_len+4);
	sleep(1);
	set_dtg_value(cmd_buf, data_len+4);

	set_innoca_dtg_setting_enable(1);

	return 0;
}




int sms_set_dtg_setting_value2(char* sms_msg)
{
	int i;
	int total_length = 0;
	char tmp_str[80] = {0,};
	int tmp_str_idx = 0;
	int tmp_value;

	char cmd_buf[256];
	unsigned short data_len = 103;
	unsigned short write_bit_flag = 0;
	int idx = 0;
	innocar_dtg_set_value_t ino_dtg_val;

	memset(cmd_buf, 0, sizeof(cmd_buf));
	memset(&ino_dtg_val, 0, sizeof(innocar_dtg_set_value_t));
	cmd_buf[0] = 0x02;
	cmd_buf[1] = 0x50;
	memcpy(&cmd_buf[2], &data_len, 2);
	strncpy(&cmd_buf[4], "RW", 2);

	int step = 0;

	//printf("sizeof(innocar_dtg_set_value_t) = [%d]\n", sizeof(innocar_dtg_set_value_t));
	//sizeof(innocar_dtg_set_value_t) --> length : 97
	total_length = strlen(sms_msg);
	memset(tmp_str, 0x00, sizeof(tmp_str));
	tmp_str_idx = 0;
	for(i = 0; i <= total_length; i++) {
		if(sms_msg[i] == ',' || i == total_length)
		{
			switch(step)
			{
				case 0: //company name
					if(strlen(tmp_str) > 0) {
						strncpy(ino_dtg_val.commay_name, tmp_str, 12);
						write_bit_flag |= SET_FLAG_COMPANY_NAME;
						printf("company name = [%s]\n", ino_dtg_val.commay_name);
					}
					break;
				case 1: //k-factor
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.k_factor = tmp_value;
						write_bit_flag |= SET_FLAG_K_FACTOR;
						printf("k_factor = [%d]\n", ino_dtg_val.k_factor);
					}
					break;
				case 2: //rpm-factor
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.rmp_factor = tmp_value;
						write_bit_flag |= SET_FLAG_RPM_FACTOR;
						printf("rmp_factor = [%d]\n", ino_dtg_val.rmp_factor);
					}
					break;
				case 3: //odo
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.odo = tmp_value;
						write_bit_flag |= SET_FLAG_OOD;
						printf("odo = [%ld]\n", ino_dtg_val.odo);
					}
					break;
				case 4: //cumu_oil_usage
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.cumul_oil_usage = tmp_value;
						write_bit_flag |= SET_FLAG_CUMUL_OIL_USAGE;
						printf("cumul_oil_usage = [%ld]\n", ino_dtg_val.cumul_oil_usage);
					}
					break;
				case 5: //weight1
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.weight1 = tmp_value;
						write_bit_flag |= SET_FLAG_WEIGHT1;
						printf("weight1 = [%d]\n", ino_dtg_val.weight1);
					}
					break;
				case 6: //weight2
					if(strlen(tmp_str) > 0) {
						tmp_value = strtol(tmp_str, NULL, 10);
						ino_dtg_val.weight2 = tmp_value;
						write_bit_flag |= SET_FLAG_WEIGHT2;
						printf("weight2 = [%d]\n", ino_dtg_val.weight2);
					}
					break;
			}
			step += 1;
			memset(tmp_str, 0x00, sizeof(tmp_str));
			tmp_str_idx = 0;
		}
		else
		{
			tmp_str[tmp_str_idx++] = sms_msg[i];
		}
	}

	printf("write_bit_flag = [0x%08x]\n", write_bit_flag);
	memcpy(&cmd_buf[6], &write_bit_flag, 2); 
	memcpy(&cmd_buf[8], &ino_dtg_val, sizeof(innocar_dtg_set_value_t));
	
	for(i = 0; i < 8+sizeof(innocar_dtg_set_value_t); i++){
		cmd_buf[8+sizeof(innocar_dtg_set_value_t)] += cmd_buf[i];
	}

	cmd_buf[8+sizeof(innocar_dtg_set_value_t)+1] = 0x03;
	printf("packet end idx = [%d]\n", 8+sizeof(innocar_dtg_set_value_t)+1);

	set_dtg_value(cmd_buf, data_len+4);
	sleep(1);
	set_dtg_value(cmd_buf, data_len+4);

	set_innoca_dtg_setting_enable(1);

	return 0;
}

int sms_device_status_req(char *sender)
{
	char sms_buf[128];
	char tmp[128];
	int idx;
	int len;

	idx = 0;
	//strncpy(&sms_buf[idx], "MDS", 3);
	//idx += 3;
	//sms_buf[idx++] = 0x20; //0x20 is space

	memset(sms_buf, 0x20, sizeof(sms_buf));

	strncpy(&sms_buf[idx], "W200K", 5);
	idx += 5;
	sms_buf[idx++] = 0x20; //0x20 is space

	strncpy(&sms_buf[idx], SW_VERSION, 5); //SW_VERSION : vx.xx
	idx += 5;
	sms_buf[idx++] = 0x20; //0x20 is space

	strncpy(&sms_buf[idx], "INO", 3);
	idx += 3;
	sms_buf[idx++] = 0x20; //0x20 is space

	len = strlen(get_dtg_version());
	strncpy(&sms_buf[idx], get_dtg_version(), len);
	idx += len;
	sms_buf[idx++] = 0x20; //0x20 is space

	sprintf(tmp, "%s:%d", get_server_ip_addr(), get_server_port());
	len = strlen(tmp);
	strncpy(&sms_buf[idx], tmp, len);
	idx += len;
	sms_buf[idx++] = 0x20; //0x20 is space

	sprintf(tmp, "%d:%d", 1, get_dtg_report_period());
	len = strlen(tmp);
	strncpy(&sms_buf[idx], tmp, len);
	idx += len;
	sms_buf[idx++] = 0x20; //0x20 is space

	sprintf(tmp, "%d:%d:%d:%d", get_k_factor(), get_rmp_factor(), get_weight1(), get_weight2());
	len = strlen(tmp);
	strncpy(&sms_buf[idx], tmp, len);
	idx += len;
	sms_buf[idx++] = 0x20; //0x20 is space

	sms_buf[79] = 0x00;
	atcmd_send_sms(sms_buf, NULL, sender);

	//MDS W200K v1.00 ECHO_DTG-1000H 012345F 111.111.111.111:51000 123456:123456 123456:123:123:123 --> +13
	//sprintf(sms_buf, "W200K v1.00 INO 012345F 111.111.111.111:51000 123456:123456 123456:123:123:123");
	//sms_buf[79] = 0x00;
	//atcmd_send_sms(sms_buf, NULL, sender);
	
	return 0;
}
