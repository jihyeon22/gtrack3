<<<<<<< HEAD

#ifndef __DTG_TACOC_WRAPPER_RPC_CLNT_H__
#define __DTG_TACOC_WRAPPER_RPC_CLNT_H__

#define CURRENT_DATA        1
#define REMAIN_DATA         2
#define ACCUMAL_DATA        3
#define CLEAR_DATA          4
#define ACCUMAL_MDT_DATA    5


#include <tacom/tacoc.h>

int tacoc_taco_cb_data_call_wrapper(tacoc_stream_t *frame, int *clnt_res);
int tacoc_breakdown_report_call_wrapper(int *integer, int *clnt_res);
int tacoc_mdmc_power_off_call_wrapper();
int tacoc_sms_cb_data_call_wrapper(char **string, int *clnt_res);


=======

#ifndef __DTG_TACOC_WRAPPER_RPC_CLNT_H__
#define __DTG_TACOC_WRAPPER_RPC_CLNT_H__

#define CURRENT_DATA        1
#define REMAIN_DATA         2
#define ACCUMAL_DATA        3
#define CLEAR_DATA          4
#define ACCUMAL_MDT_DATA    5


#include <tacom/tacoc.h>

int tacoc_taco_cb_data_call_wrapper(tacoc_stream_t *frame, int *clnt_res);
int tacoc_breakdown_report_call_wrapper(int *integer, int *clnt_res);
int tacoc_mdmc_power_off_call_wrapper();
int tacoc_sms_cb_data_call_wrapper(char **string, int *clnt_res);


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
#endif