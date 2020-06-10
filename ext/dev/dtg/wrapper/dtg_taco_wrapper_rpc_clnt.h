#ifndef __DTG_TACO_WRAPPER_RPC_CLNT_H__
#define __DTG_TACO_WRAPPER_RPC_CLNT_H__

#include <tacom/tacom_inc.h>


int taco_request_call_wrapper();
int taco_set_info_call_wrapper(tacom_info_t *info);
int taco_command_call_wrapper(int command, int config, int buf_size);


#endif //__DTG_TACO_WRAPPER_RPC_CLNT_H__



