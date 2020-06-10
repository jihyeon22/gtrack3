/*
 * rpc_clnt_operation.h
 *
 *  Created on: 2013. 3. 18.
 *      Author: ongten
 */

#ifndef RPC_CLNT_OPERATION_H_
#define RPC_CLNT_OPERATION_H_

int network_connect();
int net_time_sync();
int data_req_to_taco();
int reboot(int delay);

#endif /* RPC_CLNT_OPERATION_H_ */
