/**
 * @file    webtool.c
 * @date   	2013-04-24
 * @author 	lonycell@gmail.com
 * @brief  	Classes that help with network access, beyond the normal MDS.net.* APIs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ether.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <util/tools.h>

#include "routetool.h"
#include "logd/logd_rpc.h"

#define LOG_TARGET eSVC_MODEL


/*
/etc/ppp/options.ttyMSM

-detach
115200
modem
asyncmap 0
noauth
192.168.38.2:192.168.38.3
proxyarp
ms-dns 168.126.63.1
ms-dns 8.8.8.8

*/
int strlen_without_cr(const char *s)
{
    int cnt = 0;

    while (*s)
    {
        //printf("strlen [%c]\r\n" ,*s);
        if ( ( *s != '\r' ) && ( *s != '\n' ) && ( *s != ' ' ) )
            cnt++;
        s++;
    }
    //printf("strlen count [%d]\r\n" ,cnt);
    return cnt;
}


int routetool_save_ppp_target_ip(const char* source_port, char* ip)
{
	char write_buff[512] = {0,};
	char option_file[128] = {0,};
	char option_file_bak[128] = {0,};

	char *tr;
	char token[] = ".";
	
	// xxx.xxx.xxx.xxx
	int ip_buff[4] = {0,};
	
	int i = 0;
	int cmd_idx = 0;
	int cnt = 0;
	
	printf("_save_vpn_setting start\r\n");
	
	tr = strtok(ip, token);
	
	for( i = 0 ; i < 4 ; i++ )
	{
		if (tr == NULL)
		{
			break;
		}
		else
		{
			ip_buff[cmd_idx] = atoi(tr);
			cmd_idx ++;
		}
		tr = strtok(NULL, token);
	}
	
	
	printf("_save_vpn_setting start\r\n");
	
	LOGI(LOG_TARGET, "%s : set ppp ip ==> %d.%d.%d.%d ", __func__, ip_buff[0], ip_buff[1], ip_buff[2], ip_buff[3]);
	
	cnt += sprintf(write_buff + cnt, "-detach\r\n");
	cnt += sprintf(write_buff + cnt, "115200\r\n");
	cnt += sprintf(write_buff + cnt, "modem\r\n");
	cnt += sprintf(write_buff + cnt, "asyncmap 0\r\n");
	cnt += sprintf(write_buff + cnt, "noauth\r\n");
	cnt += sprintf(write_buff + cnt, "%d.%d.%d.%d:%d.%d.%d.%d\r\n", ip_buff[0], ip_buff[1], ip_buff[2], ip_buff[3]-1, ip_buff[0], ip_buff[1], ip_buff[2], ip_buff[3]);
	cnt += sprintf(write_buff + cnt, "proxyarp\r\n");
	cnt += sprintf(write_buff + cnt, "ms-dns 168.126.63.1\r\n");
	cnt += sprintf(write_buff + cnt, "ms-dns 8.8.8.8\r\n");
	
	// 먼저 백업본을 만들고...
	sprintf(option_file, "%s%s", FILE_NAME_PPP_OPTION, source_port);
	sprintf(option_file_bak, "%s%s.bak", FILE_NAME_PPP_OPTION, source_port);

	tools_cp(option_file, option_file_bak, 1);
	
	// 삭제하고...
	remove(option_file);
	
	// 라이팅한다.
	tools_write_data(option_file, write_buff, cnt, 0);
	
	printf("%s\r\n",write_buff);

	return 0;
}


int routetool_get_ppp_source_ip(const char* source_port, char* ip_addr)
{
	char read_buff[512] = {0,};
	char option_file[128] = {0,};
	
	int cmd_idx = 0 ;
	int i = 0;
	
	char *tr;
	char token[ ] = "\r\n";
	
	memset(read_buff, 0x00, 512);
	
	sprintf(option_file, "%s%s", FILE_NAME_PPP_OPTION, source_port);
	
	tools_read_data(option_file, read_buff, 512);
	

	/*
	printf(" FILE_NAME_PPP_OPTION read ======\r\n");
	printf("%s\r\n",read_buff);
	printf("=================================\r\n");
	*/
	
	tr = strtok(read_buff, token);
	
	for( i = 0 ; i < CFG_IDX__PPP_MAX ; i++ )
	{
		if (tr == NULL)
		{
			break;
		}
		else
		{
			if ( strlen_without_cr(tr) <= 0 )
				continue;

			switch(cmd_idx++)
			{
				case CFG_IDX__PPP_IP:
				{
					char* find_str = NULL;
					
					find_str = strstr(tr, ":");
					if ( find_str != NULL)
						strncpy(ip_addr, tr, strlen(tr) - strlen(find_str) );
					break;
				}
				default:
					;

			}
			
			
		}
		tr = strtok(NULL, token);
	}
	
	return 0;
}



