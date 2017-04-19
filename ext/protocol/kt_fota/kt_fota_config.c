#include <iniparser.h>

#include "logd/logd_rpc.h"

#include "kt_fota_config.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE

static dictionary  *g_ini_handle = NULL;

static KT_FOTA_SVR_CONF g_kfs_info =
{
	.dm_server_ip = "",
	.dm_port = 80,
	.qty_server_ip = "",
	.qty_port = 80,
	.acc_qty_report = 0,
	.acc_fota_req_report = 30,
};

int load_ini_kt_fota_svc_info()
{
	int num;
	char *str;
	int retry = 0;

	while ((g_ini_handle == NULL) && (retry < 5)) {
		g_ini_handle = iniparser_load(KT_FOTA_FILE_PATH);
		if (g_ini_handle == NULL){
			LOGE(LOG_TARGET, "Can't load %s at %s", KT_FOTA_FILE_PATH, __func__);
			sleep(2);
			retry++;
		}
	}
	if (g_ini_handle==NULL)
		return -1;
	
	str = iniparser_getstring(g_ini_handle, "kt_fota_srv:dm_server_ip", NULL);
	if(str == NULL) return -1;
	set_kt_fota_dm_server_ip_addr(str);

	num = iniparser_getint(g_ini_handle, "kt_fota_srv:dm_port", -1);
	if(num == -1) return -1;
	set_kt_fota_dm_server_port(num);

	str = iniparser_getstring(g_ini_handle, "kt_fota_srv:qty_server_ip", NULL);
	if(str == NULL) return -1;
	set_kt_fota_qty_server_ip_addr(str);

	num = iniparser_getint(g_ini_handle, "kt_fota_srv:qty_port", -1);
	if(num == -1) return -1;
	set_kt_fota_qty_server_port(num);

	num = iniparser_getint(g_ini_handle, "kt_fota_srv:acc_qty_report", -1);
	if(num == -1) return -1;
	set_kt_fota_qry_report(num);

	num = iniparser_getint(g_ini_handle, "kt_fota_srv:acc_fota_req_report", -1);
	if(num == -1) return -1;
	set_kt_fota_req_report(num);

	return 1;
}

int save_ini_kt_fota_svc_info()
{
	char tmp[50] = {0,};

    if (g_ini_handle==NULL) {
		LOGE(LOG_TARGET, "Can't load %s at %s", KT_FOTA_FILE_PATH, __func__);
        return -1 ;
    }

	iniparser_set(g_ini_handle, "kt_fota_srv:dm_server_ip", get_kt_fota_dm_server_ip_addr());

	sprintf(tmp, "%d", get_kt_fota_dm_server_port());
	iniparser_set(g_ini_handle, "kt_fota_srv:dm_port", tmp);

	iniparser_set(g_ini_handle, "kt_fota_srv:qty_server_ip", get_kt_fota_qty_server_ip_addr());

	sprintf(tmp, "%d", get_kt_fota_qty_server_port());
	iniparser_set(g_ini_handle, "kt_fota_srv:qty_port", tmp);

	sprintf(tmp, "%d", get_kt_fota_qry_report());
	iniparser_set(g_ini_handle, "kt_fota_srv:acc_qty_report", tmp);

	sprintf(tmp, "%d", get_kt_fota_req_report());
	iniparser_set(g_ini_handle, "kt_fota_srv:acc_fota_req_report", tmp);

	iniparser_store(g_ini_handle, KT_FOTA_FILE_PATH) ;
	return 0;
}

void free_ini_file()
{
	if(g_ini_handle != NULL) 
	{
		iniparser_freedict(g_ini_handle);
		g_ini_handle = NULL;
	}
}

char* get_kt_fota_dm_server_ip_addr()
{
	return g_kfs_info.dm_server_ip;
}
void set_kt_fota_dm_server_ip_addr(char* ip_addr)
{
	strcpy(g_kfs_info.dm_server_ip, ip_addr);
}

char* get_kt_fota_qty_server_ip_addr()
{
	return g_kfs_info.qty_server_ip;
}
void set_kt_fota_qty_server_ip_addr(char* ip_addr)
{
	strcpy(g_kfs_info.qty_server_ip, ip_addr);
}

unsigned short get_kt_fota_dm_server_port()
{
	return g_kfs_info.dm_port;
}
void set_kt_fota_dm_server_port(unsigned short port)
{
	g_kfs_info.dm_port = port;
}

unsigned short get_kt_fota_qty_server_port()
{
	return g_kfs_info.qty_port;
}
void set_kt_fota_qty_server_port(unsigned short port)
{
	g_kfs_info.qty_port = port;
}

void set_kt_fota_qry_report(int num)
{
	g_kfs_info.acc_qty_report = num;
}
int get_kt_fota_qry_report()
{
	return g_kfs_info.acc_qty_report;
}

void set_kt_fota_req_report(int num)
{
	g_kfs_info.acc_fota_req_report = num;
}
int get_kt_fota_req_report()
{
	return g_kfs_info.acc_fota_req_report;
}

