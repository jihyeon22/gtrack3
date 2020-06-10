<<<<<<< HEAD
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

#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#endif
#include <at/at_util.h>
#include <board/power.h>
#include <board/battery.h>
#include "nettool.h"
#include "webtool.h"
#include "webdm.h"
#include <logd_rpc.h>

extern int sock_webdm;

/*===========================================================================================*/
int webdm_register(const char* imei, const char* imsi, const char* ip, const int port)
{
	configurationBase_t * conf = NULL;
	char body[WEBTOOL_MAX_SNDBODY];
	memset(body, 0x00, WEBTOOL_MAX_SNDBODY);

	char *p = (char*)body;
	p += sprintf(p, "<IoThing xmlns=\"http://schemas.datacontract.org/2004/07/OpenM2M\">");
	p += sprintf(p, "<imei>%s</imei>", imei);
	p += sprintf(p, "<imsi>%s</imsi>", imsi);
	p += sprintf(p, "<ip>%s</ip>", ip);
	p += sprintf(p, "<port>%d</port>", port);
	p += sprintf(p, "</IoThing>");

	httpRequest_t query;
	memset(&query, 0x00, sizeof(httpRequest_t));

	conf = get_config_base();

	query.ContentLength = strlen(body);
	query.pContent = (char*)body;

	query.PortNum = conf->webdm.port;
	sprintf(query.ContentType, "%s", conf->webdm.bodytype);
	sprintf(query.Method, "%s", conf->webdm.method);
	//sprintf(query.HostAddress, "%s", conf->webdm.ip);
	sprintf(query.HostAddress, "%s", conf->webdm.ip_addr);
//	printf ("query.HostAddress is [%s]\r\n",query.HostAddress);
	sprintf(query.Destination, "%s", conf->webdm.service);

	return webtool_send_Request((const httpRequest_t*)&query);
}

int webdm_status(const char* imei, const char* imsi, const char* ip, const int port, const int no_event, const float lat, const float lon, const int ingn, const int dist)
{
	configurationBase_t * conf = NULL;
	conf = get_config_base();

	char body[WEBTOOL_MAX_SNDBODY];
	memset(body, 0x00, WEBTOOL_MAX_SNDBODY);

	char *p = (char*)body;

	printf("openm2m_status %f %f %d %d\n", lat, lon, ingn, no_event);

	p += sprintf(p, "<Status xmlns=\"http://schemas.datacontract.org/2004/07/OpenM2M\">");
	//p += sprintf(p, "<angle>%d</angle>", angle);
	p += sprintf(p, "<distance>%d</distance>", dist);
	p += sprintf(p, "<eventType>%d</eventType>", no_event);
	p += sprintf(p, "<ignition>%d</ignition>", ingn);
	p += sprintf(p, "<imsi>%s</imsi>", imsi);
	p += sprintf(p, "<latitude>%f</latitude>", lat);
	p += sprintf(p, "<longitude>%f</longitude>", lon);
	//p += sprintf(p, "<speed>%f</speed>", speed);
	p += sprintf(p, "</Status>");

	httpRequest_t query;
	memset(&query, 0x00, sizeof(httpRequest_t));

	query.ContentLength = strlen(body);
	query.pContent = (char*)body;

	query.PortNum = conf->webdm.port;
	sprintf(query.ContentType, "%s", conf->webdm.bodytype);
	sprintf(query.Method, "%s", conf->webdm.method);
	//sprintf(query.HostAddress, "%s", conf->webdm.ip);
	sprintf(query.HostAddress, "%s", conf->webdm.ip_addr);
//	printf ("query.HostAddress is [%s]\r\n",query.HostAddress);
	sprintf(query.Destination, "%s", conf->webdm.service_status);

	return webtool_send_Request((const httpRequest_t*)&query);
}

int webdm_log(const char* phonenum, const char* imei, const char* model, const int  type, const char* log)
{
	configurationBase_t * conf = NULL;
	conf = get_config_base();

	char body[WEBTOOL_MAX_SNDBODY];
	memset(body, 0x00, WEBTOOL_MAX_SNDBODY);

	char *p = (char*)body;

	p += sprintf(p, "<LogData xmlns=\"http://schemas.datacontract.org/2004/07/OpenM2M\">");
	p += sprintf(p, "<imei>%s</imei>", imei);
	p += sprintf(p, "<log>%s</log>", log);
	p += sprintf(p, "<model>%s</model>", model);
	p += sprintf(p, "<phonenum>%s</phonenum>", phonenum);
	p += sprintf(p, "<type>%d</type>", type);
	p += sprintf(p, "</LogData>");

	httpRequest_t query;
	memset(&query, 0x00, sizeof(httpRequest_t));

	query.ContentLength = strlen(body);
	query.pContent = (char*)body;

	query.PortNum = conf->webdm.port;
	sprintf(query.ContentType, "%s", conf->webdm.bodytype);
	sprintf(query.Method, "%s", conf->webdm.method);
	//sprintf(query.HostAddress, "%s", conf->webdm.ip);
	sprintf(query.HostAddress, "%s", conf->webdm.ip_addr);
//	printf ("query.HostAddress is [%s]\r\n",query.HostAddress);
	sprintf(query.Destination, "%s", "log/");

	return webtool_send_Request((const httpRequest_t*)&query);
}


=======
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

