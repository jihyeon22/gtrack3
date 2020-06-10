#ifndef __SKYAN_RESP_H__
#define __SKYAN_RESP_H__

#define SKYAN_JSON_PARSE_SUCCESS  2
#define SKYAN_JSON_PARSE_FAIL     -3
#define SKYAN_JSON_PKT_RET_FAIL       -4

int skyan_resp__parse(char* recv_buff);

#endif