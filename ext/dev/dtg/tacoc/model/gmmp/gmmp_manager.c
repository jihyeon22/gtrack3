<<<<<<< HEAD
#include <stdio.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>

#include "gmmp_manager.h"

static GMMP_DATA_MANAGER g_gmmp_data_mng;

void gmmp_manager_init()
{
	memset(&g_gmmp_data_mng, 0x00, sizeof(GMMP_DATA_MANAGER));
}

void gmmp_set_server_ip(char *ip)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.server_ip, ip);
}
char* gmmp_get_server_ip()
{
	return g_gmmp_data_mng.device_conf_dat.server_ip;
}

void gmmp_set_server_port(int port)
{
	g_gmmp_data_mng.device_conf_dat.server_port = port;
}
int gmmp_get_server_port()
{
	return g_gmmp_data_mng.device_conf_dat.server_port;
}

void gmmp_set_domain_id(char *domain_id)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.DomainCode, domain_id);
}
char *gmmp_get_domain_id()
{
	return g_gmmp_data_mng.device_conf_dat.DomainCode;
}

void gmmp_set_manufacture_id(char *manufactureID)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.manufacture_id, manufactureID);
}
char *gmmp_get_manufacture_id()
{
	return g_gmmp_data_mng.device_conf_dat.manufacture_id;
}

void gmmp_set_profile_path(char *profile_path)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.profile_path, profile_path);
}
char *gmmp_get_profile_path()
{
	return g_gmmp_data_mng.device_conf_dat.profile_path;
}

void gmmp_set_auth_id(char *auth_id)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.AuthID, auth_id);
}
char *gmmp_get_auth_id()
{
	return g_gmmp_data_mng.device_conf_dat.AuthID;
}

void gmmp_set_auth_key(char *auth_key)
{
	strcpy(g_gmmp_data_mng.device_profile_dat.AuthKey, auth_key);
}
char *gmmp_get_auth_key()
{
	return g_gmmp_data_mng.device_profile_dat.AuthKey;
}


void gmmp_set_network_time_out(int sec)
{
	g_gmmp_data_mng.device_profile_dat.ResponseTimerout = sec;
}
int gmmp_get_network_time_out()
{
	if(g_gmmp_data_mng.device_profile_dat.ResponseTimerout <= 0) //0초 이하라면 미세팅으로 default 30초로 동작하도록 한다.
		return DEFAULT_NETWORK_TIME;

	return g_gmmp_data_mng.device_profile_dat.ResponseTimerout;
}

void gmmp_set_gw_id(char *gw_id)
{
	strcpy(g_gmmp_data_mng.device_profile_dat.GwId, gw_id);
}
char* gmmp_get_gw_id()
{
	return g_gmmp_data_mng.device_profile_dat.GwId;
}

void gmmp_set_reporting_period(int reporting_period)
{
	g_gmmp_data_mng.device_profile_dat.ReportPeriod = reporting_period;
}
int gmmp_get_reporting_period()
{
	return g_gmmp_data_mng.device_profile_dat.ReportPeriod;
}

void gmmp_set_transaction_id(int tid)
{
	if(tid > 99999 || tid < 0)
		g_gmmp_data_mng.device_conf_dat.transaction_id = 0;
	g_gmmp_data_mng.device_conf_dat.transaction_id = tid;
}

int gmmp_get_transaction_id()
{
	if(g_gmmp_data_mng.device_conf_dat.transaction_id > 99999){
		g_gmmp_data_mng.device_conf_dat.transaction_id = 0;
	}
	return g_gmmp_data_mng.device_conf_dat.transaction_id++;
}

int gmmp_get_transaction_id_not_increase()
{
	if(g_gmmp_data_mng.device_conf_dat.transaction_id > 99999){
		g_gmmp_data_mng.device_conf_dat.transaction_id = 0;
	}
	return g_gmmp_data_mng.device_conf_dat.transaction_id;
}

void gmmp_set_delivery_state(int value)
{
	g_gmmp_data_mng.device_conf_dat.delivery_on = value;
}

int gmmp_get_delivery_state()
{
	return g_gmmp_data_mng.device_conf_dat.delivery_on;
}

int gmmp_get_time_stamp()
{
	struct timeval tv;
	if(gettimeofday(&tv,NULL)){
		return 0;
	}
	return tv.tv_sec;
}

int write_profile_contents(DEVICE_PROFILE_INFO profile, char *profile_path)
{
	int err = 0;
	int write_bytes;
	
	int fd = open(profile_path, O_CREAT|O_WRONLY, S_IRWXU);
	if(fd <= 0) {
		err = -1;
		goto FINISH;
	}

	write_bytes = write(fd, &profile, sizeof(DEVICE_PROFILE_INFO));
	if(write_bytes != sizeof(DEVICE_PROFILE_INFO)) {
		err = -2;
		goto FINISH;
	}

	memcpy(&g_gmmp_data_mng.device_profile_dat, &profile, sizeof(DEVICE_PROFILE_INFO));

FINISH:
	if(fd > 0)
		close(fd);

	return 0;
}
int read_profile_contents(char *profile_path)
{
	DEVICE_PROFILE_INFO tmp;
	int err = 0;
	int read_bytes;

	int fd = open(profile_path, O_RDONLY);
	if(fd <= 0) {
		err = -1;
		goto FINISH;
	}

	read_bytes = read(fd, &tmp, sizeof(DEVICE_PROFILE_INFO));
	if(read_bytes != sizeof(DEVICE_PROFILE_INFO)) {
		err = -2;
		goto FINISH;
	}

	memcpy( &g_gmmp_data_mng.device_profile_dat,
			&tmp, 
			sizeof(DEVICE_PROFILE_INFO));


FINISH:
	if(fd > 0)
		close(fd);

	return 0;
=======
#include <stdio.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <time.h>
#include <sys/time.h>

#include "gmmp_manager.h"

static GMMP_DATA_MANAGER g_gmmp_data_mng;

void gmmp_manager_init()
{
	memset(&g_gmmp_data_mng, 0x00, sizeof(GMMP_DATA_MANAGER));
}

void gmmp_set_server_ip(char *ip)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.server_ip, ip);
}
char* gmmp_get_server_ip()
{
	return g_gmmp_data_mng.device_conf_dat.server_ip;
}

