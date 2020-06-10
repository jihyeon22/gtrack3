#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <base/config.h>
#include <at/at_util.h>
#include <base/watchdog.h>
#include <base/thermtool.h>
#include <board/power.h>
#include <board/led.h>
#include <board/battery.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include "callback.h"
#include "config.h"
#include "custom.h"
#include "data-list.h"
#include "debug.h"
#include "netcom.h"
#include "config.h"

#include "server_resp.h"


#include <nisso_mdt800/packet.h>
#include <nisso_mdt800/gpsmng.h>
#include <nisso_mdt800/hdlc_async.h>
#include <nisso_mdt800/file_mileage.h>

int server_resp_proc(char* resp_buff)
{
    int buff_len = strlen(resp_buff);
    char resp_str[5][SERVER_RESP_MAX_LEN] = {0,};
    int i = 0;
    int j = 0;

    int res_1 = -1;
    int res_2 = -1;
    int res = -1;

	char *tr;
    char token_0[ ] = "[]";
    char *temp_bp = NULL;
    
    char *p_cmd = resp_buff;

    memset(&resp_str, 0x00, sizeof(resp_str));

    LOGI(LOG_TARGET, "%s> resp proc 1 :: [%s]\r\n", __func__, resp_buff);
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    while(tr != NULL)
    {
        strcpy(resp_str[i++], tr);
        tr = strtok_r(NULL, token_0, &temp_bp);
    }

    if ( i <= 0 )
        return -1;

    for( j = 0 ; j < i ; j ++)
    {
        printf("parse ret str : [%d] => [%s]\r\n", j, resp_str[j]);
        if ( strcmp(resp_str[j],"0") == 0)
            res_1 = 0;
        else
            res_2 = parse_model_sms("0000", SERVER_RESP_PROC_PHONENUM, resp_str[j]);
    }

//	tr = strtok_r(NULL, token_0, &temp_bp);
//    if(tr == NULL) return AT_RET_FAIL;
//	printf(" >>>> tr is [%s]\r\n", tr);

    printf( "%s> resp proc 2 :: res_1 [%d] res_2[%d]\r\n", __func__, res_1, res_2);

    if ((res_1 >= 0 ) || (res_2 >= 0) )
        res = 0;
    else
        res = -1;

    
    return res;

}