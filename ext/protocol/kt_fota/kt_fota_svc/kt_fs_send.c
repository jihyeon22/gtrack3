#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <sys/sysctl.h>
#include <errno.h>
#include <netdb.h>

#include <jansson.h>
#include <logd/logd_rpc.h>

#include "kt_fs_send.h"
#include "kt_fs_udf.h"
#include "kt_fs_parser.h"

#include <include/defines.h>
#include <util/nettool.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_BASE

void make_req_header(char *pheader, char *api, char *auth, char *type, char *addr, int port, int length, char *chset)
{
	char *hdr_format = "POST /SFOTA/%s HTTP/1.1\r\nAuthorization:%s\r\nContent-Type:%s;character-set=%s\r\nHost:%s:%d\r\nContent-Length:%d\r\n\r\n";
	sprintf(pheader, hdr_format, api, auth, type, chset, addr, port, length);
}

int kt_fota_srv_send(KT_Fota_Svc_t api_type, char *post_data, int time_out)
{
	int contents_len;
	char tmp_data[2048];
	int sock;
	struct sockaddr_in remote;
	int ret;
	char *http_recv_dat = NULL;
	char *tr;
	char *temp_bp;
	int rev_count = 0;
	char *ip;
	int port;

	char *api[] = {"ReqFOTA", "ReqQTY", "ReqOnNoti", "ReqOffNoti", "ReqModemQTY"};

	if(nettool_get_state() != DEFINES_MDS_OK) //No PPP Device
	{
		LOGE(LOG_TARGET, "KT_FOTA_SVC, NO PPP Device\n");
		return -1;
	}

	if(api_type < 0 || api_type >= eKT_MAX_FOTA_SVC) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, Unknown API Type Error\n");
		return -2;
	}

	if(api_type == eKT_MODEM_QTY_NOTI || api_type == eKT_DEVICE_QTY_NOTI) {
		ip = kt_qty_srv_get_ip();
		port = kt_qty_srv_get_port();
	}
	else {
		ip = kt_dm_srv_get_ip();
		port = kt_dm_srv_get_port();
	}

	contents_len = strlen(post_data);
	make_req_header(tmp_data, api[api_type], 
		"Basicb2xsZWhtYXBfdXNlcjpvbGxlaG1hcF91c2Vyabc",
		"application/json",
		ip,
		port,
		contents_len,
		"UTF-8");

	LOGD(LOG_TARGET, "KT_FOTA_SVC, header ===> %s\n", tmp_data);
	LOGD(LOG_TARGET, "KT_FOTA_SVC, data ===> %s\n", post_data);


	if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		LOGE(LOG_TARGET, "KT_FOTA_SVC, Can't create TCP socket");
		return -1;
	}


	remote.sin_family = AF_INET;
	ret = nettool_get_host_name(ip);
	if(ret == 0)	{
		fprintf(stderr, "%s is not a valid IP address\n", ip);
		close(sock);
		return -3;
	}
	remote.sin_addr.s_addr = ret;

	remote.sin_port = htons(port);
	if(nettool_connect_timeo(sock, (struct sockaddr *)&remote, sizeof(struct sockaddr), time_out)  < 0) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, Could not connect");
		close(sock);
		return -4;
	}

	if(nettool_send_timedwait(sock, tmp_data, strlen(tmp_data), 0, time_out) <= 0) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, header send error");
		close(sock);
		return -5;
	}

	if(nettool_send_timedwait(sock, post_data, contents_len, 0, time_out) <= 0) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, contents send error");
		close(sock);
		return -5;
	}

	memset(tmp_data, 0, sizeof(tmp_data));

	if( (rev_count = nettool_recv_timedwait(sock, tmp_data, sizeof(tmp_data), 0, time_out)) <= 0)
	{
		LOGE(LOG_TARGET, "KT_FOTA_SVC, contents recv error");
		close(sock);
		return -6;
	}

	if(api_type == eKT_FOTA_REQ || api_type == eKT_DEVICE_ON_NOTI)
	{
		http_recv_dat = strstr(tmp_data, "\r\n\r\n");
		if(http_recv_dat == NULL) 
		{
			LOGE(LOG_TARGET, "KT_FOTA_SVC, http none contents error");
			close(sock);
			return -7;
		}
	}
	
	tr = strtok_r(tmp_data, "\r\n", &temp_bp);
	if(tr == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, http code error = %s", tmp_data);
		close(sock);
		return -8;
	}

	if(strncmp(tr, "HTTP/1.1 200 OK", 15)) 
	{
		LOGE(LOG_TARGET, "KT_FOTA_SVC, http code error = %s\n", tr);
		close(sock);
		return -9;
	}

	if(api_type == eKT_DEVICE_OFF_NOTI || api_type == eKT_DEVICE_QTY_NOTI || api_type == eKT_MODEM_QTY_NOTI)
	{
		//recieve contents data none api
		close(sock);
		return 0;
	}

	if(kt_fota_srv_data_parse(api_type, http_recv_dat) < 0) {
		/*
		printf("1====> %s\n", tmp_data);

		if( (rev_count = nettool_recv_timedwait(sock, tmp_data, sizeof(tmp_data), 0, time_out)) > 0)
		{
			printf("2====> %s\n", tmp_data);
		}
		*/
		close(sock);
		return -10;
	}

	close(sock);
    return 0;
}

