#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <at/at_util.h>
#include <dm/package.h>
#include <kt_fota_ver.h>

#include <mdsapi/mds_api.h>

#include "../kt_fota_config.h"

char *kt_dm_srv_get_ip()
{
	return get_kt_fota_dm_server_ip_addr();
}

int kt_dm_srv_get_port()
{
	return get_kt_fota_dm_server_port();
}

char *kt_qty_srv_get_ip()
{
	return get_kt_fota_qty_server_ip_addr();
}
int kt_qty_srv_get_port()
{
	return get_kt_fota_qty_server_port();
}

char *kt_srv_get_cti(char *buf_out)
{
	char phonenum[AT_LEN_PHONENUM_BUFF] = {0,};
	if(at_get_phonenum(phonenum, sizeof(phonenum)) < 0)
	{
		strcpy(buf_out, "");
	}
	else
	{
		strcpy(buf_out, phonenum);//"01221999772");
	}
	
	return buf_out;	
}

char *kt_srv_get_model_name()
{
	return KT_FOTA_MODEL;
}

char *kt_srv_get_cust_tag(char *buf_out)
{
	strcpy(buf_out, "KT0000001");
	//strcpy(buf_out, "");
	return buf_out;
}


char *dummy_ktdevstat(char *buf_out) 
{

	//char *data[] = {"0","0","0","0","0","0","0","0","0","0","0","0","0","0",KT_FOTA_MODEL,"0","0","0", NULL};
	////KTDEVSTAT:{"0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "0", "D/N":KT_FOTA_MODEL, "0", "0", "0", "0", "0", "0", "0", "0", NULL}

	char *data = "\"2.0.0\", \"-86\", \"0\", \"privatelte.ktfwing.com\", \"00/0000\", \"0\", \"2\", \"NS\", \"NS\", \"10812\", \"WCDMA\", \"TL500K\", \"1.1.0\", \"352992033762503\", \"8982300814008521900F\", \"NS\", \"NS\", \"NS\", \"NS\", \"NS\"";

	sprintf(buf_out, "[%s]",data);
	printf(buf_out, "%s",data);
	printf(buf_out, "%s",data);
	printf(buf_out, "%s",data);
	printf(buf_out, "%s",data);

	return buf_out;
}

int at_get_ktdevstat(char *stat_buf, int buf_len)
{
	return -1;
}


char *kt_srv_get_modem_qty(char *buf_out, int buf_len) 
{
	if(get_at_ktdevstat_for_tl500k(buf_out) < 0)
		dummy_ktdevstat(buf_out);

	return buf_out;
}

extern int tools_get_available_memory(void);

int check_hardware()
{
	char ant[10] = {0};
	if (mds_api_gps_util_get_gps_ant() == DEFINES_MDS_API_OK )
		return 0;
	else
		return 1; //0 : nomal, 1 : abnormal

}

char* get_error_status(char *err_buf)
{
	char ant[10] = {0};
	if (mds_api_gps_util_get_gps_ant() == DEFINES_MDS_API_OK )
	{
		strcpy(err_buf, "");
	}
	else
	{
		strcpy(err_buf, "E002");
	}

	return err_buf;
}

char *kt_srv_get_device_qty_cpu(char *buf_out)
{
	return "";
}

char *kt_srv_get_device_qty_mem(char *buf_out)
{
	//Memory
	int mem = 0;
	float percent = 0;
	mem = tools_get_available_memory();
	printf("---------------------------------------------------\r\n");
	printf(" >> mem is [%d]\r\n",mem);
	
	if(mem < 0) mem = 9; //default 9MB
	else mem /= 1024;

	percent = 100.0 - (float)mem/160000.0*100.0;
	printf(" >> percent is [%f]\r\n",percent);
	sprintf(buf_out, "%d", (int)percent);
	printf(" >> buf_out is [%s]\r\n",buf_out);

	printf("---------------------------------------------------\r\n");
	
	return buf_out;
}

char *kt_srv_get_device_qty_hw(char *buf_out)
{
	sprintf(buf_out, "%d", check_hardware());
	return buf_out;
}

char *kt_srv_get_device_qty_err(char *buf_out)
{
	return get_error_status(buf_out);
}

char *kt_srv_get_package_version(char *buf_out)
{
#if 0
	char buf[256];
	//sprintf(buf, "%s.%s.%s-%s", MDT_MODEL, SVR_MODEL, DTG_MODEL, SW_VERSION);
	//sprintf(buf, "%s.%s.%s-%s", MDT_MODEL, SVR_MODEL, DTG_MODEL, SW_VERSION);
	sprintf(buf, "NEO-W200K.V1.00");
	strcpy(buf_out, buf);
	return buf_out;
#endif
	return KT_FOTA_VER;
}

