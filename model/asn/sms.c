#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/watchdog.h>
#include <board/power.h>
#include <config.h>
#include <logd_rpc.h>
#include "sms.h"

extern void ans_cmd_forward(char *cmd, int cmd_len);
int parse_model_sms(const char *time, const char *phonenum, const char *sms)
{

    int argc = 0;
    char *argv[10] = {0};
    int len = 0;
    char *base = 0;
	int port;
    int i;

    char sms_trans_buffer[256] = {0};

	if(sms == NULL) {
		LOGE(eSVC_COMMON, "%s> err sms data is NULL\n", __func__);
		return -1;
	}
	if(time == NULL) {
		LOGE(eSVC_COMMON, "%s> err time data is NULL\n", __func__);
		return -1;
	}
	if(phonenum == NULL) {
		LOGE(eSVC_COMMON, "%s> err phonenum data is NULL\n", __func__);
		return -1;
	}

	LOGT(eSVC_COMMON, "%s> %s %s %s\n", __func__, time, phonenum, sms);

    strcpy(sms_trans_buffer, sms);

    len = strnlen(sms_trans_buffer, sizeof(sms_trans_buffer));
    
	argc = 0;
    base = sms_trans_buffer;
	argv[argc] = base;
    while(argc <= 10 && len--) {
        switch(*base) {
            case ',':
                if(argc == 10)
                {
                    break;
                }
                *base = 0x00;//'\0';
				argc++;
                argv[argc] = base + 1;
                break;
            case 0x0a:
                *base = 0x00;//'\0';
				argc++;
                argv[argc] = base + 1;
                break;
            default:
                break;
        }
        base++;
    }
    argc--;
	
    for(i = 0; i <= argc; i++)
	{
		LOGI(eSVC_COMMON, "%d %s\n", i, argv[i]);
        LOGI(eSVC_COMMON, "%d %s\n", i, argv[i]);
        LOGI(eSVC_COMMON, "%d %s\n", i, argv[i]);
        usleep(100);
	}

	if(argc < 3) {
		LOGE(eSVC_COMMON, "%s> err SMS format is wrong => argc is [%d]\n", __func__, argc);
		return -1;
	}
	if(strncmp(argv[1], "KFRI1234", 8)) {
		if(strncmp(argv[1], "KARI1234", 8)) {
			LOGE(eSVC_COMMON, "%s> err SMS passwrd is wrong\n", __func__);
			return -1;
		}
	}
	
	if(argv[0][0] == '&' && argv[0][1] == '1')
	{
		
		//ip, port chanage
		configurationModel_t *conf = get_config_model();
		ans_cmd_forward((char *)sms, strlen(sms));

		if(strncmp(conf->model.report_ip, argv[2], sizeof(conf->model.report_ip)))
		{
			strcpy(conf->model.report_ip, argv[2]);
			save_config_user("user:report_ip", argv[2]);
		}
		port = atoi(argv[3]);
		if(port > 0) {
			if(conf->model.report_port != port) {
				conf->model.report_port = port;
				save_config_user("user:report_port", argv[3]);
			}
		}

	}
	else if(argv[0][0] == '&' && argv[0][1] == '2')
	{
		//period chanage
		ans_cmd_forward((char *)sms, strlen(sms));
	}

	return 0;
}
