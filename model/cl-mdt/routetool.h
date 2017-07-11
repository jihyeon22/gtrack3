#ifndef __UTIL_routetool_H__
#define __UTIL_routetool_H__



#define CFG_IDX__VPN_ID			0
#define CFG_IDX__VPN_PASS		1
#define CFG_IDX__VPN_IP			2
#define CFG_IDX__VPN_PORT		3
#define CFG_IDX__VPN_SITE_OPT	7
#define CFG_IDX__VPN_MAX		8

#define CFG_IDX__PPP_IP			5
#define CFG_IDX__PPP_MAX		6


typedef struct VPN_SETTING_INFO
{
	char vpn_id[32];
	char vpn_pw[32];
	char vpn_ip[64];
	int port;
	char site_to_site[32];
	char ppp_target_ip[64];
}VPN_SETTING_INFO_T;


#define FILE_NAME_PPP_OPTION		"/etc/ppp/options."
//#define FILE_NAME_PPP_SMD28_OPTION_BAK	"/etc/ppp/options.ttyMSM.bak"

int routetool_get_ppp_source_ip(const char* port, char* ip_addr);
int routetool_get_ppp_target_ip(const char* port, char* ip_addr);

int routetool_save_ppp_target_ip(const char* source_port, char* ip);

void set_rout_table_port_forward(const char* source_port, char* source_interface);

#define PORT_FORWARD_SCRIPT		"/tmp/route.portfrd.sh"

#endif
