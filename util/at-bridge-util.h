#ifndef __AT_BRIDGE_UTIL_H__
#define __AT_BRIDGE_UTIL_H__

#define ARG_RET_FAIL				-1
#define ARG_RET_NORMAL				0	// 완전히 일치하는 명령어
#define ARG_RET_READ_SETTING		1
#define ARG_RET_READ_SETTING_CHAR	'?'
#define ARG_RET_SET_VALUE			2
#define ARG_RET_SET_VALUE_CHAR		'='

#include "at_bridge_net.h"
#include "at_bridge_cmd.h"

// string util
int atb_strlen_without_cr(const char *s);
int atb_remove_cr(const char *s, char* target, int target_len);
int atb_get_tok(const char* input, const char start, const char* end, char* result, int result_len);
int atb_read_line ( const char *cmd, const int cmd_len, char* buff, int buff_len);
int atb_str_replace ( const char *string, const char *substr, const char *replacement , char* buff, int buff_len);

// at cmd util
int atb_get_noti_code(const char* cmd, const char* arg);
int atb_check_argument(const char* cmd, char** arg);
int atb_get_pure_cmd ( const char *cmd, const int cmd_len, char* buff);

// result and parse
int _atb_parse_ip_info_type1(char* buff, int buff_len, int* index, CONN_SERVER_INFO_T* net_info);
int _atb_parse_ip_info_type2(char* buff, int buff_len, CONN_SERVER_INFO_T* net_info);
int _atb_parse_cmd_rssi(const char* result);
int _atb_parse_cmd_modem_stat(char* cmd, MODEM_STAT_CMD_RESULT_T* result);
int _atb_parse_ktdevstat_tela(const char* cmd, char* buff, int buff_len);

// at cmd echo util
int atb_ret_echo_ok(const char* cmd, const char* value);
int atb_ret_echo_err(const char* cmd, const char* value);

// search string : boyer moore algorithm
int bm_search( const char *txt,  int n, const char *pat, int m);




#ifdef DEBUG_ATCMD_HISTORY
//void dbg_util_save_cmd(char* input_str);
void dbg_util_save_cmd(const char* input_str);
void dbg_util_print_saved_cmd(const char* file_name);
void dbg_util_print_webdm(void);
#endif

#endif