void gmmp_set_server_port(int port)
{
	g_gmmp_data_mng.device_conf_dat.server_port = port;
}
int gmmp_get_server_port()
{
	return g_gmmp_data_mng.device_conf_dat.server_port;
}

void gmmp_set_domain_id(char *domain_id)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.DomainCode, domain_id);
}
char *gmmp_get_domain_id()
{
	return g_gmmp_data_mng.device_conf_dat.DomainCode;
}

void gmmp_set_manufacture_id(char *manufactureID)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.manufacture_id, manufactureID);
}
char *gmmp_get_manufacture_id()
{
	return g_gmmp_data_mng.device_conf_dat.manufacture_id;
}

void gmmp_set_profile_path(char *profile_path)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.profile_path, profile_path);
}
char *gmmp_get_profile_path()
{
	return g_gmmp_data_mng.device_conf_dat.profile_path;
}

void gmmp_set_auth_id(char *auth_id)
{
	strcpy(g_gmmp_data_mng.device_conf_dat.AuthID, auth_id);
}
char *gmmp_get_auth_id()
{
	return g_gmmp_data_mng.device_conf_dat.AuthID;
}

void gmmp_set_auth_key(char *auth_key)
{
	strcpy(g_gmmp_data_mng.device_profile_dat.AuthKey, auth_key);
}
char *gmmp_get_auth_key()
{
	return g_gmmp_data_mng.device_profile_dat.AuthKey;
}


void gmmp_set_network_time_out(int sec)
{
	g_gmmp_data_mng.device_profile_dat.ResponseTimerout = sec;
}
int gmmp_get_network_time_out()
{
	if(g_gmmp_data_mng.device_profile_dat.ResponseTimerout <= 0) //0초 이하라면 미세팅으로 default 30초로 동작하도록 한다.
		return DEFAULT_NETWORK_TIME;

	return g_gmmp_data_mng.device_profile_dat.ResponseTimerout;
}

void gmmp_set_gw_id(char *gw_id)
{
	strcpy(g_gmmp_data_mng.device_profile_dat.GwId, gw_id);
}
char* gmmp_get_gw_id()
{
	return g_gmmp_data_mng.device_profile_dat.GwId;
}

void gmmp_set_reporting_period(int reporting_period)
{
	g_gmmp_data_mng.device_profile_dat.ReportPeriod = reporting_period;
}
int gmmp_get_reporting_period()
{
	return g_gmmp_data_mng.device_profile_dat.ReportPeriod;
}

void gmmp_set_transaction_id(int tid)
{
	if(tid > 99999 || tid < 0)
		g_gmmp_data_mng.device_conf_dat.transaction_id = 0;
	g_gmmp_data_mng.device_conf_dat.transaction_id = tid;
}

int gmmp_get_transaction_id()
{
	if(g_gmmp_data_mng.device_conf_dat.transaction_id > 99999){
		g_gmmp_data_mng.device_conf_dat.transaction_id = 0;
	}
	return g_gmmp_data_mng.device_conf_dat.transaction_id++;
}

int gmmp_get_transaction_id_not_increase()
{
	if(g_gmmp_data_mng.device_conf_dat.transaction_id > 99999){
		g_gmmp_data_mng.device_conf_dat.transaction_id = 0;
	}
	return g_gmmp_data_mng.device_conf_dat.transaction_id;
}

void gmmp_set_delivery_state(int value)
{
	g_gmmp_data_mng.device_conf_dat.delivery_on = value;
}

int gmmp_get_delivery_state()
{
	return g_gmmp_data_mng.device_conf_dat.delivery_on;
}

int gmmp_get_time_stamp()
{
	struct timeval tv;
	if(gettimeofday(&tv,NULL)){
		return 0;
	}
	return tv.tv_sec;
}

int write_profile_contents(DEVICE_PROFILE_INFO profile, char *profile_path)
{
	int err = 0;
	int write_bytes;
	
	int fd = open(profile_path, O_CREAT|O_WRONLY, S_IRWXU);
	if(fd <= 0) {
		err = -1;
		goto FINISH;
	}

	write_bytes = write(fd, &profile, sizeof(DEVICE_PROFILE_INFO));
	if(write_bytes != sizeof(DEVICE_PROFILE_INFO)) {
		err = -2;
		goto FINISH;
	}

	memcpy(&g_gmmp_data_mng.device_profile_dat, &profile, sizeof(DEVICE_PROFILE_INFO));

FINISH:
	if(fd > 0)
		close(fd);

	return 0;
}
int read_profile_contents(char *profile_path)
{
	DEVICE_PROFILE_INFO tmp;
	int err = 0;
	int read_bytes;

	int fd = open(profile_path, O_RDONLY);
	if(fd <= 0) {
		err = -1;
		goto FINISH;
	}

	read_bytes = read(fd, &tmp, sizeof(DEVICE_PROFILE_INFO));
	if(read_bytes != sizeof(DEVICE_PROFILE_INFO)) {
		err = -2;
		goto FINISH;
	}

	memcpy( &g_gmmp_data_mng.device_profile_dat,
			&tmp, 
			sizeof(DEVICE_PROFILE_INFO));


FINISH:
	if(fd > 0)
		close(fd);

	return 0;
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}