<<<<<<< HEAD
#pragma once

#pragma pack(push, 1)

struct cu_time_req_packet_body {
	char id[2];
	char comma;
	char hubid[8];
	char crlf[2];
}__attribute__((packed));
typedef struct cu_time_req_packet_body cu_time_req_packet_body_t;


struct cu_por_reset_packet_body {
	char id[2];
	char hubid[8];
	char yymmddhhmm[10];
	char phone_num[11];
	char crlf_1[2];
	char seperator;
	char crlf_2[2];
}__attribute__((packed));
typedef struct cu_por_reset_packet_body cu_por_reset_packet_body_t;


#define REQ_TIME_RESPONSE_LENGTH	(18)

=======
#pragma once

#pragma pack(push, 1)

struct cu_time_req_packet_body {
	char id[2];
	char comma;
	char hubid[8];
	char crlf[2];
}__attribute__((packed));
typedef struct cu_time_req_packet_body cu_time_req_packet_body_t;


struct cu_por_reset_packet_body {
	char id[2];
	char hubid[8];
	char yymmddhhmm[10];
	char phone_num[11];
	char crlf_1[2];
	char seperator;
	char crlf_2[2];
}__attribute__((packed));
typedef struct cu_por_reset_packet_body cu_por_reset_packet_body_t;


#define REQ_TIME_RESPONSE_LENGTH	(18)

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
#pragma pack(pop)