int routetool_get_ppp_target_ip(const char* source_port, char* ip_addr)
{
	char read_buff[512] = {0,};
	char option_file[128] = {0,};
	
	int cmd_idx = 0 ;
	int i = 0;
	
	char *tr;
	char token[ ] = "\r\n";
	
	memset(read_buff, 0x00, 512);
	
	sprintf(option_file, "%s%s", FILE_NAME_PPP_OPTION, source_port);
	
	tools_read_data(option_file, read_buff, 512);
	

	/*
	printf(" FILE_NAME_PPP_OPTION read ======\r\n");
	printf("%s\r\n",read_buff);
	printf("=================================\r\n");
	*/
	
	tr = strtok(read_buff, token);
	
	for( i = 0 ; i < CFG_IDX__PPP_MAX ; i++ )
	{
		if (tr == NULL)
		{
			break;
		}
		else
		{
			if ( strlen_without_cr(tr) <= 0 )
				continue;

			switch(cmd_idx++)
			{
				case CFG_IDX__PPP_IP:
				{
					char* find_str = NULL;
					
					find_str = strstr(tr, ":");
					if ( find_str != NULL)
						strncpy(ip_addr, tr + strlen(find_str), strlen(tr) - strlen(find_str) );
					break;
				}
				default:
					;

			}
			
			
		}
		tr = strtok(NULL, token);
	}
	
	return 0;
}



void set_rout_table_port_forward(const char* source_port, char* source_interface)
{
	char write_buff[1024];

	char option_file[128] = {0,};
	char option_file_bak[128] = {0,};
	
	char iptables_bin_path[] = "/usr/sbin/iptables";
	char iptables_extif[] ="ppp0";
	char* iptabbles_intif;
	
	iptabbles_intif = source_interface;
	
	int cnt = 0;
	int ret = 0;
	
	char target_ip[60] = {0,};
	
	tools_write_procfs("1", "/proc/sys/net/ipv4/ip_forward");
	tools_write_procfs("1", "/proc/sys/net/ipv4/ip_dynaddr");

	sprintf(option_file, "%s%s", FILE_NAME_PPP_OPTION, source_port);
	sprintf(option_file_bak, "%s%s.bak", FILE_NAME_PPP_OPTION, source_port);
	
	routetool_get_ppp_source_ip(source_port, target_ip);
	
	// 1. remove script file...
	remove(PORT_FORWARD_SCRIPT);
	 
	// 2. write script
	cnt += sprintf(write_buff + cnt, "EXTIF=\"%s\"\n", iptables_extif);
	cnt += sprintf(write_buff + cnt, "INTIF=\"%s\"\n", iptabbles_intif);
	cnt += sprintf(write_buff + cnt, "IPTABLES_BIN_PATH=\"%s\"\n" , iptables_bin_path);
	cnt += sprintf(write_buff + cnt, "/sbin/modprobe ip_tables\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -P INPUT ACCEPT\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -F INPUT\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -P OUTPUT ACCEPT\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -F OUTPUT\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -P FORWARD DROP\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -F FORWARD\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -t nat -F\n");
	cnt += sprintf(write_buff + cnt, "\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -A FORWARD -i $EXTIF -o $INTIF -j ACCEPT\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -A FORWARD -i $INTIF -o $EXTIF -j ACCEPT\n");
	cnt += sprintf(write_buff + cnt, "\n");
	cnt += sprintf(write_buff + cnt, "$IPTABLES_BIN_PATH -t nat -A POSTROUTING -o $EXTIF -s %s/24 -j MASQUERADE\n",target_ip);
	
	printf("write_buff [[ %s]]\r\n", write_buff);
	tools_write_data(PORT_FORWARD_SCRIPT, write_buff, cnt, 0);

	// 3. set run permission
	memset(write_buff, 0x00, 1024);
	sprintf(write_buff, "chmod 775 %s", PORT_FORWARD_SCRIPT);
	ret = system(write_buff);
	
	// 4. run...
	ret = system(PORT_FORWARD_SCRIPT);
	
}

