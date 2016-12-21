#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#include <base/config.h>
//#include <base/gpstool.h>
#include <at/at_util.h>
#include <base/sender.h>
#include <base/devel.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <logd_rpc.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>
#include <data-list.h>

#include <include/defines.h>
#include <base/at-bridge.h>
#include <util/validation.h>

#include "model_util.h"

#include "at_bridge_cmd.h"
#include "at_bridge_net.h"

#include <util/at-bridge-util.h>
#include <board/modem-time.h>


static int _count_char(const char* str, int str_len, const char c)
{
	int count = 0;
	int i = 0;
	
	for (i = 0 ; i < str_len ; i++ )
	{
		if ( str[i] == c )
			count ++;
	}
	
	printf("count_char ret is [%d]\r\n", count);
	return count;
}

// -------------------------------------------------------------
//  string and char util...
// -------------------------------------------------------------

int atb_strlen_without_cr(const char *s)
{
	int cnt = 0;

	while (*s)
	{
		//printf("strlen [%c]\r\n" ,*s);
		if ( ( *s != '\r' ) && ( *s != '\n' ) )
			cnt++;
		s++;
	}
	//printf("strlen count [%d]\r\n" ,cnt);
	return cnt;
}

int atb_remove_cr(const char *s, char* target, int target_len)
{
	int cnt = 0;

	while (*s)
	{
		//printf("strlen [%c]\r\n" ,*s);
		if ( ( *s != '\r' ) && ( *s != '\n' ) )
		{
			target[cnt] = *s;
			cnt++;
			
			if (cnt > target_len)
				return -1;
		}
		s++;
	}
	//printf("strlen count [%d]\r\n" ,cnt);
	return cnt;
}


int atb_remove_char(const char *s, char* target, int target_len, char targ_char)
{
	int cnt = 0;

	while (*s)
	{
		//printf("strlen [%c]\r\n" ,*s);
		if ( ( *s != targ_char ) )
		{
			target[cnt] = *s;
			cnt++;
			
			if (cnt > target_len)
				return -1;
		}
		s++;
	}
	//printf("strlen count [%d]\r\n" ,cnt);
	return cnt;
}


int atb_get_tok(const char* input, const char start, const char* end, char* result, int result_len)
{
	char* end_tok = NULL;
	
	int i = 0;
	int found_end_tok = 0;
	
	if ((input == NULL) || (end == NULL) || (result == NULL))
		return -1;
	
	while(*input)
	{
		if (*input == start)
		{
			break;
		}
		input++;
	}

	input++;
	
	while(*input)
	{
		end_tok = (char*) end;
		found_end_tok = 0;
		
		while(*end_tok)
		{
			if ( (*input == *end_tok) )
			{
				found_end_tok = 1;
				break;
			}
			end_tok++;
		}
		
		if (found_end_tok == 1)
			break;
		
		result[i++] = *input;
		*input++;
	}
	
	return i;
}
	
	
int atb_read_line ( const char *cmd, const int cmd_len, char* buff, int buff_len)
{
	int i = 0;
	int j = 0;
	
	for ( i = 0 ; i < cmd_len ; i ++)
	{
		if ( (cmd[i] == '\r') || (cmd[i] == '\n'))
		{
			//printf("buff[j] => [%c] / cmd[i] => [%c]\r\n", buff[j-1] ,cmd[i] );
			break;
		}
		buff[j++] = cmd[i];
		//printf("buff[j] => [%c] / cmd[i] => [%c]\r\n", buff[j-1] ,cmd[i] );
	}

	return ++i;
}

