#include <stdio.h>
#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <iniparser.h>
#include <dictionary.h>

#include <zlib.h>
#include <zconf.h>

#include "gmmp_utill.h"
#include "gmmp_manager.h"

#ifndef TACOC_STANDALONE
#define CONF_INI "/system/mds/system/bin/gmmp.ini"
#else
#define CONF_INI "gmmp.ini"
#endif

static unsigned char g_encode_buf[MAX_BUFFER_SIZE] = {0, };
unsigned long g_encode_len = 0;
int gzlib_compress(unsigned char *buf_src, unsigned int orgsize)
{
	int ret;
	
	memset(g_encode_buf, 0x00, sizeof(MAX_BUFFER_SIZE));

	g_encode_len = MAX_BUFFER_SIZE;
	ret = compress(g_encode_buf, &g_encode_len, buf_src, orgsize);
	if(ret < 0)
		return -1;

	return 0;
}

unsigned char *get_encode_buffer()
{
	return g_encode_buf;
}

unsigned long get_encode_length()
{
	return g_encode_len;
}
int is_file_exist(char *filename)
{
	int ret = 0;
	ret = open(filename, O_RDONLY);
	if(ret <= 0)
		return -1;

	close(ret);
	
	return 0;
}

dictionary *ini;
int load_ini_file()
{
	if(is_file_exist(CONF_INI)==-1)
	{
		FILE *temp;
		temp=fopen(CONF_INI,"w");
		fprintf(temp,"[gmmp]\n");
		fclose(temp);
	}
	ini = iniparser_load(CONF_INI);
	read_ini_initdata();
	read_ini_config();
	//if(is_file_exist(CONF_INI)==-1)

	write_ini_initdata();
	write_ini_config();
	return 0;
}

void write_ini_initdata()
{
	char temp[10]={0,};
	iniparser_set(ini, "GMMP:ServerIP", gmmp_get_server_ip());
	sprintf(temp,"%d",gmmp_get_server_port());
	iniparser_set(ini, "GMMP:ServerPort", temp);
	iniparser_set(ini, "GMMP:DomainID", gmmp_get_domain_id());
	iniparser_set(ini, "GMMP:ManufactureID", gmmp_get_manufacture_id());
	iniparser_set(ini, "GMMP:ProfilePath", gmmp_get_profile_path());
	iniparser_store(ini,CONF_INI);
}

int read_ini_initdata()
{
	gmmp_set_server_ip(iniparser_getstring(ini, "GMMP:ServerIP", "211.115.15.213"));
	gmmp_set_server_port(iniparser_getint(ini, "GMMP:ServerPort", 31051));
	gmmp_set_domain_id(iniparser_getstring(ini, "GMMP:DomainID", "neom2msvc"));
	gmmp_set_manufacture_id(iniparser_getstring(ini, "GMMP:ManufactureID", "MDSTECHNOLOGY"));
	gmmp_set_profile_path(iniparser_getstring(ini, "GMMP:ProfilePath", "/system/mds/system/bin/m2m_gateway_profile.dat"));
	return 0;
}

int read_ini_config(void)
{
	if(is_file_exist(CONF_INI) < 0)
	{
		return -1;
	}
	ini = iniparser_load(CONF_INI);
	gmmp_set_delivery_state(iniparser_getint(ini, "GMMP:DeliveryOn", 1));
	gmmp_set_transaction_id(iniparser_getint(ini, "GMMP:TransactionID", 0));
	return 0;
}

int write_ini_config(void)
{
	char temp[10]={0,};
	sprintf(temp,"%d",gmmp_get_delivery_state());
	iniparser_set(ini, "GMMP:DeliveryOn", temp);
	sprintf(temp,"%d",gmmp_get_transaction_id_not_increase());
	iniparser_set(ini, "GMMP:TransactionID", temp);
	iniparser_store(ini, CONF_INI) ;
	return 0;
}

int write_ini_tid(void)
{
	char temp[10]={0,};
	sprintf(temp,"%d",gmmp_get_delivery_state());
	iniparser_set(ini, "GMMP:DeliveryOn", temp);
	sprintf(temp,"%d",gmmp_get_transaction_id_not_increase());
	iniparser_set(ini, "GMMP:TransactionID", temp);
	iniparser_store(ini, CONF_INI) ;
	return 0;
}
