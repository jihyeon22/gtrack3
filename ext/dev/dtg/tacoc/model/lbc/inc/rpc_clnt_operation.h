<<<<<<< HEAD
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
int data_req_to_taco_cmd(int command, int period, int size, int action_state);
int reboot(int delay);

#endif /* RPC_CLNT_OPERATION_H_ */
=======
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
int data_req_to_taco_cmd(int command, int period, int size, int action_state);
int reboot(int delay);

#endif /* RPC_CLNT_OPERATION_H_ */
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