int atb_str_replace ( const char *string, const char *substr, const char *replacement , char* buff, int buff_len)
{
	int total_len = 0;
	
	int input_len = strlen(string);
	int substr_len = strlen(substr);
	int replacement_len = strlen(replacement);
	
	int replace_offset_start = 0;
	int replace_offset_end = 0;
	
	char* tok;
	// check argument
	if ( substr == NULL || replacement == NULL )
	{
		//LOGE(LOG_TARGET, "%s: Argument is null!! \r\n",__FUNCTION__);
		return -1;
	}
	
	// check buff size
	total_len = input_len - substr_len + replacement_len + 1 ;
	//printf("total_len is [%d]\r\n",total_len);
	
	if ( total_len > buff_len  )
	{
		//LOGE(LOG_TARGET, "%s: buffer size is too small!! \r\n",__FUNCTION__);
		return -1;
	}
	
	// find substr index
	tok = strstr ( string, substr );
	replace_offset_start = input_len - strlen(tok);
	replace_offset_end = replace_offset_start + substr_len;
	
	//check buff len
	strncpy( buff, string, replace_offset_start);
	strncpy( buff + replace_offset_start, replacement, replacement_len );
	strncpy( buff + replace_offset_start + replacement_len , string + replace_offset_end, input_len - replace_offset_end);
	
	return 0;
	
}


// -------------------------------------------------------------
//   at cmd check util
// -------------------------------------------------------------

// ?§ÏùåÍ≥?Í∞ôÏ? Î∞©Ïãù?ºÎ°ú ?§Ïñ¥??noti Î•?code Îß?Î¶¨ÌÑ¥?úÎã§.
/*
$$ALT: 101, Camped on WCDMA

$$ALT: 350, IMSI detach indication

$$ALT: 300, Cell attach success
*/
int atb_get_noti_code(const char* cmd, const char* arg)
{
	char code[32] = {0,};
	
	// cmd Î°úÎäî "$$ALT" ???ïÌÉúÎ°??§Ïñ¥?®Îã§.
	atb_get_tok(arg, ':', ",", code, 32);
	
	if (validation_check_is_num(code, strlen(code)) == DEFINES_MDS_OK)
	{
		return atoi(code);
	}
	return 0;
}


int atb_check_argument(const char* cmd, char** arg)
{
	char tok;
	int cmd_len = strlen(cmd);
	int ret;
	
	// argument check.
	ret == strncasecmp(*arg, cmd, cmd_len);
	
	if ( ret != 0 )
	{
		return ARG_RET_FAIL;
	}
	
	// Î™ÖÎ†π?¥Í? ?ÑÏ†Ñ???ºÏπò. Ï¶??§Î•∏ Î™ÖÎ†π?¥ÏóÜ??.. ?®Ï†Ñ???¥Îãπ Î™ÖÎ†π??
	if ( atb_strlen_without_cr(*arg) == strlen(cmd) )
	{
		return ARG_RET_NORMAL;
	}
	
	tok = *(*arg + cmd_len);
	
	switch(tok)
	{
		case ARG_RET_READ_SETTING_CHAR:
		{
			//printf("arg parse : ARG_RET_READ_SETTING_CHAR\r\n");
			*arg += cmd_len + 1;
			ret = ARG_RET_READ_SETTING;
			break;
		}
		case ARG_RET_SET_VALUE_CHAR:
		{
			//printf("arg parse : ARG_RET_SET_VALUE_CHAR\r\n");
			*arg += cmd_len + 1;
			ret = ARG_RET_SET_VALUE;
			break;
		}
		default :
		{
			//printf("arg parse : fail??? [%c]\r\n",tok);
			ret = ARG_RET_FAIL;
		}
	}
	return ret;
}



int atb_get_pure_cmd ( const char *cmd, const int cmd_len, char* buff)
{
	int i = 0;
	int j = 0;
	
	for ( i = 0 ; i < cmd_len ; i ++)
	{
		if ( (cmd[i] == '\r') || (cmd[i] == '\n') || (cmd[i] == ':') || (cmd[i] == '=') || (cmd[i] == '?') )
		{
			break;
		}
		buff[j++] = cmd[i];
	}
	
	return j;
}




// -----------------------------------------------------------
//  at cmd parse... 
// -----------------------------------------------------------

