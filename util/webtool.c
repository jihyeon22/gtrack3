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

#include <util/nettool.h>
#include "webtool.h"
#include <logd_rpc.h>

static int webtool_get_Method(char *str, httpHeader_t *request);
static int webtool_get_HeaderLength(char *str, httpHeader_t *request);
static int webtool_get_Header(char *str, httpHeader_t *request);
static int webtool_get_Body(char *str, char *body);
static int webtool_get_result(const int sock);

#define FINDSTR_CONTENTS_LEN "Content-Length: "
#define FINDSTR_HTTP_HDRSEP "\r\n\r\n"
#define FINDSTR_RESULT_CODE "<code>"

/*===========================================================================================*/
int webtool_send_Request(const httpRequest_t *query)
{
	int sock = -1;
	struct sockaddr_in server;
	int len = 0;
	char buff[WEBTOOL_MAX_SNDBUFF];

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) {
		fprintf(stderr, "<webdm> %s : %d\n", "error at", __LINE__);
		return -1;
	}

	printf("query->HostAddress is [%s]\r\n", query->HostAddress);
	if((int)(server.sin_addr.s_addr = inet_addr(query->HostAddress)) == -1)
	{
		close(sock);
		fprintf(stderr, "<webdm> %s : %d\n", "error at", __LINE__);
		return -1;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(query->PortNum);

	if(nettool_connect_timeo(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in), 4) < 0)
	{
		close(sock);
		fprintf(stderr, "<webdm> %s : %s:%d --> %s\n", "connect", query->HostAddress, query->PortNum, "cannot connect to server.");
		return -1;
	}

	memset((char*)buff, 0x00, sizeof(buff));

	char* p = (char*)buff;
	p += sprintf(p, "%s ", query->Method);
	p += sprintf(p, "/%s HTTP/1.1\r\n", query->Destination);
	p += sprintf(p, "Host: %s:%d\r\n", query->HostAddress, query->PortNum);

	if(query->ContentLength > 0)
	{
		p += sprintf(p, "Content-Length: %d\r\n", query->ContentLength);
	}

	if(strlen(query->ContentType) > 0)
	{
		p += sprintf(p, "Content-Type: %s\r\n\r\n", query->ContentType);
	}

	if(query->pContent > 0)
	{
		p += sprintf(p, "%s", query->pContent);
	}

	len = p - (char*)buff;
	//printf("query->pContent is [%s]\r\n",query->pContent );

	if(send(sock, (char*)buff, len, MSG_NOSIGNAL) != len)
	{
		close(sock);
		fprintf(stderr, "<webdm> %s : %d\n", "error at", __LINE__);
		return -1;
	}

	int rc = webtool_get_result(sock);
	close(sock);
	fprintf(stderr, "<webdm> %s : %s:%d --> %s with return code %d\n", "connect", query->HostAddress, query->PortNum, "end.", rc);
	return !(rc < 0);
}

/*===========================================================================================*/
int webtool_send_Response(const int sock, char *content_buff, const int content_buff_size, const char *http_ver, const char *codemsg)
{
	int length = 0;
	char header[WEBTOOL_HEADERSIZE];
	memset(header, 0x00, WEBTOOL_HEADERSIZE);

	if(content_buff != NULL) {
		length = strlen(content_buff);
		sprintf(header, "%s %s \r\n"
		        "Server: %s\r\n"
		        "Connection: close\r\n"
		        "Content-Type: text/xml;"
		        "Content-Length:%d"
		        "\r\n\r\n",
		        http_ver, codemsg, WEBTOOL_USER_AGENT, length);

		write(sock, header, strlen(header));
		write(sock, content_buff, length);
	} else {
		sprintf(header, "%s %s \r\n"
		        "Server: %s\r\n"
		        "Connection: close\r\n"
		        "Content-Type: text/html;"
		        "Content-Length:%d\r\n\r\n",
		        http_ver, codemsg, WEBTOOL_USER_AGENT, 0);

		write(sock, header, strlen(header));
	}

	return 1;
}

