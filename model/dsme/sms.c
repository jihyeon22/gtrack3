#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <board/power.h>
#include <config.h>
#include <logd_rpc.h>
#include <util/poweroff.h>
#include "sms.h"

int parse_model_sms(const char *time, const char *phonenum, const char *sms)
{

	char *temp_bp = NULL;
	char token_0[ ] = "[";
	char token_1[ ] = "]";
	char token_2[ ] = ",";
	char token_3[ ] = "/";
	char *tr;
	int i;
	int j;
	char sms_filter[128];
	char *cmd_list[] = {"&IP,[", "[IVSCLT,", "[RBREST,", "[IVSSTP,", "[RESET]", "[ADDCLT,", NULL};

	char *p_str_ip;
	char *p_str_port;

	char *p_str_vold_t_time;
	char *p_str_report_time;

	
	configurationModel_t * conf = get_config_model();

	memset(sms_filter, 0x00, sizeof(sms_filter));
	for(i = 0, j = 0; i < strlen(sms); i++) {
		if(sms[i] >= 'a' && sms[i] <= 'z')
			sms_filter[j++] = sms[i]-32;
		else if(sms[i] == ' ')
			continue;
		else
			sms_filter[j++] = sms[i];
	}

	printf("org sms ==> %s\n", sms);
	printf("filter sms ==> %s\n", sms_filter);


	i = 0;
	while(1) {
		if(cmd_list[i] == NULL) {
			printf("unknown sms command\n");
			return -1;
		}
		if(strstr(sms_filter, cmd_list[i]) != NULL) {
			break;
		}
		i += 1;
	}

	switch(i) {
		case 0: //IP Command;
			tr = strtok_r(sms_filter, token_0, &temp_bp); //token_0 : '['
			if(tr == NULL) {
				printf("ip command parse error #1\n");
				return -1;
			}
			p_str_ip = strtok_r(NULL, token_1, &temp_bp); //token_1 : ']'
			if(p_str_ip == NULL) {
				printf("ip command parse error #2\n");
				return -1;
			}
			printf("ip#2 : IP : %s\n", p_str_ip);
			

			tr = strtok_r(NULL, token_0, &temp_bp);
			if(tr == NULL) {
				printf("ip command parse error #3\n");
				return -1;
			}

			p_str_port = strtok_r(NULL, token_1, &temp_bp);
			if(p_str_port == NULL) {
				printf("ip command parse error #4\n");
				return -1;
			}
			printf("ip#4 : Port: %s\n", p_str_port);

			
			strncpy(conf->model.report_ip, p_str_ip, sizeof(conf->model.report_ip));
			conf->model.report_port = atoi(p_str_port);

			if(save_config_user("user:report_ip", p_str_ip) < 0)
			{
				printf("<%s> save config error #1\n", __FUNCTION__);
				return -1;
			}
			if(save_config_user("user:report_port", p_str_port) < 0)
			{
				printf("<%s> save config error #2\n", __FUNCTION__);
				return -1;
			}

		break;

		case 1: //IVSCLT Command;
			tr = strtok_r(sms_filter, token_2, &temp_bp);
			if(tr == NULL) {
				printf("IVSCLT command parse error #1\n");
				return -1;
			}
			//printf("iIVSCLT#1 : tr[%s]\n", tr);

			tr = strtok_r(NULL, token_2, &temp_bp); //auto play
			if(tr == NULL) {
				printf("IVSCLT command parse error #2\n");
				return -1;
			}
			printf("iIVSCLT#2 : autoplay %s\n", tr);

			p_str_vold_t_time = strtok_r(NULL, token_2, &temp_bp); //volt & temp colletion period time
			if(p_str_vold_t_time == NULL) {
				printf("IVSCLT command parse error #3\n");
				return -1;
			}
			printf("iIVSCLT#3 : volt & temp collection period time %s\n", p_str_vold_t_time);

			p_str_report_time = strtok_r(NULL, token_1, &temp_bp); //report period time
			if(p_str_report_time == NULL) {
				printf("IVSCLT command parse error #4\n");
				return -1;
			}
			printf("iIVSCLT#4 : report period time %s\n", p_str_report_time);

			conf->model.InV_T_collection_interval = atoi(p_str_vold_t_time);
			conf->model.report_interval = atoi(p_str_report_time);

			if(save_config_user("user:InV_T_collection_interval", p_str_vold_t_time) < 0)
			{
				printf("<%s> save config error #1\n", __FUNCTION__);
				return -1;
			}
			if(save_config_user("user:report_interval", p_str_report_time) < 0)
			{
				printf("<%s> save config error #2\n", __FUNCTION__);
				return -1;
			}

			
		break;

		case 2: //RBREST Command;
			tr = strtok_r(sms_filter, token_2, &temp_bp);
			if(tr == NULL) {
				printf("RBREST command parse error #1\n");
				return -1;
			}
			tr = strtok_r(NULL, token_1, &temp_bp);
			if(tr == NULL) {
				printf("RBREST command parse error #2\n");
				return -1;
			}
			printf("RBREST#2 : baudrate %s\n", tr);
		break;

		case 3: //IVSSTP Command;
			tr = strtok_r(sms_filter, token_2, &temp_bp);
			if(tr == NULL) {
				printf("IVSSTP command parse error #1\n");
				return -1;
			}
			tr = strtok_r(NULL, token_1, &temp_bp);
			if(tr == NULL) {
				printf("IVSSTP command parse error #2\n");
				return -1;
			}
			printf("IVSSTP#2 : mode %s\n", tr);
		break;

		case 4: //RESET Command;
			//system reset;
			poweroff("Reset by SMS", sizeof("Reset by SMS"));
			printf("system reset\n");
		break;

		case 5: //ADDCLT Command;
			tr = strtok_r(sms_filter, token_2, &temp_bp);
			if(tr == NULL) {
				printf("ADDCLT command parse error #1\n");
				return -1;
			}
			//printf("ADDCLT#1 : tr[%s]\n", tr);
			tr = strtok_r(NULL, token_3, &temp_bp);
			if(tr == NULL) {
				printf("ADDCLT command parse error #2\n");
				return -1;
			}
			printf("ADDCLT#2 : command %s\n", tr);
			tr = strtok_r(NULL, token_1, &temp_bp);
			if(tr == NULL) {
				printf("ADDCLT command parse error #3\n");
				return -1;
			}
			printf("ADDCLT#3 : tr[%s]\n", tr);
		break;

		default:
			printf("unknown sms command index over\n");
		return -1;
	}

	return 0;
}