int _atb_parse_cmd_rssi(const char* result)
{
	// $$RSSI: 74
	int i = 0;
	char code[32] = {0,};
	
	char* cmd = (char*)result;
	
	// cmd Î°úÎäî "$$RSSI" ???ïÌÉúÎ°??§Ïñ¥?®Îã§.
	while(*cmd)
	{
		if (*cmd == ':')
		{
			break;
		}
		cmd++;
	}

	cmd++;
	
	while(*cmd)
	{
		if ((*cmd == '\r') || (*cmd == '\r') )
		{
			break;
		}
		code[i++] = *cmd;
		cmd++;
	}
	
	if (validation_check_is_num(code, strlen(code)) == DEFINES_MDS_OK)
	{
		//printf("rssi is [%d] \r\n", atoi(code));
		return atoi(code);
	}
	return 0;
}


// Ï¥?6Í∞úÏùò ?∏ÏûêÍ∞Ä ?§Ïñ¥?¨Îïå ?¥Îã§.
// ex) 0,121,131,160,167,3395 
//     1 2   3   4   5   6
//
//     - 1 : set ip index
//     - 2 ~ 5 : ip info
//     - 6 : port info

int _atb_parse_ip_info_type1(char* buff, int buff_len, int* index, CONN_SERVER_INFO_T* net_info)
{
	char token[ ] = ",";
	char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	int err = 0;
	int result[6] = {0,};

	// check argument 
	// AT$$IPCTRIP=0,121,131,160,167,3395
	
	// check argument count... 
	memset(net_info, 0x00, sizeof(CONN_SERVER_INFO_T));
	if (_count_char(buff,buff_len,',') != 5 )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	
	//
	// 1 : index
	tr = strtok_r(buff, token, &temp_bp);
	printf("_set_ip_info check arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 0 [%s]\r\n", tr);
		result[0] = atoi(tr);
	}
	
	// 2: ip 1
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 1 [%s]\r\n", tr);
		result[1] = atoi(tr);
	}
	
	// 3: ip 2
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 2 [%s]\r\n", tr);
		result[2] = atoi(tr);
	}
	
	
	// 4: ip 3
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 3 [%s]\r\n", tr);
		result[3] = atoi(tr);
	}
	
	
	// 5: ip 4
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 4 [%s]\r\n", tr);
		result[4] = atoi(tr);
	}
	
	// 6: port
	tr = strtok_r(NULL, token_1, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 5 [%s]\r\n", tr);
		result[5] = atoi(tr);
	}
		
	// fill net info
	sprintf(net_info->server_ip, "%d.%d.%d.%d", result[1], result[2], result[3], result[4] );
	net_info->port = result[5];
	
	*index = result[0];

CMD_PARSE_FAIL:

	if (err < 0)
		return err;
	else
		return AT_BRIDGE_CMD_TRUE;
}

// Ï¥?5Í∞úÏùò ?∏ÏûêÍ∞Ä ?§Ïñ¥?¨Îïå ?¥Îã§. ?Ä?•Ï? ?êÎèô?ºÎ°ú 0Î≤??∏Îç±?§Ïóê ?úÎã§.
// ex) 121,131,160,167,3395 
//     1   2   3   4   5   
//
//     - 1 ~ 4 : ip info
//     - 5 : port info

int _atb_parse_ip_info_type2(char* buff, int buff_len, CONN_SERVER_INFO_T* net_info)
{
	char token[ ] = ",";
	char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	int err = 0;
	int result[6] = {0,};
	
	// check argument 
	// AT$$IPCTOSIP=219,251,4,182,30001
	
	memset(net_info, 0x00, sizeof(CONN_SERVER_INFO_T));
	// check argument count... 
	if (_count_char(buff,buff_len,',') != 4 )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	
	//
	// 1 : index

	printf("_set_ip_info check arg 0 skip\r\n");
	result[0] = 0;

	// 2: ip 1
	tr = strtok_r(buff, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 1 [%s]\r\n", tr);
		result[1] = atoi(tr);
	}
	
	// 3: ip 2
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 2 [%s]\r\n", tr);
		result[2] = atoi(tr);
	}
	
	
	// 4: ip 3
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 3 [%s]\r\n", tr);
		result[3] = atoi(tr);
	}
	
	
	// 5: ip 4
	tr = strtok_r(NULL, token, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 4 [%s]\r\n", tr);
		result[4] = atoi(tr);
	}
	
	// 6: port
	tr = strtok_r(NULL, token_1, &temp_bp);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		printf("_set_ip_info check arg 5 [%s]\r\n", tr);
		result[5] = atoi(tr);
	}
		
	// fill net info
	sprintf(net_info->server_ip, "%d.%d.%d.%d", result[1], result[2], result[3], result[4] );
	net_info->port = result[5];
	
