#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/thread.h>
#include <board/power.h>
#include <util/tools.h>
#include <config.h>

#include "include/defines.h"
#include "util/nettool.h"
#include "netcom.h"

#include "logd/logd_rpc.h"
#include "util/validation.h"

#include <at/at_util.h>
#include "sms.h"

#include "kjtec_rfid_cmd.h"
#include "kjtec_rfid_tools.h"
#include "cl_rfid_tools.h"

extern RFID_DEV_INFO_T g_rfid_dev_info;

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

static SMS_CMD_FUNC_T sms_cmd_func[] =
{
	{eSMS_CMD_GET__DEVICE_INFO, "&0G", _sms_cmd_proc_get_dev_info},
	{eSMS_CMD_SET__DEVICE_CLR_REDOWN, "&CLR0", _sms_cmd_proc_clear_redown_rfid},
	{eSMS_CMD_SET__DEVICE_CLR, "&CLR1", _sms_cmd_proc_clear},
	{eSMS_CMD_SET__DEVICE_RESET, "&RST", _sms_cmd_proc_device_reset},
	{eSMS_CMD_GET__RFID_FW_VER, "&RFVER", _sms_cmd_proc_rfid_fw_ver},
	{eSMS_CMD_SET__RFID_FW_DOWNLOAD, "&RFDN", _sms_cmd_proc_rfid_fw_down},
};

int parse_model_sms(char *time, char *phonenum, char *sms)
{
//	int msg_type = 0;
	int ret = 0;
	
	int model_argc = 0;
	char *model_argv[32] = {0};

	char *base = 0;
	int len = 0;
	int i = 0;
	
	int found_cmd = 0;

	base = sms;
	len = strlen(sms);
	
	model_argv[model_argc++] = base;

	printf("parse_model_sms start [%s]\r\n", sms);
	while(model_argc <= 32 && len--) 
	{
		switch(*base) {
			case ':':
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			case ',':
				*base = '\0';
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			case '\r':
			case '\n':
				*base = '\0';
				break;
			default:
				break;
		}
		base++;
	}
	model_argc--;
	
	for(i = 0; i <= model_argc; i++)
	{
		LOGD(LOG_TARGET, "%d %s\n", i, model_argv[i]);
	}

	if(model_argv[0] == NULL)
	{
		return -1;
	}
	
	// Command prefix always place in 1st argv.

	// find function..
	for(i = 0; i < MAX_SMS_CMD; i++)
	{
		printf("model_argv[0] [%s], sms_cmd_func[%d].cmd => [%s]\r\n", model_argv[0], i, sms_cmd_func[i].cmd);
		if  (!( strncasecmp ( model_argv[0], sms_cmd_func[i].cmd, strlen(sms_cmd_func[i].cmd) ) ))
		{
			found_cmd = 1;
			break;
		}
	}

	if (found_cmd)
	{
		if ( sms_cmd_func[i].proc_func != NULL )
			sms_cmd_func[i].proc_func(model_argc, model_argv, phonenum);
	}
	
	printf("parse_model_sms end\r\n");
	return ret;
}

// ---------------------------------------------------------------

int _sms_cmd_proc_get_dev_info(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - devinfo / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	sprintf(result_buff,"dev[%s],rfid_conn[%s],rfid_user[%d],last_up[%s]", 
			rfid_tool__get_senario_stat_str(), 
			rfid_tool__get_rifd_dev_stat_str(),
			g_rfid_dev_info.total_passenger_cnt,
			g_rfid_dev_info.saved_timestamp);
	
	at_send_sms(phonenum, result_buff);
	
	return 0;
}


int _sms_cmd_proc_clear_redown_rfid(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - devinfo / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}

	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	if ( rfid_tool__get_senario_stat() == e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS )
	{
		sprintf(result_buff,"CLEAR SUCCESS : REDOWN USER LIST");
		kjtec_rfid_mgr__clr_all_user_data();
	}
	else
	{
		sprintf(result_buff,"CLEAR FAIL : INVALID STAT, RETRY LATER");
	}

	if ( atoi(argv[2]) == 1 )
		at_send_sms(phonenum, result_buff);
	
	return 0;
}


