#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_tools.h>
#include <wrapper/dtg_atcmd.h>
#include <dm/update.h>

#include "diag.h"
#include "dtg_ini_utill.h"
#include "dtg_net_com.h"
#include "dtg_packet.h"
//#include "rpc_clnt_operation.h"

//// // #include "taco_rpc.h"

#define SMSSIGN				"SD"
#define SMSVERSTRING		'0'
#define SMSREPLYSTRING		'R'

#define TACOC_SMS			"TACOC SMS : "

//

/*
 * SMS Format
 * SDXYY Content
 * X  : Version
 * YY : Opcode
 */

typedef struct {
	char date[25];
	char tel[20];
	char contents[100];
}MSG_CONTENTS;

#define MSG_HISTORY_FILE_NAME	"/var/msg.com.history"
#define MSG_HISTORY_MAX_COUNT	20


int insert_sms_command_history(char *p_phone_number, char *pmsg)
{
	struct timeval tv;
	struct tm *ptm;
	MSG_CONTENTS data;
	MSG_CONTENTS hist_data[MSG_HISTORY_MAX_COUNT];
	int fd;
	int nread;
	int nhist_cnt;
	int i;

	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);

	memset(&data, 0x00, sizeof(MSG_CONTENTS));

	sprintf(data.date, "%04d-%02d-%02d %02d:%02d:%02d\n", 
			ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, 
			ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	if(p_phone_number != NULL)
		sprintf(data.tel, "[%s]\n", p_phone_number);

	if(pmsg != NULL)
		sprintf(data.contents, "[%s]\n\n", pmsg);

	fd = open(MSG_HISTORY_FILE_NAME, O_RDWR | O_CREAT , 0644);
	if(fd < 0) {
		printf("file open fail : err[%d]\n", errno);
		return -1;
	}

	nread = read(fd, hist_data, sizeof(MSG_CONTENTS)*MSG_HISTORY_MAX_COUNT);
	printf("nread = [%d]\n", nread);
	nhist_cnt = nread / sizeof(MSG_CONTENTS);
	printf("nhist_cnt = [%d]\n", nhist_cnt);

	lseek(fd, 0, SEEK_SET);
	
	write(fd, &data, sizeof(MSG_CONTENTS));

	if( (nhist_cnt + 1) > MSG_HISTORY_MAX_COUNT )
		nhist_cnt -= 1;

	write(fd, hist_data, sizeof(MSG_CONTENTS)*nhist_cnt);

		
	close(fd);

	return 0;

}

static int find_text(int *idx, int max_len, char *msg, char *txt, int txt_len)
{
	int i;
	int cur_idx = *idx;

	for(i = cur_idx; i < max_len; i++) {
		if(msg[i] == ' ' || msg[i] == 0x0d || msg[i] == 0x0a)
			continue;
		else {
			if( (max_len - i) < txt_len)
				return -1;
				
			if(!memcmp(&msg[i], txt, txt_len)) {
				*idx = i + txt_len;
				return 0;
			}
		}
		
	}
	return -2;
}

static int get_sms_opcode(int *idx, int max_len, char *msg, int *opcode)
{
	int i;
	int cur_idx = *idx;

	for(i = cur_idx; i < max_len; i++) {
		if(msg[i] == ' ' || msg[i] == 0x0d || msg[i] == 0x0a)
			continue;
		else {
			if( (max_len - i) < 2)
				return -1;
				
			if(msg[i] >= '0' && msg[i] <= '9') {
				if(msg[i+1] >= '0' && msg[i+1] <= '9') {
					*idx = i + 2;
					*opcode = (msg[i] - '0') * 10;
					*opcode += msg[i+1] - '0';
					return 0;
				}
				else {
					return -2;
				}
			}
			else {
				return -3;
			}
		}
		
	}
	return -4;
}

