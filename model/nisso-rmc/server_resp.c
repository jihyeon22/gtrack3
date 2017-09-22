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
    char resp_str[SERVER_RESP_MAX_LEN] = {0,};

    LOGI(LOG_TARGET, "%s> resp proc :: [%s]\r\n", __func__, resp_buff);
    
    // check valid resp
    if ( ( resp_buff[0] != '[') || ( resp_buff[buff_len-1] != ']') )
    {
        LOGE(LOG_TARGET, "%s> resp proc :: error invalid data [%s]\r\n", __func__, resp_buff);
        return -1;
    }

    strncpy(resp_str, resp_buff+1, buff_len-2);

    // &12,1,0,2,37.4756163,126.8818914,60,1,2,00.0000000,000.0000000,0,2,2,00.0000000,000.0000000,0,1

    if ( strcmp(resp_str, "0") == 0 )
    {
        LOGI(LOG_TARGET, "%s> resp proc :: normal resp return.\n", __func__);
        return 1;
    }

    parse_model_sms("0000", SERVER_RESP_PROC_PHONENUM, resp_str);
    return 1;

}