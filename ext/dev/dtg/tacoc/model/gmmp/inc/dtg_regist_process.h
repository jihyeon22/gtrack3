/*
 * dtg_regist_process.h
 *
 *  Created on: 2013. 4. 8.
 *      Author: ongten
 */

#ifndef DTG_REGIST_PROCESS_H_
#define DTG_REGIST_PROCESS_H_

typedef struct {
	const char xml_starter[6];	//<xml>\n
	const char xml_header_starter[4];	//<H>\n
	const char xml_header_ender[5];	//</H>\n
	const char xml_body_starter[4];	//<B>\n
	const char xml_imei_starter[3];	//<I>
	char imei[15];
	const char xml_imei_ender[5]; //</I>\n
	const char xml_dtgenv_starter[3];	//<D>
	char dtg_env[2];
	const char xml_dtgenv_ender[5]; //</D>\n
	const char xml_dtgstat_starter[4];	//<DS>
	char dtg_stat[6];
	const char xml_dtgstat_ender[6]; //</DS>\n
	const char xml_dtgrl_starter[4];	//<RL>
	char dtg_rl[10];
	const char xml_dtgrl_ender[6]; //</RL>\n
	const char xml_dtgvrn_starter[4];	//<VR>
	char dtg_vrn[12];
	const char xml_dtgvrn_ender[6]; //</VR>\n
	const char xml_body_ender[5];
	const char xml_ender[7];	//</xml>\n
}__attribute__((packed))REGIST_DATA;

void send_device_registration();
void send_device_de_registration();
int send_reg_to_server(int type, unsigned char  *buffer_in, int buffer_len, int orgsize, void *pData);

#endif /* DTG_REGIST_PROCESS_H_ */