static int get_sms_data(int *idx, int max_len, char *msg, char delemiter, char *data, int data_len)
{
	int i;
	int data_idx = 0;
	int cur_idx = *idx;
	
	DTG_LOGD(TACOC_SMS "get_sms_data> cur_idx[%d], delemiter[0x%02x]", cur_idx, delemiter);
	
	memset(data, 0x00, data_len);
	for(i = cur_idx; i < max_len; i++) {
		if(msg[i] == ' ' || msg[i] == 0x0d || msg[i] == 0x0a) {
			continue;
		}
		else {
			if(msg[i] == delemiter) {
				if(data_idx == 0) {
					return -1;
				}
				*idx = i + 1;
				return 0;
			}
			if( (data_idx + 1) >= data_len)
				return -2;
				
			data[data_idx++] = msg[i];

		}
		
	}
	if(delemiter == 0x00) {
		if(data_idx == 0) {
			return -3;
		}
		*idx = i;
		return 0;
	}
	return -4;
}

int common_sms_parser(char *sms_msg, char *caller)
{
	char reponse_sms[80];
	char p_msg[128] = {0};
	int opcode;
	int err;
	int idx;
	int sms_msg_len;
	bool sms_reply = false;

	strcpy(p_msg, sms_msg);

	if(p_msg == NULL) {
		DTG_LOGE(TACOC_SMS "sms_parser> sms msg is NULL error");
		return -1;
	}
	sms_msg_len = strlen(p_msg);

	if (insert_sms_command_history(caller, p_msg) < 0)
		DTG_LOGE(TACOC_SMS "Fail to save sms.");
	
	idx = 0;
	//find signature ++
	if( (err = find_text(&idx, sms_msg_len, p_msg, SMSSIGN, 2)) < 0) {
		DTG_LOGE(TACOC_SMS "Signature isn't detected : err[%d]", err);
		return -2;
	}else {
		DTG_LOGW(TACOC_SMS "Signature is detected");
	}
	//find signature --
	
	//find version ++
	switch (p_msg[idx]) {
		case SMSVERSTRING:
			DTG_LOGW(TACOC_SMS "version string is detected");
			idx++;
			break;
		case SMSREPLYSTRING:
			DTG_LOGW(TACOC_SMS "reply string is detected");
			sms_reply = true;
			idx++;
			break;
		default:
			DTG_LOGE(TACOC_SMS "unique code is missmatched");
			return -3;
	}

	//get opcode ++
	if( (err = get_sms_opcode(&idx, sms_msg_len, p_msg, &opcode)) < 0) {
		DTG_LOGE(TACOC_SMS "opcode isn't detected : err[%d]", err);
		return -4;
	}else {
		DTG_LOGW(TACOC_SMS "opcode is detected : opcode[%d]", opcode);
	}
	//get opcode --
	
	switch(opcode) {
		case 1: //
			DTG_LOGW(TACOC_SMS "ATD Reserved");
			break;

		case 2: //server ip and port change
		{
		#if ( defined(SERVER_MODEL_CIP) || defined(SERVER_MODEL_LBC) )

			char ip[32] = {0};
			char port[32] = {0};
			DTG_LOGW(TACOC_SMS "Change Transfer Server IP and PORT");
			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, ':', ip, sizeof(ip)) ) < 0) {
				DTG_LOGE(TACOC_SMS "server ip isn't detected : err[%d]\n", err);
				return -21;
			}else {
				DTG_LOGW(TACOC_SMS "server ip is detected : IP[%s]\n", ip);
			}

			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, 0x00, port, sizeof(port)) ) < 0) {
				DTG_LOGE(TACOC_SMS "server port isn't detected : err[%d]\n", err);
				return -22;
			}else {
				DTG_LOGW(TACOC_SMS "server ip is detected : port[%s]\n", port);
			}

			set_server_ip_addr(ip);
			set_server_port(atoi(port));

			save_ini_server_ip_and_port();
			
			if (sms_reply) {
				sprintf(reponse_sms, "OK IP[%s]/Port[%s] Command", ip, port);
				atcmd_send_sms(reponse_sms, NULL, caller);
			}
		#else
			DTG_LOGW(TACOC_SMS "This SMS Command support Only in CIP, LBC\n");
		#endif
		}

		break;

		case 3: //change period sending into server
			{
			#if ( defined(SERVER_MODEL_CIP) || defined(SERVER_MODEL_LBC) )
				char interval[8] = {0};

				DTG_LOGW(TACOC_SMS "Change Transfer Period");
				if( (err = get_sms_data(&idx, sms_msg_len, p_msg, 0x00, interval, sizeof(interval)) ) < 0) {
					DTG_LOGW(TACOC_SMS "server sending period isn't detected : err[%d]\n", err);
					return -31;
				}else {
					DTG_LOGW(TACOC_SMS "server sending period is detected : period[%s]\n", interval);
				}

				set_interval(atoi(interval)*60);
				save_ini_packet_sending_period();
				
				if (sms_reply) {
					sprintf(reponse_sms, "OK Report Interval [%s]min Command", interval);
					atcmd_send_sms(reponse_sms, NULL, caller);
				}
			#else
				DTG_LOGW(TACOC_SMS "This SMS Command support Only in CIP, LBC\n");
			#endif
			}
			break;

		case 4: //request diag infor
			{
			#if ( defined(SERVER_MODEL_CIP) || defined(SERVER_MODEL_LBC) )
				char message[80] = { 0 };
				int ret;
				FILE *fp;
				DTG_LOGW(TACOC_SMS "Request Diag Information");

				/* SMS */
				if (sms_reply) {
					
					//sprintf(message, "RSSI:[%d]\nIP:[%s]\nPORT:[%d]\nInterval:[%d]min",
					sprintf(message, "[%d]\n[%s]\n[%d]\n[%d]",
							atcmd_get_rssi(),
							get_server_ip_addr(),
							get_server_port(),
							get_interval()/60	/* 기본?�위??분이??*/
						);
					DTG_LOGW(TACOC_SMS "DIAGINFO SMS>\n%s", message);
					atcmd_send_sms(message, NULL, caller);
				}

			#ifdef SERVER_MODEL_CIP
				/* TCP */
				if (sms_reply == 0) {
					ret = create_diag_info(DIAG_ALL);

					send_to_reg_server(MSG_TYPE_DIAG_DATA);
				}
			#endif //#ifdef SERVER_MODEL_CIP

			#else //#if ( defined(SERVER_MODEL_CIP) || defined(SERVER_MODEL_LBC) )
				DTG_LOGW(TACOC_SMS "This SMS Command support Only in CIP, LBC Server\n");
			#endif
			}
			break;

		case 5: 
		case 6: 
			{
			#if (defined(DEVICE_MODEL_UCAR))
				char *result = malloc(128);
				char *cmd = malloc(128);
				tacom_info_t info;
				DTG_LOGW(TACOC_SMS "Request Set DTG config. (Only supported UCAR DTG)");
				memset(result, 0, 128);
				memset(cmd, 0, 128);
				if( (err = get_sms_data(&idx, sms_msg_len, p_msg, ':', cmd, 128) ) < 0) {
					atcmd_send_sms("Fail to set DTG factor.", NULL, caller);
					return 0;
				}
				info.code = strtol(cmd,NULL,10);
				if( (err = get_sms_data(&idx, sms_msg_len, p_msg, 0x00, cmd, 128) ) < 0) {
					atcmd_send_sms("Fail to set DTG factor.", NULL, caller);
					return 0;
				}
				info.data = cmd;
				int ret = taco_set_info_call_wrapper(&info);
				
				if (sms_reply) {
					if (result == NULL)
						atcmd_send_sms("Fail to set DTG factor.", NULL, caller);
					else {
						if ((*result) < 0)
							atcmd_send_sms("Fail setting.", NULL, caller);
						else
							atcmd_send_sms(result, NULL, caller);
							//atcmd_send_sms("Success setting.", NULL, caller);
					}
				}
				//free(result); //taco_set_info_call ?�서 free?�다.
				//free(cmd);    //taco_set_info_call ?�서 free?�다.
			#else
				DTG_LOGW(TACOC_SMS "This SMS Command support Only in UCAR DTG");
			#endif
			}
			break;

	/*
	 * 

	 */
		case 7: 
			{
		#if (defined(DEVICE_MODEL_LOOP))
				char *result = malloc(128);
				char *cmd = malloc(128);
				int ret;
				DTG_LOGW(TACOC_SMS "Request Set DTG proofreading factor. (Only suppoted LOOP DTG in this version)");
				memset(result, 0, 128);
				memset(cmd, 0, 128);
				if( (err = get_sms_data(&idx, sms_msg_len, p_msg, 0x00, cmd, 128) ) < 0) {
					atcmd_send_sms("Fail to set DTG factor.", NULL, caller);
					break;
				}

				ret = taco_set_factor_call(&cmd, &result, clnt_taco);
				

				if (sms_reply) {
					if (result == NULL)
						atcmd_send_sms("Fail to set DTG factor.", NULL, caller);
					else
						atcmd_send_sms(result, NULL, caller);
				}
				//free(result); //taco_set_factor_call ?�서 free?�다.
				//free(cmd);	//taco_set_factor_call ?�서 free?�다.
			#else
				DTG_LOGW(TACOC_SMS "This SMS Command support Only in LOOP DTG");
			#endif
			}
			break;

		case 8://phone number for reporting errors
