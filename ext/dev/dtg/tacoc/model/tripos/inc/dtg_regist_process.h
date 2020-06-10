/*
 * dtg_regist_process.h
 *
 *  Created on: 2013. 4. 8.
 *      Author: ongten
 */

#ifndef DTG_REGIST_PROCESS_H_
#define DTG_REGIST_PROCESS_H_

void send_device_registration();
void send_device_de_registration();
int send_reg_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData);

#endif /* DTG_REGIST_PROCESS_H_ */