int send_device_on_packet(int time_out)
{
	json_t *json = NULL;
	char tmp_buf[256];

	char *post_data = NULL;
	int result = 0;

	json = json_object();
	if(json == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json create object error\n");
		result = -1;
		goto error_finish;
	}

	json_object_set_new(json, "Protocol_Version",	json_string(KT_FOTA_PROTOCOL_VER));
	json_object_set_new(json, "CTN",				json_string(kt_srv_get_cti(tmp_buf)));
	json_object_set_new(json, "Model_Name",			json_string(kt_srv_get_model_name()));
	json_object_set_new(json, "Pkg_Version",		json_string(kt_srv_get_package_version(tmp_buf)));
	json_object_set_new(json, "Custom_Tag",			json_string(kt_srv_get_cust_tag(tmp_buf)));
	json_object_set_new(json, "Echo_Tag",			json_string(""));

	post_data = json_dumps( json, 0 );
	if(post_data == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json dump data NULL error\n");
		result = -3;
		goto error_finish;
	}

	result = kt_fota_srv_send(eKT_DEVICE_ON_NOTI, post_data, time_out);
	if(result == -10) //parse error
		result = kt_fota_srv_send(eKT_DEVICE_ON_NOTI, post_data, time_out);

	if(result < 0) result += -100;



error_finish:
	if(post_data != NULL)
		free(post_data);

	/* free json object */
	if(json != NULL)
		json_decref(json);

	return result;
}

int send_device_off_packet(int time_out)
{
	json_t *json = NULL;
	char tmp_buf[256];

	char *post_data = NULL;
	int result = 0;

	json = json_object();
	if(json == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json create object error\n");
		result = -1;
		goto error_finish;
	}

	json_object_set_new(json, "Protocol_Version",	json_string(KT_FOTA_PROTOCOL_VER));
	json_object_set_new(json, "CTN",				json_string(kt_srv_get_cti(tmp_buf)));
	json_object_set_new(json, "Model_Name",			json_string(kt_srv_get_model_name()));
	json_object_set_new(json, "Custom_Tag",			json_string(kt_srv_get_cust_tag(tmp_buf)));

	post_data = json_dumps( json, 0 );
	if(post_data == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json dump data NULL error\n");
		result = -3;
		goto error_finish;
	}

	result = kt_fota_srv_send(eKT_DEVICE_OFF_NOTI, post_data, time_out);
	if(result < 0) result += -100;


error_finish:
	if(post_data != NULL)
		free(post_data);

	/* free json object */
	if(json != NULL)
		json_decref(json);

	return result;
}