CMD_PARSE_FAIL:

	if (err < 0)
		return err;
	else
		return AT_BRIDGE_CMD_TRUE;
}

int _atb_parse_cmd_modem_stat(char* cmd, MODEM_STAT_CMD_RESULT_T* result)
{
	// $$MODEM_STATE: 0, 25, 0, 1, 0, 1, 3, 0
//	int i = 0;
//	char code[32] = {0,};
	
	//char* cmd_tmp = (char*)cmd;
	
	char token[ ] = ",";
	char token_1[ ] = "\r\n";
	char *temp_bp = NULL;
	char *tr = NULL;
	int err = 0;
	//int result[6] = {0,};
	
	// check argument 
	if (_count_char(cmd,strlen(cmd),',') != 7 )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	
	
	
	// cmd Î°úÎäî "$$RSSI" ???ïÌÉúÎ°??§Ïñ¥?®Îã§.
	while(*cmd)
	{
		if (*cmd == ':')
		{
			break;
		}
		cmd++;
	}
	
	cmd++;
	
	printf("%s : cmd is [%s]\r\n", __FUNCTION__, cmd);
	// 0: result code.  skip
	tr = strtok_r(cmd, token, &temp_bp);
	
	// 1 : rssi
	tr = strtok_r(NULL, token, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		result->rssi = atoi(tr);
		printf("_atb_parse_cmd_modem_stat result->rssi [%d]\r\n", result->rssi);
	}
	
	// 2 : usim stat
	tr = strtok_r(NULL, token, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		// ?îÎùº?òÍ≤∞Í≥ºÍ∞í?ºÎ°ú Î≥Ä??		int convert_ret_code =0;
		int ret_code = atoi(tr);
		switch(ret_code)
		{
			case 0: // ready
				convert_ret_code = 0;	// tela - ready
				break;
			case 1: // sim pin, sim puk
			case 2: // net pin
				convert_ret_code = 1; // tela - net pin
				break;
			case 3: // lock
				convert_ret_code = 2; // tela - lock
				break;
			case 4: // open
				convert_ret_code = 3; // tela - open
				break;
			case 5: // open test
				convert_ret_code = 4; // tela - test
				break;
			case 6: // ruim type
				convert_ret_code = 6; // noting... tela - fail
				break;
			case 7: // fail
				convert_ret_code = 6; // tela - fail
				break;
			case 8: // perm lock
				convert_ret_code = 6; // noting... tela - fail
				break;
			case 9: // wait
				convert_ret_code = 5; // tela - wait
				break;
			case 10: // remove
				convert_ret_code = 99;  // tela - sim remove
				break;
			default:
				convert_ret_code = ret_code;
		}
		
		result->usim_state = convert_ret_code;
		
		printf("_atb_parse_cmd_modem_stat result->usim_state [%d]\r\n", result->usim_state);
	}
	
	// 3 : network regi stat
	tr = strtok_r(NULL, token, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		// ?îÎùº?òÍ≤∞Í≥ºÍ∞í?ºÎ°ú Î≥Ä??		int convert_ret_code =0;
		int ret_code = atoi(tr);
		switch(ret_code)
		{
			case 0 : // network ?±Î°ù?äÎê®
				convert_ret_code = 1;
				break;
			case 1 :
				convert_ret_code = 0;
				break;
			case 2 : 
			case 3 :
			case 4 :
			case 5 :
			default:
				convert_ret_code = ret_code;
		}
		result->net_reg = convert_ret_code;
		
		// deregi..
		if ( g_regi_usim_stat == USIM_STAT_DEREGI)
		{
			result->net_reg = 101;
		}
		printf("_atb_parse_cmd_modem_stat result->net_reg [%d]\r\n", result->net_reg);
	}
	
	// 4 : network state
	tr = strtok_r(NULL, token, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		// ?§Ï†ú Í∞íÏù¥ ?ÑÎãà??at bridge ?ÑÎ°úÍ∑∏Îû®??network stat ??Î¶¨ÌÑ¥?úÎã§.
		result->net_stat = _teladin_convert_net_stat();
		printf("_atb_parse_cmd_modem_stat result->net_stat [%d]\r\n", result->net_stat);
	}
	
	// 5 : sock state
	tr = strtok_r(NULL, token, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		result->sock_stat = atoi(tr);
		printf("_atb_parse_cmd_modem_stat result->sock_stat [%d]\r\n", result->sock_stat);
	}
	
	// 6 : service state
	tr = strtok_r(NULL, token, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		// ?îÎùº?òÍ≤∞Í≥ºÍ∞í?ºÎ°ú Î≥Ä??		int convert_ret_code =0;
		int ret_code = atoi(tr);
		switch(ret_code)
		{
			case 0:
			case 1:
			case 2:
				convert_ret_code = 1;
				break;
			case 3:
			case 4:
			case 5:
				convert_ret_code = 0;
				break;
			default:
				convert_ret_code = ret_code;
		}
		
		result->svc_stat = convert_ret_code;
		printf("_atb_parse_cmd_modem_stat result->svc_stat [%d]\r\n", result->svc_stat);
	}
	
	// 7 : pdp reject code
	tr = strtok_r(NULL, token_1, &temp_bp);
	//printf("_atb_parse_cmd_modem_stat arg 0 [%s]\r\n", tr);
	if ( (tr != NULL) && ( validation_check_is_num(tr, strlen(tr)) != DEFINES_MDS_OK ) )
	{
		err = -1;
		goto CMD_PARSE_FAIL;
	}
	else
	{
		/*
		// ?îÎùº?òÍ≤∞Í≥ºÍ∞í?ºÎ°ú Î≥Ä??		int convert_ret_code =0;
		int ret_code = atoi(tr);
		switch(ret_code)
		{
			default:
				convert_ret_code = ret_code;
		}*/
		
		result->pdp_rej_cause = atoi(tr);
		printf("_atb_parse_cmd_modem_stat result->pdp_rej_cause [%d]\r\n", result->pdp_rej_cause);
	}
CMD_PARSE_FAIL:

	if (err < 0)
		return err;
	else
		return AT_BRIDGE_CMD_TRUE;
	
}


