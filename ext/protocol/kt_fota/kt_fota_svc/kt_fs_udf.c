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


#define _AT_DEV_FILE			"/dev/smd25"
#define _AT_MAX_BUFF_SIZE		512
#define _AT_MAX_RETRY_WRITE		3
#define _AT_MAX_WAIT_READ_SEC	1

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
	fd_set reads;
	struct timeval tout;
	int result = 0;

	FD_ZERO(&reads);
	FD_SET(fd, &reads);

	while (1) {
		tout.tv_sec = ftime;
		tout.tv_usec = 0;
		result = select(fd + 1, &reads, 0, 0, &tout);
		if(result <= 0) //time out & select error
			return -1;
		
		if ( read(fd, buf, buf_len) <= 0)
			return -1;

		break; //success
	}

	return 0;
}


char *dummy_ktdevstat(char *buf_out) 
{
	int i;
	char tmp_buf[128];
	char *data[] = {"0","0","0","0","0","0","0","0","0","0","0","0","0","0",KT_FOTA_MODEL,"0","0","0", NULL};
	sprintf(buf_out, "[");

	i = 0;
	while(1) {
		
		sprintf(tmp_buf, "%c%s%c", 0x22, data[i], 0x22);
		strcat(buf_out, tmp_buf);
		i += 1;
		if(data[i] == NULL) {
			strcat(buf_out, "]");
			break;
		}
		else {
			strcat(buf_out, ", ");
		}

	}
	return buf_out;
}

int at_get_ktdevstat(char *stat_buf, int buf_len)
{
	int fd;
	int ret = -1;
	int retry = 0;
	char *pAtcmd = NULL;
	char *pAT_KT_DEV_STAT = NULL;
	char buffer[_AT_MAX_BUFF_SIZE] = {0,};

	if(stat_buf == NULL) {
		printf("<atd> %s : %d, buffer NULL error", __func__, __LINE__);
		return -1;
	}

	fd = open(_AT_DEV_FILE, O_RDWR | O_NONBLOCK);
	if(fd < 0)
	{
		printf("<atd> %s : %d, AT Channel Open Error errno[%d]", __func__, __LINE__, errno);
		return -1;
	}

	retry = 0;
	while(retry++ < _AT_MAX_RETRY_WRITE) 
	{
		if(retry > 1)
			sleep(1);

		ret = write(fd, "ATKTDEVSTAT?\r\n", sizeof("ATKTDEVSTAT?\r\n"));
		if(ret != sizeof("ATKTDEVSTAT?\r\n"))
			continue;

		memset(buffer, 0, sizeof(buffer));
		if(_wait_read(fd, buffer, _AT_MAX_BUFF_SIZE, _AT_MAX_WAIT_READ_SEC) < 0)
			continue;

		if((pAtcmd = strstr(buffer, "KTDEVSTAT: \"Modem_Qinfo\":")) == NULL)
			continue;

		if((pAtcmd = strstr(buffer, "[")) == NULL)
			continue;

		pAT_KT_DEV_STAT = pAtcmd;

		if((pAtcmd = strstr(buffer, "]")) == NULL)
			continue;

		*(pAtcmd + 1) = 0x00;
		break; //success
	}
	close(fd);

	if(retry >= _AT_MAX_RETRY_WRITE) { //fail
		return -1;
	}

	memset(stat_buf, 0x00, sizeof(buf_len));
	if(pAT_KT_DEV_STAT != NULL)
		strncpy(stat_buf, pAT_KT_DEV_STAT, buf_len-1);
	return 0;
}

char *kt_srv_get_modem_qty(char *buf_out, int buf_len) 
{
	if(at_get_ktdevstat(buf_out, buf_len) < 0)
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
	if(mem < 0) mem = 9; //default 9MB
	else mem /= 1024;

	percent = 100.0 - (float)mem/10.0*100.0;
	
	sprintf(buf_out, "%d", (int)percent);
	
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

