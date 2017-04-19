#ifndef __BASE_KT_FOTA_CONF_H__
#define __BASE_KT_FOTA_CONF_H__
typedef struct
{
	char dm_server_ip[64];
	int dm_port;
	char qty_server_ip[64];
	int qty_port;
	int acc_qty_report;
	int acc_fota_req_report;
}KT_FOTA_SVR_CONF;

void set_kt_fota_qry_report(int num);
int get_kt_fota_qry_report();
void set_kt_fota_req_report(int num);
int get_kt_fota_req_report();
char* get_kt_fota_dm_server_ip_addr();
void set_kt_fota_dm_server_ip_addr(char* ip_addr);
char* get_kt_fota_qty_server_ip_addr();
void set_kt_fota_qty_server_ip_addr(char* ip_addr);
unsigned short get_kt_fota_dm_server_port();
void set_kt_fota_dm_server_port(unsigned short port);
unsigned short get_kt_fota_qty_server_port();
void set_kt_fota_qty_server_port(unsigned short port);
int load_ini_kt_fota_svc_info();
int save_ini_kt_fota_svc_info();

#endif
