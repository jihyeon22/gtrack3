/*
 * dtg_regist_process.h
 *
 *  Created on: 2013. 4. 8.
 *      Author: ongten
 */

#ifndef DTG_REGIST_PROCESS_H_
#define DTG_REGIST_PROCESS_H_

#define DTG_FILE_STATUS_PATH	"/var/dtg_status"
#define DTG_FILE_STATUS_SIZE	128
#define DTG_ENV_SIZE			2 
#define DTG_STAT_SIZE			6
#define DTG_RL_SIZE				10
#define DTG_VRN_SIZE			12

void send_device_registration();
void send_device_de_registration();
void check_update();
int send_reg_to_server(int type);
char* get_dtg_env(void);
char* get_dtg_stat(void);
char* get_dtg_rl(void);
char* get_dtg_vrn(void);

void send_server_error_report(char *packet_type, int server_resp);
#endif /* DTG_REGIST_PROCESS_H_ */