int _sms_cmd_proc_clear(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - devinfo / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}

	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	if ( rfid_tool__get_senario_stat() == e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS )
	{
		RIFD_DATA_ALL_CLR_T all_clr_result;
		memset(&all_clr_result, 0x00, sizeof(all_clr_result));
		if ( kjtec_rfid__dev_rfid_all_clear(&all_clr_result) == KJTEC_RFID_RET_SUCCESS )
		{
			LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : ALL ERASE SUCCESS \r\n");
			sprintf(result_buff ,"%s", "CLEAR SUCCESS");
		}
		else
		{
			// 모두지우는데 실패. 그러므로 다시 시도하기 위해서 리턴
			LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : ALL ERASE FAIL RETURN \r\n");
			sprintf(result_buff,"%s", "CLEAR FAIL : CMD FAIL, RETRY LATER");
		}

		g_need_to_rfid_info = 1;
	}
	else
	{
		sprintf(result_buff,"%s", "CLEAR FAIL : INVLID STAT, RETRY LATER");
	}

	if ( atoi(argv[2]) == 1 )
		at_send_sms(phonenum, result_buff);
	
	return 0;
}








int _sms_cmd_proc_device_reset(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - devinfo / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( argc != 2 )
	{
		LOGE(LOG_TARGET, " - SMS argument invalid [%d] / [%d] \n", argc , 4);
		return -1;
	}

	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
	// ---------------
	sprintf(result_buff,"reset : 1 min after…");
	

	if ( atoi(argv[2]) == 1 )
		at_send_sms(phonenum, result_buff);
	
	poweroff(NULL,0);
	return 0;
}


int _sms_cmd_proc_rfid_fw_ver(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	int max_fw_read_retry = 10;
    int get_fw_ver_success = 0;
    RFID_FIRMWARE_VER_T cur_ver_info;

	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : get - fw ver / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
    memset(&cur_ver_info, 0x00, sizeof(cur_ver_info));

    while(max_fw_read_retry--)
    {
        if ( kjtec_rfid__firmware_ver_info(&cur_ver_info) == KJTEC_RFID_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[sms] kjtec version info [%s]\n", cur_ver_info.data_result  );
            //devel_webdm_send_log("FW DOWN CHK => [%s]", cur_ver_info.data_result);
            get_fw_ver_success = 1;
            break;
        }
        else
            sleep(1);
    }

	if (get_fw_ver_success == 1)
	{
		sprintf(result_buff,"cur rfid firm ver [%s] / last ver [%s]", cur_ver_info.data_result, FW_DOWNLOAD_FILE_LAST_VER_STR);
	}
	else
	{
		sprintf(result_buff,"cur rfid firm ver [%s] / last ver [%s]", "GET FAIL", FW_DOWNLOAD_FILE_LAST_VER_STR);
	}

	// ---------------

	
	at_send_sms(phonenum, result_buff);
	
	return 0;
}



int _sms_cmd_proc_rfid_fw_down(int argc, char* argv[], const char* phonenum)
{
	int i = 0 ;
	int str_len = 0;
	
	int max_fw_read_retry = 10;
    int get_fw_ver_success = 0;
    RFID_FIRMWARE_VER_T cur_ver_info;

	unsigned char result_buff[80] = {0,};
	unsigned char tmp_buff[32] = {0,};
	
	LOGI(LOG_TARGET, "SMS PROC : set - firmware down / from [%s]\n",phonenum);

	for(i = 0; i <= argc; i++)
	{
		LOGD(LOG_TARGET, "SMS - arg [%d] - [%s]\n", i, argv[i]);
	}
	
	// passwd argument check.
	if ( argc == 0 )
	{
		LOGE(LOG_TARGET, "SMS - invalid arg [%d]\n", argc);
		return -1;
	}
	
	if ( strncasecmp(SMS_CMD_PWD, argv[1], strlen(argv[1]) ) != 0 )
	{
		LOGE(LOG_TARGET, " - SMS PWD invalid [%s] / [%s]\n", argv[1], SMS_CMD_PWD);
		return -1;
	}
	
    memset(&cur_ver_info, 0x00, sizeof(cur_ver_info));

	kjtec_rfid_mgr__download_sms_noti_enable(1, phonenum);


    if ( rfid_tool__get_senario_stat() != e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS )
    {
        //LOGI(LOG_TARGET, "[FWDOWN] INVALID STAT [%d]\n", rfid_tool__get_senario_stat()  );
        devel_webdm_send_log("[FWDOWN] INVALID STAT [%d]\n", rfid_tool__get_senario_stat()  );
        at_send_sms(phonenum, "FW DOWN CHK => FAIL : INVALID STAT. TRY LATER");
		return -1;
    }

	
	if ( argc >= 2 )
	{
		set_fwdown_target_ver(argv[2]);
	}
	else
		set_fwdown_target_ver(FW_DOWNLOAD_FILE_LAST_VER_NUM);

	rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_START);
	// ---------------

	
	//at_send_sms(phonenum, result_buff);
	
	return 0;
}