<<<<<<< HEAD
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <base/devel.h>
#include <at/at_util.h>
#include "logd/logd_rpc.h"

#include "tl500s_fota_proc.h"

#define LOG_TARGET eSVC_BASE

int tl500s_fota_proc()
{
    char tmp_ver_str[64] = {0,};
    char tmp_cmd_str[128] = {0,};

    static int fota_proc_run_flag = 0;

    int i = 0;

    if ( fota_proc_run_flag == 1 )
    {
        LOGI(LOG_TARGET, "[TL500S FOTA] already chk fota.. do nothing");
        return 0;
    }

    for (i = 0 ; i < MODEM_VER_CHK_MAX_RETRY ; i ++)
    {
        if ( at_get_modem_swver(tmp_ver_str, 64) == AT_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[TL500S FOTA] get modem version success [%s]",tmp_ver_str);
            break;
        }
        sleep(1);
    }

    if ( i > MODEM_VER_CHK_MAX_RETRY)
    {
        LOGE(LOG_TARGET, "[TL500S FOTA] version check fail.");
        devel_webdm_send_log("[TL500S FOTA] version check fail");
        return -1;
    }

//    LOGI(LOG_TARGET, "[TL500S FOTA] modem version is [%s]", tmp_ver_str);
//    devel_webdm_send_log("[TL500S FOTA] modem version is [%s]", tmp_ver_str);
    // 항상 실행한다.
/*
    if ( strncmp(tmp_ver_str, TL500S_TARGET_ORIGINAL_IMG_VERSION, strlen(TL500S_TARGET_ORIGINAL_IMG_VERSION)) != 0 )
    {
        LOGE(LOG_TARGET, "[TL500S FOTA] FOTA TARGET INVAILD", __func__);
        devel_webdm_send_log("[TL500S FOTA] FOTA TARGET INVAILD");
        fota_proc_run_flag = 1;
        return -1;
    }
*/
    
    
    if ( strncmp(tmp_ver_str, TL500S_TARGET_ORIGINAL_IMG_VERSION, strlen(TL500S_TARGET_ORIGINAL_IMG_VERSION)) == 0 )
        devel_webdm_send_log("[TL500S FOTA] NEED TO FOTA PROC");

    sprintf(tmp_cmd_str, "%s &", TL500S_FOTA_SCRIPT_PATH);
    // devel_webdm_send_log("[TL500S FOTA] FOTA START");

    system(tmp_cmd_str);
    
    fota_proc_run_flag = 1 ;

    return 0;
=======
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <base/devel.h>
#include <at/at_util.h>
#include "logd/logd_rpc.h"

#include "tl500s_fota_proc.h"

#define LOG_TARGET eSVC_BASE

int tl500s_fota_proc()
{
    char tmp_ver_str[64] = {0,};
    char tmp_cmd_str[128] = {0,};

    static int fota_proc_run_flag = 0;

    int i = 0;

    if ( fota_proc_run_flag == 1 )
    {
        LOGI(LOG_TARGET, "[TL500S FOTA] already chk fota.. do nothing");
        return 0;
    }

    for (i = 0 ; i < MODEM_VER_CHK_MAX_RETRY ; i ++)
    {
        if ( at_get_modem_swver(tmp_ver_str, 64) == AT_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[TL500S FOTA] get modem version success [%s]",tmp_ver_str);
            break;
        }
        sleep(1);
    }

    if ( i > MODEM_VER_CHK_MAX_RETRY)
    {
        LOGE(LOG_TARGET, "[TL500S FOTA] version check fail.");
        devel_webdm_send_log("[TL500S FOTA] version check fail");
        return -1;
    }

//    LOGI(LOG_TARGET, "[TL500S FOTA] modem version is [%s]", tmp_ver_str);
//    devel_webdm_send_log("[TL500S FOTA] modem version is [%s]", tmp_ver_str);
    // 항상 실행한다.
/*
    if ( strncmp(tmp_ver_str, TL500S_TARGET_ORIGINAL_IMG_VERSION, strlen(TL500S_TARGET_ORIGINAL_IMG_VERSION)) != 0 )
    {
        LOGE(LOG_TARGET, "[TL500S FOTA] FOTA TARGET INVAILD", __func__);
        devel_webdm_send_log("[TL500S FOTA] FOTA TARGET INVAILD");
        fota_proc_run_flag = 1;
        return -1;
    }
*/
    
    
    if ( strncmp(tmp_ver_str, TL500S_TARGET_ORIGINAL_IMG_VERSION, strlen(TL500S_TARGET_ORIGINAL_IMG_VERSION)) == 0 )
        devel_webdm_send_log("[TL500S FOTA] NEED TO FOTA PROC");

    sprintf(tmp_cmd_str, "%s &", TL500S_FOTA_SCRIPT_PATH);
    // devel_webdm_send_log("[TL500S FOTA] FOTA START");

    system(tmp_cmd_str);
    
    fota_proc_run_flag = 1 ;

    return 0;
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}