int _atb_parse_ktdevstat_tela(const char* cmd, char* buff, int buff_len)
{
	char tmp_result[512] = {0,};
	char ret_result[512] = {0,};
	
	atb_get_tok(cmd, '[', "]", tmp_result, 512);
	atb_remove_char(tmp_result, ret_result, 512, '\"');
	
	strcpy(buff, ret_result);
	return 0;
}


// -----------------------------------------------------------
// write to uart : cmd echo...
// -----------------------------------------------------------

int atb_ret_echo_ok(const char* cmd, const char* value)
{
	int offset = 2; // "at" strlen is 2
	char buff[512] = {0,};
	
	sprintf(buff, "\r\n%s: %s\r\n\r\n",cmd + offset, value);
	atb_write_uart_append(buff, strlen(buff), WRITE_APPEND);
	atb_write_uart_append("OK\r\n", strlen("OK\r\n") , WRITE_AND_FLUSH);
	//atb_write_uart(buff, strlen(buff), SEND_CR);
	//atb_write_uart("OK\r\n", strlen("OK\r\n"), SEND_CR);
	return 0;
}

int atb_ret_echo_err(const char* cmd, const char* value)
{
	int offset = 2; // "at" strlen is 2
	char buff[512] = {0,};
	
	sprintf(buff, "\r\n%s: %s\r\n\r\n",cmd+offset, value);
	atb_write_uart_append(buff, strlen(buff), WRITE_APPEND);
	atb_write_uart_append("ERROR\r\n", strlen("ERROR\r\n") , WRITE_AND_FLUSH);
	return 0;
}

