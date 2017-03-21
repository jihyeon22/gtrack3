#ifndef __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__
#define __DTG_NETWORK_COMMUNICATION_FUNCTION_DEFINE_HEADER__

int send_to_reg_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData);
int send_to_server(unsigned char  *buffer_in, int buffer_len);
int send_to_summary_server(unsigned char  *buffer_in, int buffer_len);


#endif