#ifdef NO_DIAG_ERR_REPORT
			{
				char phonenum[32] = {0};
				char message[80] = { 0 };
				char *erpn = NULL;
				
				/* parameter 가 ?�을 경우 err reporting phonenum ��?SMS ��??�린??*/
				if( (err = get_sms_data(&idx, sms_msg_len, p_msg, 0x00, phonenum, sizeof(phonenum)) ) < 0) {
					erpn = get_err_report_phonenum();
					sprintf(message, "Current ER: %s", erpn ? erpn : "None");
				}
				/* parameter 가 0 ??경우 ?�러 리포?�을 ?��? ?�는??*/
				else if (strcmp(phonenum, "0") == 0) {
					set_err_report_phonenum(NULL);
					save_ini_err_report_phonenum(NULL);
					sprintf(message, "%s", "Error Reporting is disabled");
				} 
				/* parameter 가 ?�화번호?�경???�러 리포???�정???�다 */
				else if (isphonenum(phonenum)) {
					set_err_report_phonenum(phonenum);
					save_ini_err_report_phonenum(phonenum);
					sprintf(message, "Error Reporting is enabled (%s)", phonenum);
				} 
				/* parameter 가 ?�화번호가 ?�닌경우 ?�러처리?�다 */
				else {
					sprintf(message, "Error Reporting Phone Number(%s) is not valid", phonenum);
					DTG_LOGW(TACOC_SMS "%s", message);
					if (sms_reply)
						atcmd_send_sms(message, NULL, caller);

					return -81;
				}
				
				DTG_LOGW(TACOC_SMS "%s", message);
				if (sms_reply)
					atcmd_send_sms(message, NULL, caller);

			}