// -----------------------------------------------------------
//  search string : boyer moore algorithm
// -----------------------------------------------------------


# include <limits.h>
# include <string.h>
# include <stdio.h>
 
# define NO_OF_CHARS 256

// A utility function to get maximum of two integers
static int max (int a, int b) { return (a > b)? a: b; }
 
// The preprocessing function for Boyer Moore's bad character heuristic
//static void badCharHeuristic( char *str, int size, int badchar[NO_OF_CHARS])
static void badCharHeuristic(const char *str, int size, int badchar[NO_OF_CHARS])
{
    int i;
 
    // Initialize all occurrences as -1
    for (i = 0; i < NO_OF_CHARS; i++)
         badchar[i] = -1;
 
    // Fill the actual value of last occurrence of a character
    for (i = 0; i < size; i++)
         badchar[(int) str[i]] = i;
}
 
/* A pattern searching function that uses Bad Character Heuristic of
   Boyer Moore Algorithm */
int bm_search( const char *txt,  int n, const char *pat, int m)
{
 
    int badchar[NO_OF_CHARS];
 
    /* Fill the bad character array by calling the preprocessing
       function badCharHeuristic() for given pattern */
    badCharHeuristic(pat, m, badchar);
 
    int s = 0;  // s is shift of the pattern with respect to text
    while(s <= (n - m))
    {
        int j = m-1;
 
        /* Keep reducing index j of pattern while characters of
           pattern and text are matching at this shift s */
        while(j >= 0 && pat[j] == txt[s+j])
            j--;

 
        /* If the pattern is present at current shift, then index j
           will become -1 after the above loop */
        if (j < 0)
        {
            // printf("\n pattern occurs at shift = %d\r\n", s);
			// kksowrks : return char start index..
			return s;
 
            /* Shift the pattern so that the next character in text
               aligns with the last occurrence of it in pattern.
               The condition s+m < n is necessary for the case when
               pattern occurs at the end of text */
            s += (s+m < n)? m-badchar[(unsigned int)txt[s+m]] : 1;
 
        }
 
        else
            /* Shift the pattern so that the bad character in text
               aligns with the last occurrence of it in pattern. The
               max function is used to make sure that we get a positive
               shift. We may get a negative shift if the last occurrence
               of bad character in pattern is on the right side of the
               current character. */
            s += max(1, j - badchar[(unsigned int)txt[s+j]]);
    }
	
	return -1;
	
}





// ---------------------------------------------------------------
// for debug
// ---------------------------------------------------------------
#ifdef DEBUG_ATCMD_HISTORY

#define MAX_SAVED_CMD			20
#define MAX_SAVED_CMD_LEN		512

//static char debug_saved_cmd[MAX_SAVED_CMD][MAX_SAVED_CMD_LEN] = {0,};
typedef struct 
{
	time_t log_time;
	char saved_cmd[MAX_SAVED_CMD_LEN];
}saved_cmd_info_t;

saved_cmd_info_t saved_cmd_info[MAX_SAVED_CMD];

static int saved_cmd_idx = 0;