int send_qty_packet(KT_Fota_Req_Mode_t req_type, KT_Fota_Qty_Mode_t qty_mode, int time_out)
{
	char tmp_buf[512];
	json_error_t error;
	json_t *json = NULL;
	json_t *dqty = NULL;
	json_t *mtmp = NULL;
	char *post_data = NULL;
	int result = 0;

	json = json_object();
	if(json == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json create object error\n");
		result = -1;
		goto error_finish;
	}

	json_object_set_new(json, "Protocol_Version",	json_string(KT_FOTA_PROTOCOL_VER));
	json_object_set_new(json, "CTN",				json_string(kt_srv_get_cti(tmp_buf)));
	if(qty_mode == eDEVICE_MODEM_QTY_MODE)
	{
	json_object_set_new(json, "Model_Name",			json_string(kt_srv_get_model_name()));
	if(req_type == ePUSH_MODE)
		json_object_set_new(json, "REQ_Type",			json_string("PUSH"));
	else
		json_object_set_new(json, "REQ_Type",			json_string("POLLING"));

	if(qty_mode == eMODEM_QTY_MODE)
		json_object_set_new(json, "QTY_Type",			json_string("M"));
	else
		json_object_set_new(json, "QTY_Type",			json_string("A"));
	}
	
	mtmp = json_loads(kt_srv_get_modem_qty(tmp_buf, 512), 0, &error);
	if(!mtmp){
		LOGE(LOG_TARGET, "KT_FOTA_SVC, error : root\n");
		LOGE(LOG_TARGET, "KT_FOTA_SVC, error : on line %d: %s\n", error.line, error.text);
		result = -2;
		goto error_finish;
	}
	json_object_set_new(json, "Modem_Qinfo", mtmp);
	

	if(qty_mode == eDEVICE_MODEM_QTY_MODE)
	{
		dqty = json_array();

		json_array_append_new(dqty, json_string(kt_srv_get_device_qty_cpu(tmp_buf)));
		json_array_append_new(dqty, json_string(kt_srv_get_device_qty_mem(tmp_buf)));
		json_array_append_new(dqty, json_string(kt_srv_get_device_qty_hw(tmp_buf)));
		json_array_append_new(dqty, json_string(kt_srv_get_device_qty_err(tmp_buf)));
		json_object_set_new(json, "Device_Qinfo", dqty);

		json_object_set_new(json, "Custom_Tag",			json_string(kt_srv_get_cust_tag(tmp_buf)));
	}

	post_data = json_dumps( json, 0 );
	if(post_data == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json dump data NULL error\n");
		result = -3;
		goto error_finish;
	}

	if(qty_mode == eMODEM_QTY_MODE)
		result = kt_fota_srv_send(eKT_MODEM_QTY_NOTI, post_data, time_out);
	else
		result = kt_fota_srv_send(eKT_DEVICE_QTY_NOTI, post_data, time_out);

	if(result < 0) result += -100;


error_finish:
	if(post_data != NULL)
		free(post_data);

	/* free json object */
	if(json != NULL)
		json_decref(json);

	if(mtmp != NULL)
		json_decref(mtmp);

	if(dqty != NULL)
		json_decref(dqty);

	return result;
}

int send_fota_req_packet(KT_Fota_Req_Mode_t req_type, KT_Fota_DL_Mode_t dl_mode, int time_out)
{
	char tmp_buf[512];
	json_t *json = NULL;
	char *post_data = NULL;
	int result = 0;

	json = json_object();
	if(json == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json create object error\n");
		result = -1;
		goto error_finish;
	}

	json_object_set_new(json, "Protocol_Version",	json_string(KT_FOTA_PROTOCOL_VER));
	json_object_set_new(json, "CTN",				json_string(kt_srv_get_cti(tmp_buf)));
	json_object_set_new(json, "Model_Name",			json_string(kt_srv_get_model_name()));
	json_object_set_new(json, "Pkg_Version",		json_string(kt_srv_get_package_version(tmp_buf)));

	if(dl_mode == eHTTP)
		json_object_set_new(json, "DL_Protocol",			json_string("HTTP"));
	else
		json_object_set_new(json, "DL_Protocol",			json_string("FTP"));

	if(req_type == ePUSH_MODE)
		json_object_set_new(json, "FOTA_Type",			json_string("PUSH"));
	else
		json_object_set_new(json, "FOTA_Type",			json_string("POLLING"));
	

	json_object_set_new(json, "Custom_Tag",			json_string(kt_srv_get_cust_tag(tmp_buf)));
	json_object_set_new(json, "Echo_Tag",			json_string(""));

	post_data = json_dumps( json, 0 );
	if(post_data == NULL) {
		LOGE(LOG_TARGET, "KT_FOTA_SVC, json dump data NULL error\n");
		result = -3;
		goto error_finish;
	}

	result = kt_fota_srv_send(eKT_FOTA_REQ, post_data, time_out);
	if(result < 0) result += -100;


error_finish:
	if(post_data != NULL)
		free(post_data);

	/* free json object */
	if(json != NULL)
		json_decref(json);

	return result;
}