#else
			DTG_LOGW(TACOC_SMS "not supported this server model.");
#endif
			break;

		case 9://dtg device configuration
			DTG_LOGW(TACOC_SMS "SMS : DTG Device Configuration(This Version is not Support)");
			if (sms_reply)
				atcmd_send_sms("This version is not support!!", NULL, caller);
			break;
		case 10://ATD DTG initiative Command Reserved
			DTG_LOGW(TACOC_SMS "ATD DTG initiative Command Reserved");
			break;
		case 11://ATD get dtg ppp0 ip address 
			DTG_LOGW(TACOC_SMS "ATD get dtg ppp0 ip address");
			break;
/*
		case 12: //manual ftp update. 
		{

			char ip[32] = {0};
			char port[12] = {0};
			char id[32] = {0};
			char passwrd[32] = {0};
			char file_path[128] = {0};
			int manual_update_ret;
			DTG_LOGW(TACOC_SMS "TACOC Manual ftp update");
			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, ',', ip, sizeof(ip)) ) < 0) {
				DTG_LOGW(TACOC_SMS "TACOC Manual ftp update, IP Parse Error [%d]", err);
				if (sms_reply)
					atcmd_send_sms("ip error", NULL, caller);
				break;
			}
			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, ',', port, sizeof(port)) ) < 0) {
				DTG_LOGW(TACOC_SMS "TACOC Manual ftp update, Port Parse Error [%d]", err);
				if (sms_reply)
					atcmd_send_sms("port error", NULL, caller);
				break;
			}
			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, ',', id, sizeof(id)) ) < 0) {
				DTG_LOGW(TACOC_SMS "TACOC Manual ftp update, ID Parse Error [%d]", err);
				if (sms_reply)
					atcmd_send_sms("ID error", NULL, caller);
				break;
			}
			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, ',', passwrd, sizeof(passwrd)) ) < 0) {
				DTG_LOGW(TACOC_SMS "TACOC Manual ftp update, ID Parse Error [%d]", err);
				if (sms_reply)
					atcmd_send_sms("passwrd error", NULL, caller);
				break;
			}
			if( (err = get_sms_data(&idx, sms_msg_len, p_msg, 0x00, file_path, sizeof(file_path)) ) < 0) {
				DTG_LOGW(TACOC_SMS "TACOC Manual ftp update, file_path Parse Error [%d]", err);
				if (sms_reply)
					atcmd_send_sms("file_path error", NULL, caller);
				break;
			}
			DTG_LOGI(TACOC_SMS "manual update info> update_server_ip[%s]", ip);
			DTG_LOGI(TACOC_SMS "manual update info> update_server_port[%d]", atoi(port));
			DTG_LOGI(TACOC_SMS "manual update info> id[%s]", id);
			DTG_LOGI(TACOC_SMS "manual update info> passwrd[%s]", passwrd);
			DTG_LOGI(TACOC_SMS "manual update info> file_path[%s]", file_path);
	
			
			// Step 1. FTP Download
			DTG_LOGI(TACOC_SMS "Manual FTP Download Run..");
			manual_update_ret = update_ftp_download(ip,
													atoi(port),
													id,
													passwrd,
													file_path);
			if (manual_update_ret < 0) {
				DTG_LOGE(TACOC_SMS "%s", update_strerror(update_errno));
				if (sms_reply)
					atcmd_send_sms(update_strerror(update_errno), NULL, caller);
				break;
			}

			// Step 2. uncompress for downloaded file
			DTG_LOGI(TACOC_SMS "Manual UPDATE Run..");
			manual_update_ret = update_cmd(1,"NONE","NONE");
			if (manual_update_ret < 0) {
				DTG_LOGE(TACOC_SMS "%s", update_strerror(update_errno));
				if (sms_reply)
					atcmd_send_sms(update_strerror(update_errno), NULL, caller);
				break;
			}

			if (sms_reply)
				atcmd_send_sms("Manual Update File Download OK!! Device will reboot", NULL, caller);

			// Step 3. Reboot 
			do {
				manual_update_ret = reboot(60);
			} while (manual_update_ret < 0);
		}
			break;
*/

/* case 13 ~ 15 SMS Command for DTG Test
		case 13:
			DTG_LOGE(TACOC_SMS "taco file save[%d]\n", opcode);
			idx = 3;
			while(idx-- > 0) 
			{
				err = data_req_to_taco_cmd(ACCUMAL_DATA, 0x10000000, 0, 2); //DTG Data File Save
				DTG_LOGE(TACOC_SMS "data_req_to_taco_cmd[%d]\n", err);
				if(err >= 0)
					break;
				sleep(1);
			}
			break;
		case 14:
			DTG_LOGE(TACOC_SMS "taco file save & mdmc_power_off test[%d]\n", opcode);
			extern void mdmc_power_off();
			mdmc_power_off();
			break;
		case 15:
			data_req_to_taco_cmd(ACCUMAL_DATA, 0x20000000, 0, 2); //data abort test
			break;
*/
		default:
			DTG_LOGE(TACOC_SMS "unsuported opcode[%d]\n", opcode);
			if (sms_reply)
				atcmd_send_sms("Unsupported OPCODE", NULL, caller);
			return -10000;
	}
	
	return 0;
}
