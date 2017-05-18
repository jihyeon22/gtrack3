#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kt_fota_svc/kt_dmc_proc.h"

#include "logd/logd_rpc.h"

#define LOG_TARGET eSVC_BASE


int kt_dmc_proc_for_tl500k(char* buf)
{
	// APPS_AT$$KT_DMC_EXECUTE=E3087F6B0C0EE24A28C854302A60464702C800000028DB094B545349445F303031,
    char *tr;
    char token_0[ ] = "=\r\n,";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
	int i = 0;

	char* prog_argv[32] = {0,};
	int prog_argv_index = 0;

	char cmd_line_str[1024] = {0,};
	char cmd_line_argument[512] = {0,};
	int cmd_line_arugment_len = 0;

	if (buf == NULL)
		return -1;

	if (( strstr(buf, KT_MODEM_FOTA_DMC_PROC_1_STR) == NULL ) ||
		( strstr(buf, KT_MODEM_FOTA_DMC_PROC_2_STR) == NULL ))
	{
		LOGE(LOG_TARGET, "[KT FOTA SVC] error no proc str\r\n");
		return -1;
	}

	p_cmd = buf;
	
	tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return -1;
//	printf(" >>>> tr is [%s]\r\n", tr);
	
	// argument "-m"
	prog_argv[prog_argv_index] = malloc(strlen("-m") + 1);
	memset(prog_argv[prog_argv_index], 0, strlen("-m") + 1);
	strncpy(prog_argv[prog_argv_index], "-m", strlen("-m"));

	prog_argv_index ++;

	while(1)
	{
		tr = strtok_r(NULL, token_0, &temp_bp);
    	if(tr == NULL) break;
		//printf(" >>>> tr is [%s]\r\n", tr);

		prog_argv[prog_argv_index] = malloc(strlen(tr) + 1);
		memset(prog_argv[prog_argv_index], 0, strlen(tr) + 1);
		strncpy(prog_argv[prog_argv_index], tr, strlen(tr));

		prog_argv_index ++;
	}
	
	for ( i = 0 ; i < prog_argv_index ; i ++ )
	{
		LOGI(LOG_TARGET, "[KT FOTA SVC] run dmc cmd argv [%d]:[%s]\r\n", i, prog_argv[i]);
	}

	if ( prog_argv_index > 1 )
	{
		mds_api_system_fork("/kt_dms/bin/kt_dmc",1, prog_argv);
	}
	else
	{
		LOGE(LOG_TARGET, "[KT FOTA SVC] error : no argument \r\n");
		return -1;
	}


		
	return 1;
}
