#include <stdio.h>
#include <string.h>
#include <jansson.h>

#include <logd/logd_rpc.h>
#include "kt_fs_send.h"
#include "../kt_fota_config.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE

int device_on_noti_parser(char *contents_data)
{
	int result = 0;
	json_t *root = NULL;
	json_error_t error;
	char *retrun_code = NULL;
	char *config_fota_acc_time = NULL;
	char *config_dev_qinfo_acc_time = NULL;
	char *cust_tag = NULL;
	int num;
	int save_flag = 0;

	root = json_loads(contents_data, 0, &error);
	if(!root){
		LOGE(LOG_TARGET, "%s:%d> json_loads \n", __func__, __LINE__);
		LOGE(LOG_TARGET, "error : on line %d: %s\n", error.line, error.text);
		result = -20;
		goto error_finish;
	}


	if( json_unpack(root, "{s:s, s:{s:s, s:s}, s:s}",	"Return_Code", &retrun_code,
														"Config", 
														"fota_acc_time", &config_fota_acc_time,
														"device_qinfo_acc_time", &config_dev_qinfo_acc_time,
														"Custom_Tag", &cust_tag
														) < 0) 
	{
		LOGE(LOG_TARGET, "%s:%d> Error json_unpack \n", __func__, __LINE__);
		result = -30;
		goto error_finish;
	}


	num = atoi(retrun_code);
	LOGD(LOG_TARGET, "%s:%d> fota server return code [%d]\n", __func__, __LINE__, num);
	if(config_fota_acc_time != NULL) {
		num = atoi(config_fota_acc_time) * 60;
		if(num != get_kt_fota_req_report()) 
		{
			set_kt_fota_req_report(num);
			save_flag = 1;
		}
	}

	if(config_dev_qinfo_acc_time != NULL)
	{
		num = atoi(config_dev_qinfo_acc_time) * 60;
		if(num != get_kt_fota_qry_report())
		{
			set_kt_fota_qry_report(num);
			save_flag = 1;
		}
	}

	if(save_flag == 1) 
	{
		save_ini_kt_fota_svc_info();
	}

error_finish:

	if(root != NULL)
		json_decref(root);

	return result;
}


int fota_req_parser(char *contents_data)
{
		int result = 0;
	json_t *root = NULL;
	json_error_t error;
	char *retrun_code = NULL;
	char *url = NULL;
	char *crc = NULL;
	char *config_reset = NULL;
	char *config_fota_acc_time = NULL;
	char *config_dev_qinfo_acc_time = NULL;
	char *cust_tag = NULL;
	char *echo_tag = NULL;
	int num;
	int save_flag = 0;

	root = json_loads(contents_data, 0, &error);
	if(!root){
		LOGE(LOG_TARGET, "%s:%d> json_loads \n", __func__, __LINE__);
		LOGE(LOG_TARGET, "error : on line %d: %s\n", error.line, error.text);
		result = -20;
		goto error_finish;
	}

	if( json_unpack(root, "{s:s, s:s, s:s, s:{s:s, s:s, s:s}, s:s, s:s}",	
															"Return_Code", &retrun_code,
															"URL", &url,
															"CRC", &crc,
															"Config", 
															"reset", &config_reset,
															"fota_acc_time", &config_fota_acc_time,
															"device_qinfo_acc_time", &config_dev_qinfo_acc_time,
															"Custom_Tag", &cust_tag,
															"Echo_Tag", &echo_tag
														) < 0)
	{
		LOGE(LOG_TARGET, "%s:%d> Error json_unpack \n", __func__, __LINE__);
		result = -30;
		goto error_finish;
	}

	num = atoi(retrun_code);
	LOGD(LOG_TARGET, "%s:%d> fota server return code [%d]\n", __func__, __LINE__, num);

	if(config_fota_acc_time != NULL) {
		num = atoi(config_fota_acc_time) * 60;
		if(num != get_kt_fota_req_report()) 
		{
			set_kt_fota_req_report(num);
			save_flag = 1;
		}
	}

	if(config_dev_qinfo_acc_time != NULL)
	{
		num = atoi(config_dev_qinfo_acc_time) * 60;
		if(num != get_kt_fota_qry_report())
		{
			set_kt_fota_qry_report(num);
			save_flag = 1;
		}
	}

	if(save_flag == 1) 
	{
		save_ini_kt_fota_svc_info();
	}

error_finish:

	if(root != NULL)
		json_decref(root);

	return result;
}


int kt_fota_srv_data_parse(KT_Fota_Svc_t api, char *contents_data)
{
	int result = -1;
	LOGD(LOG_TARGET, "%s> %s\n", __func__, contents_data);
	if(contents_data == NULL)
		return -1;

	if(strlen(contents_data) < 10)
		return -1;
	
	if(api == eKT_DEVICE_ON_NOTI)
	{
		result = device_on_noti_parser(contents_data);
		
	}
	else if(api == eKT_FOTA_REQ)
	{
		result = fota_req_parser(contents_data);
	}
	
	return result;
}
