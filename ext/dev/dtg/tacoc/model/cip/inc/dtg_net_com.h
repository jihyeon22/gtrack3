<<<<<<< HEAD
#ifndef __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__
#define __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__

#define MDS_IP	1
#define CIP_IP	2

int send_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData, int IPOfServer);
int send_to_reg_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData);
	
#endif
=======
#ifndef __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__
#define __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__

#define MDS_IP	1
#define CIP_IP	2

int send_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData, int IPOfServer);
int send_to_reg_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData);
	
#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