//void dbg_util_save_cmd(char* input_str)
void dbg_util_save_cmd(const char* input_str)
{
	char saved_buff[MAX_SAVED_CMD_LEN] = {0,};

	int str_len_1 = 0;
	str_len_1 = atb_remove_cr( input_str, saved_buff, MAX_SAVED_CMD_LEN);

	
	if ( str_len_1 <= 0 )
	{
//		printf("%s - error [%d] [%s] \r\n",__func__,str_len_1, saved_buff);
		return ;
	}
	
//	printf("%s - success [%s]\r\n",__func__, input_str);

	if ( str_len_1 > MAX_SAVED_CMD_LEN )
	{
		str_len_1 = MAX_SAVED_CMD_LEN - 1;
	}

	memset( saved_cmd_info[saved_cmd_idx].saved_cmd, 0x00, MAX_SAVED_CMD_LEN );
	memset( &saved_cmd_info[saved_cmd_idx].log_time, 0x00, sizeof(time_t));


	strncpy( saved_cmd_info[saved_cmd_idx].saved_cmd, saved_buff, str_len_1);
	saved_cmd_info[saved_cmd_idx].log_time = get_modem_time_utc_sec();

	saved_cmd_idx += 1;
	saved_cmd_idx = saved_cmd_idx % MAX_SAVED_CMD;

	//dbg_util_print_saved_cmd(NULL);
}
#define AT_CMD_STACK_FILE_PATH	"/tmp/atcmd_stack"

void dbg_util_print_saved_cmd(const char* file_save_name)
{
	int i = 0;
	int print_cur_idx = 0;

	FILE *printf_fd = stderr;
	//time_t current_modem_tm = get_modem_time_utc_sec();

	struct tm *print_tm = NULL;

	if ( file_save_name != NULL )
	{

		printf_fd = fopen(file_save_name, "w");
	}

	if (printf_fd == NULL)
	{
		printf("ERROR!!!!!!!!!!!!! %s is err.. \r\n", __func__);
		return;
	}

	for ( i = 0 ; i < MAX_SAVED_CMD ; i++)
	{
		print_cur_idx = (saved_cmd_idx + i) % MAX_SAVED_CMD	;
		print_tm = localtime(&saved_cmd_info[print_cur_idx].log_time);

		fprintf(printf_fd, "[%d] (%d:%d:%d) %s\r\n",	print_cur_idx, 
													print_tm->tm_hour,
													print_tm->tm_min,
													print_tm->tm_sec,
													saved_cmd_info[print_cur_idx].saved_cmd);
	}

	if ( file_save_name != NULL )
	{
		fclose(printf_fd);
	}

}

extern time_t g_last_datapkt_mode_tm;
#define DEBUG_CHK_LAST_DATA_PKT_TIME_SEC 180
extern int g_dbg_write_fail_cnt;
extern int g_dbg_conn_fail_cnt;

void dbg_util_print_webdm()
{
	time_t current_modem_tm = get_modem_time_utc_sec();
	struct tm *print_tm = NULL;
	char file_name[64] = {0,};

	print_tm = localtime(&current_modem_tm);
	sprintf(file_name,"%s_%02d%02d.log", AT_CMD_STACK_FILE_PATH, print_tm->tm_hour, print_tm->tm_min);

	// 
//	if ( ( current_modem_tm - g_last_datapkt_mode_tm ) >  DEBUG_CHK_LAST_DATA_PKT_TIME_SEC)
	if (1)
	{
		print_tm = localtime(&g_last_datapkt_mode_tm);
#if 0
		devel_webdm_send_log("W : RCV TIMEOUT : Last PKT (%d:%d:%d) / CF(%d) WF (%d) / log [%s]", 	print_tm->tm_hour, print_tm->tm_min, print_tm->tm_sec,
																							  	g_dbg_conn_fail_cnt,
																								g_dbg_write_fail_cnt,
																								file_name);
#endif
//		g_last_datapkt_mode_tm.tm_hour;
//		g_last_datapkt_mode_tm.tm_min;

//		devel_webdm_send_log("Wanning :  ");
//		ÎßàÏ?Îß?AT CMD Î©îÏãúÏßÄ ?úÍ∞Ñ?êÏÑú 
		//dbg_util_print_saved_cmd(file_name);
	}
	else
	{
		
	}
//	devel_webdm_send_log("Wanning : PKT RECV TIMEOUT");
}
#endif