/*===========================================================================================*/
static int webtool_get_Header(char *str, httpHeader_t *request)
{
	int i = 0;
	int ok = -1;
	int length = strlen(str);
	char* buff = malloc(length + 1);
	memset(buff, 0x00, length + 1);

	strcpy(buff, str);

	if(webtool_get_HeaderLength(buff, request) < 0) {
		free(buff);
		return -1;
	}

	if(webtool_get_Method(buff, request) < 0) {
		free(buff);
		return -1;
	}

	for(i = 0; i < length; i++) {
		if(strncmp(buff + i, FINDSTR_CONTENTS_LEN, strlen(FINDSTR_CONTENTS_LEN)) == 0)
		{
			char* p = buff + (i + strlen(FINDSTR_CONTENTS_LEN));
			request->contents_length = atoi(p);
			ok = 1;
			break;
		}
	}

	free(buff);

	return ok;
}

/*===========================================================================================*/
static int webtool_get_Method(char *str, httpHeader_t *request)
{
	int i = 0;
	int ok = -1;
	char *tok = NULL;
	char seperator[] = " \r\n";
	char *temp_bp = NULL;

	tok = strtok_r(str, seperator, &temp_bp);

	for(i = 0; i < 3; i++) {
		if(tok == NULL) {
			break;
		}

		if(i == 0) {
			strcpy(request->method, tok);
		} else if(i == 1) {
			strcpy(request->page, tok);
		} else if(i == 2) {
			strcpy(request->http_ver, tok);
			ok = 1;
		}

		tok = strtok_r(NULL, seperator, &temp_bp);
	}

	return ok;
}

/*===========================================================================================*/
static int webtool_get_Body(char *str, char *body)
{
	int i;
	int length = strlen(str);

	for(i = 0; i < length; i++) {
		if(strncmp(str + i, FINDSTR_HTTP_HDRSEP, 4) == 0)
		{
			char* p = str + (i + 4);
			strcpy(body, p);
			break;
		}
	}

	return 1;
}

/*===========================================================================================*/
static int webtool_get_HeaderLength(char *str, httpHeader_t *request)
{
	int i = 0;
	int ok = -1;
	int length = strlen(str);

	for(i = 0; i < length; i++) {
		if(strncmp(str + i, FINDSTR_HTTP_HDRSEP, 4) == 0)
		{
			ok = request->header_length = i + 4;
			break;
		}
	}

	return ok;
}

/*===========================================================================================*/
static int webtool_get_Rc(char *str)
{
	int i = 0;
	int ok = -1;
	int length = strlen(str);

	for(i = 0; i < length; i++) {
		if(strncmp(str + i, FINDSTR_RESULT_CODE, strlen(FINDSTR_RESULT_CODE)) == 0)
		{
			char* p = str + (i + strlen(FINDSTR_RESULT_CODE));
			ok = atoi(p);
			break;
		}
	}

	return ok;
}

/*===========================================================================================*/
static int webtool_get_result(const int sock)
{
	httpHeader_t request;
	char requested[WEBTOOL_MAX_RCVBUFF];
	char body[WEBTOOL_MAX_RCVBUFF];
	char resp[WEBTOOL_MAX_SNDBUFF];
	char buff[WEBTOOL_MAXLINE];
	ssize_t recv = 0;
	ssize_t accu = 0;

	memset(&request, 0x00, sizeof(request));
	memset(requested, 0x00, WEBTOOL_MAX_RCVBUFF);
	memset(body, 0x00, WEBTOOL_MAX_RCVBUFF);
	memset(resp, 0x00, WEBTOOL_MAX_SNDBUFF);

	do {
		memset(buff, 0x00, WEBTOOL_MAXLINE);
		recv = read(sock, buff, WEBTOOL_MAXLINE - 1);

		if(recv < 0) {
			return -1;
		}

		accu += recv;
		buff[recv]  = '\0';

		if(accu > WEBTOOL_MAX_RCVBUFF) {
			break;
		}

		strcat(requested, buff);

		if(webtool_get_Header((char*)requested, &request) < 0) {
			continue;
		}

		if(request.header_length + request.contents_length <= accu) {
			break;
		}
	} while(1);

	webtool_get_Body((char*)requested, body);
//	fprintf(stderr, "<webdm> BODY: %s\n", body);


	return webtool_get_Rc(body);
}