#include <base/config.h>
#ifdef USE_GPS_MODEL
#include <base/gpstool.h>
#endif
#include <at/at_util.h>
#include <board/power.h>
#include <board/battery.h>
#include "nettool.h"
#include "webtool.h"
#include "webdm.h"
#include <logd_rpc.h>

extern int sock_webdm;

/*===========================================================================================*/
int webdm_register(const char* imei, const char* imsi, const char* ip, const int port)
{
	configurationBase_t * conf = NULL;
	char body[WEBTOOL_MAX_SNDBODY];
	memset(body, 0x00, WEBTOOL_MAX_SNDBODY);

	char *p = (char*)body;
	p += sprintf(p, "<IoThing xmlns=\"http://schemas.datacontract.org/2004/07/OpenM2M\">");
	p += sprintf(p, "<imei>%s</imei>", imei);
	p += sprintf(p, "<imsi>%s</imsi>", imsi);
	p += sprintf(p, "<ip>%s</ip>", ip);
	p += sprintf(p, "<port>%d</port>", port);
	p += sprintf(p, "</IoThing>");

	httpRequest_t query;
	memset(&query, 0x00, sizeof(httpRequest_t));

	conf = get_config_base();

	query.ContentLength = strlen(body);
	query.pContent = (char*)body;

	query.PortNum = conf->webdm.port;
	sprintf(query.ContentType, "%s", conf->webdm.bodytype);
	sprintf(query.Method, "%s", conf->webdm.method);
	//sprintf(query.HostAddress, "%s", conf->webdm.ip);
	sprintf(query.HostAddress, "%s", conf->webdm.ip_addr);
//	printf ("query.HostAddress is [%s]\r\n",query.HostAddress);
	sprintf(query.Destination, "%s", conf->webdm.service);

	return webtool_send_Request((const httpRequest_t*)&query);
}

int webdm_status(const char* imei, const char* imsi, const char* ip, const int port, const int no_event, const float lat, const float lon, const int ingn, const int dist)
{
	configurationBase_t * conf = NULL;
	conf = get_config_base();

	char body[WEBTOOL_MAX_SNDBODY];
	memset(body, 0x00, WEBTOOL_MAX_SNDBODY);

	char *p = (char*)body;

	printf("openm2m_status %f %f %d %d\n", lat, lon, ingn, no_event);

	p += sprintf(p, "<Status xmlns=\"http://schemas.datacontract.org/2004/07/OpenM2M\">");
	//p += sprintf(p, "<angle>%d</angle>", angle);
	p += sprintf(p, "<distance>%d</distance>", dist);
	p += sprintf(p, "<eventType>%d</eventType>", no_event);
	p += sprintf(p, "<ignition>%d</ignition>", ingn);
	p += sprintf(p, "<imsi>%s</imsi>", imsi);
	p += sprintf(p, "<latitude>%f</latitude>", lat);
	p += sprintf(p, "<longitude>%f</longitude>", lon);
	//p += sprintf(p, "<speed>%f</speed>", speed);
	p += sprintf(p, "</Status>");

	httpRequest_t query;
	memset(&query, 0x00, sizeof(httpRequest_t));

	query.ContentLength = strlen(body);
	query.pContent = (char*)body;

	query.PortNum = conf->webdm.port;
	sprintf(query.ContentType, "%s", conf->webdm.bodytype);
	sprintf(query.Method, "%s", conf->webdm.method);
	//sprintf(query.HostAddress, "%s", conf->webdm.ip);
	sprintf(query.HostAddress, "%s", conf->webdm.ip_addr);
//	printf ("query.HostAddress is [%s]\r\n",query.HostAddress);
	sprintf(query.Destination, "%s", conf->webdm.service_status);

	return webtool_send_Request((const httpRequest_t*)&query);
}

int webdm_log(const char* phonenum, const char* imei, const char* model, const int  type, const char* log)
{
	configurationBase_t * conf = NULL;
	conf = get_config_base();

	char body[WEBTOOL_MAX_SNDBODY];
	memset(body, 0x00, WEBTOOL_MAX_SNDBODY);

	char *p = (char*)body;

	p += sprintf(p, "<LogData xmlns=\"http://schemas.datacontract.org/2004/07/OpenM2M\">");
	p += sprintf(p, "<imei>%s</imei>", imei);
	p += sprintf(p, "<log>%s</log>", log);
	p += sprintf(p, "<model>%s</model>", model);
	p += sprintf(p, "<phonenum>%s</phonenum>", phonenum);
	p += sprintf(p, "<type>%d</type>", type);
	p += sprintf(p, "</LogData>");

	httpRequest_t query;
	memset(&query, 0x00, sizeof(httpRequest_t));

	query.ContentLength = strlen(body);
	query.pContent = (char*)body;

	query.PortNum = conf->webdm.port;
	sprintf(query.ContentType, "%s", conf->webdm.bodytype);
	sprintf(query.Method, "%s", conf->webdm.method);
	//sprintf(query.HostAddress, "%s", conf->webdm.ip);
	sprintf(query.HostAddress, "%s", conf->webdm.ip_addr);
//	printf ("query.HostAddress is [%s]\r\n",query.HostAddress);
	sprintf(query.Destination, "%s", "log/");

	return webtool_send_Request((const httpRequest_t*)&query);
}


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
