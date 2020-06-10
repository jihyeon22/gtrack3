<<<<<<< HEAD
#ifndef __UTIL_TRANSFER_H__
#define __UTIL_TRANSFER_H__

typedef struct transferSetting transferSetting_t;
struct transferSetting
{
	int retry_count_connect;
	int retry_count_send;
	int retry_count_receive;
	int timeout_secs;
	char ip[40];
	unsigned short port;
};

int transfer_packet(const transferSetting_t *setting , const unsigned char *buff, const int buff_len);
int transfer_packet_recv(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len);
int transfer_packet_recv_etx(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len, int etx);

#endif
=======
#ifndef __UTIL_TRANSFER_H__
#define __UTIL_TRANSFER_H__

typedef struct transferSetting transferSetting_t;
struct transferSetting
{
	int retry_count_connect;
	int retry_count_send;
	int retry_count_receive;
	int timeout_secs;
	char ip[40];
	unsigned short port;
};

int transfer_packet(const transferSetting_t *setting , const unsigned char *buff, const int buff_len);
int transfer_packet_recv(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len);
int transfer_packet_recv_etx(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len, int etx);

#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
