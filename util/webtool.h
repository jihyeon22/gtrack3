#ifndef __UTIL_WEBTOOL_H__
#define __UTIL_WEBTOOL_H__

#define WEBTOOL_HTTP_POST	0
#define WEBTOOL_HTTP_PUT	1
#define WEBTOOL_HTTP_GET	2
#define WEBTOOL_HTTP_DELETE	3

#define WEBTOOL_MAX_SNDBUFF		(1024 * 8)
#define WEBTOOL_MAX_SNDBODY		((1024 * 8) + 256)
#define WEBTOOL_MAX_RCVBODY		(1024 + 512)
#define WEBTOOL_MAX_RCVBUFF		(1024 + 512)
#define WEBTOOL_HEADERSIZE		512
#define WEBTOOL_MAXLINE			256

#define WEBTOOL_USER_AGENT "NEOM2M/1.01"
#define WEBTOOL_HTTP_RESP_200	"200"
#define WEBTOOL_HTTP_RESP_400	"400"
#define WEBTOOL_HTTP_RESP_500	"500"

typedef struct httpRequest httpRequest_t;
struct httpRequest {
	int	 PortNum;
	char  Method[8];
	char  HostAddress[64];
	char  Destination[64];
	char	 ContentType[16];
	char* pContent;
	int 	 ContentLength;
};

typedef struct httpResponse httpResponse_t;
struct httpResponse {
	int   RetCode;
	char	 ContentType[16];
	char* pContent;
	int 	 ContentLength;
};

typedef struct httpHeader httpHeader_t;
struct httpHeader
{
	char method[8];
	char page[256];
	char http_ver[16];
	int header_length;
	int contents_length;
};

int webtool_send_Request(const httpRequest_t *query);
int webtool_send_Response(const int sock, char *content_buff, const int content_buff_size, const char *http_ver, const char *codemsg);

#endif
