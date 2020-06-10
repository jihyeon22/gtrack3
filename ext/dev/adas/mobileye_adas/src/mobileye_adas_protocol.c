#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "mobileye_adas.h"
#include "mobileye_adas_mgr.h"
#include "mobileye_adas_protocol.h"
#include "mobileye_adas_tool.h"
#include "mobileye_adas_uart_util.h"

#define LOG_TARGET eSVC_COMMON

// --------------------------------------------------
// FCW COMMAND : 전방추돌경보
// --------------------------------------------------
// $MEFCW,70*71

int mobileye_evt_cmd_parse__fcw(int argc, char* argv[], MOBILEYE_CMD_FCW_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_FCW__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_FCW__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    retval->speed_kmh = atoi(argv[1]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}

// --------------------------------------------------
// PCW COMMAND : 보행자 충돌 경보
// --------------------------------------------------
int mobileye_evt_cmd_parse__pcw(int argc, char* argv[], MOBILEYE_CMD_PCW_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_PCW__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_PCW__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    retval->speed_kmh = atoi(argv[1]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}



// --------------------------------------------------
// LDW COMMAND : 차선이탈
// --------------------------------------------------
// $MELDW,70,R*02
// $MELDW,70,L*1c
// $MELDW,70,R*02
// $MELDW,70,R*02
int mobileye_evt_cmd_parse__ldw(int argc, char* argv[], MOBILEYE_CMD_LDW_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_LDW__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_LDW__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    retval->speed_kmh = atoi(argv[1]);
    retval->lane_val = argv[2][0];
    // printf("%s() -> retval->lane_val [%c] / [%c] / [%s]\r\n", __func__, retval->lane_val, argv[2][0], argv[2]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}

// --------------------------------------------------
// HMW COMMAND : 차간거리 경보
// --------------------------------------------------
// $MEHMW,70*71
int mobileye_evt_cmd_parse__hmw(int argc, char* argv[], MOBILEYE_CMD_HMW_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_HMW__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_HMW__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    retval->speed_kmh = atoi(argv[1]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}

// --------------------------------------------------
// SLI COMMAND : 속도제한
// --------------------------------------------------
int mobileye_evt_cmd_parse__sli(int argc, char* argv[], MOBILEYE_CMD_SLI_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_SLI__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_SLI__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    retval->warnning_level = atoi(argv[1]);
    retval->speed_kmh = atoi(argv[2]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}

// --------------------------------------------------
// ERR COMMAND : ERR
// --------------------------------------------------
int mobileye_evt_cmd_parse__err(int argc, char* argv[], MOBILEYE_CMD_ERR_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_ERR__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_ERR__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    sprintf(retval->err_code, "%s", argv[1]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}

// --------------------------------------------------
// ALIVE COMMAND : MESYNC
// --------------------------------------------------
// $MESYNC,00*23
int mobileye_evt_cmd_parse__sync(int argc, char* argv[], MOBILEYE_CMD_SYNC_VAL_T* retval)
{
    int chk_arg = MOBILEYE_CMD_SYNC__ARG_CNT;
    char* chk_cmd = MOBILEYE_CMD_SYNC__COMMAND;

    // chk argument ...
    if ( argc != chk_arg)
        return eMOBILEYE_CMD_RET__FAIL;
    
    if ( strcmp(argv[0] , chk_cmd) != 0 )
        return eMOBILEYE_CMD_RET__FAIL;

    // todo somthing..
    retval->sync_code = atoi(argv[1]);

    return MOBILEYE_ADAS_RET_SUCCESS;
}


int mobileye_adas__cmd_broadcast_msg_start()
{
    return MOBILEYE_ADAS_RET_SUCCESS;
}
int mobileye_adas__cmd_broadcast_msg_stop()
{
    return MOBILEYE_ADAS_RET_SUCCESS;
}

int mobileye_get_evt_data(int argc, char* argv[], ADAS_EVT_DATA_T* evt_data)
{
    int i = 0 ;
    static int read_fail_cnt = 0 ;

    if ( argc <= 0 )
    {
        // timeout is argc zero : 1sec interval..
        read_fail_cnt ++;
        // LOGE(LOG_TARGET, "%s() : ERR invalid argc %d\n", __func__, argc);
        goto FINISH;
    }
    
    // -----------------------------------------------------------------
    // FCW COMMAND : 전방추돌경보
    // -----------------------------------------------------------------
    if ( strcmp(argv[0], MOBILEYE_CMD_FCW__COMMAND ) == 0 ) 
    {
        MOBILEYE_CMD_FCW_VAL_T ret = {0,};
        if ( mobileye_evt_cmd_parse__fcw(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> FCW parse FAIL \n");
            read_fail_cnt ++;
        }
        else
        {
            read_fail_cnt = 0;
            LOGT(LOG_TARGET, "$$ MEADAS >> FCW parse SUCCESS : speed_kmh [%d] \n", ret.speed_kmh);
            //mobileye_adas_mgr_sendpkt(CL_ADAS_FCW_EVENT_CODE, ret.speed_kmh, "0");
            if ( ret.speed_kmh > 30 )
                evt_data->evt_code = eADAS_EVT__FCW;
            else
                evt_data->evt_code = eADAS_EVT__UFCW;
            evt_data->evt_data_1 = ret.speed_kmh;
        }
    }
    // -----------------------------------------------------------------
    // PCW COMMAND : 보행자 충돌 경보
    // -----------------------------------------------------------------
    else if ( strcmp(argv[0], MOBILEYE_CMD_PCW__COMMAND ) == 0 )    
    {
        MOBILEYE_CMD_PCW_VAL_T ret = {0,};
        if ( mobileye_evt_cmd_parse__pcw(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> PCW parse FAIL \n");
            read_fail_cnt ++;
        }
        else  // success 
        {
            read_fail_cnt = 0;
            LOGT(LOG_TARGET, "$$ MEADAS >> PCW parse SUCCESS : speed_kmh [%d] \n", ret.speed_kmh);
            //mobileye_adas_mgr_sendpkt(CL_ADAS_PCW_EVENT_CODE, ret.speed_kmh, "0");
            evt_data->evt_code = eADAS_EVT__PCW;
            evt_data->evt_data_1 = ret.speed_kmh;
        }
    }
    // -----------------------------------------------------------------
    // LDW COMMAND : 차선이탈
    // -----------------------------------------------------------------
    else if ( strcmp(argv[0], MOBILEYE_CMD_LDW__COMMAND ) == 0 )    
    {
        MOBILEYE_CMD_LDW_VAL_T ret = {0,};
        if ( mobileye_evt_cmd_parse__ldw(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> LDW parse FAIL \n");
            read_fail_cnt ++;
        }
        else  // success 
        {
            char pkt_opt_str[6+1];
            read_fail_cnt = 0;
            LOGT(LOG_TARGET, "$$ MEADAS >> LDW parse SUCCESS : speed_kmh [%d] / lane_val [%c]\n", ret.speed_kmh, ret.lane_val);
            sprintf(pkt_opt_str, "0,%c", ret.lane_val);

            evt_data->evt_code = eADAS_EVT__LDW;
            evt_data->evt_data_1 = ret.speed_kmh;

            if ( ( ret.lane_val == 'l' ) || ( ret.lane_val == 'L' ) )
                evt_data->evt_data_4 = 0;

            if ( ( ret.lane_val == 'r' ) || ( ret.lane_val == 'R' ) )
                evt_data->evt_data_4 = 1;
            //mobileye_adas_mgr_sendpkt(CL_ADAS_LDW_EVENT_CODE, ret.speed_kmh, pkt_opt_str);
        }
    }
    // -----------------------------------------------------------------
    // HMW COMMAND : 차간거리 경보
    // -----------------------------------------------------------------
    else if ( strcmp(argv[0], MOBILEYE_CMD_HMW__COMMAND ) == 0 )    
    {
        MOBILEYE_CMD_HMW_VAL_T ret = {0,};
        if ( mobileye_evt_cmd_parse__hmw(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> HMW parse FAIL \n");
            read_fail_cnt ++;
        }
        else  // success 
        {
            read_fail_cnt = 0;
            LOGT(LOG_TARGET, "$$ MEADAS >> HMW parse SUCCESS : speed_kmh [%d] / break_val [%d]\n", ret.speed_kmh, ret.break_val);
            //mobileye_adas_mgr_sendpkt(CL_ADAS_HMW_EVENT_CODE, ret.speed_kmh, "0");
            evt_data->evt_code = eADAS_EVT__HMW;
            evt_data->evt_data_1 = ret.speed_kmh;

        }
    }
    // -----------------------------------------------------------------
    // SLI COMMAND : 속도제한
    // -----------------------------------------------------------------
    else if ( strcmp(argv[0], MOBILEYE_CMD_SLI__COMMAND ) == 0 )   
    {
        MOBILEYE_CMD_SLI_VAL_T ret = {0,};
        if ( mobileye_evt_cmd_parse__sli(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> SLI parse FAIL \n");
            read_fail_cnt ++;
        }
        else  // success 
        {
            char pkt_opt_str[6+1];
            read_fail_cnt = 0;
            LOGT(LOG_TARGET, "$$ MEADAS >> SLI parse SUCCESS : speed_kmh [%d] / warnning_level [%d]\n", ret.speed_kmh, ret.warnning_level);

            evt_data->evt_code = eADAS_EVT__SLI;
            evt_data->evt_data_1 = ret.speed_kmh;
            evt_data->evt_data_4 = ret.warnning_level;
            
            //mobileye_adas_mgr_sendpkt(CL_ADAS_SLI_EVENT_CODE, "0", pkt_opt_str);
        }
    }
    // -----------------------------------------------------------------
    // ERR COMMAND : ERR
    // -----------------------------------------------------------------
    else if ( strcmp(argv[0], MOBILEYE_CMD_ERR__COMMAND ) == 0 )    
    {
        MOBILEYE_CMD_ERR_VAL_T ret = {0,};

        // send log..
        {
            int k = 0;
            char tmp_log_str[64] = {0,};
            int tmp_log_str_len = 0;

            for (k = 0; k < argc ; k++)
            {
                tmp_log_str_len += sprintf(tmp_log_str+tmp_log_str_len, "[%s]", argv[k]);
            }
            //devel_webdm_send_log("me err => %s", tmp_log_str);
        }

        if ( mobileye_evt_cmd_parse__err(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> ERR parse FAIL \n");
            read_fail_cnt ++;
        }
        else  // success 
        {
            //char pkt_opt_str[6+1];
            read_fail_cnt = 0;
            //sprintf(pkt_opt_str, "%s", ret.err_code);
            LOGT(LOG_TARGET, "$$ MEADAS >> ERR parse SUCCESS : err_code [%s]\n", ret.err_code);
            //mobileye_adas_mgr_sendpkt(CL_ADAS_ERR_EVENT_CODE, "0", pkt_opt_str);
            evt_data->evt_code = eADAS_EVT__ERR;

            evt_data->evt_data_4 = atoi(ret.err_code);
            
            if ( evt_data->evt_data_4 == 0 )
                evt_data->evt_data_4 = ret.err_code[0] * -1;

            sprintf(evt_data->evt_ext, "%s", ret.err_code);
        }
    }
    // -----------------------------------------------------------------
    // SYNC COMMAND : SYNC
    // -----------------------------------------------------------------
    else if ( strcmp(argv[0], MOBILEYE_CMD_SYNC__COMMAND ) == 0 )    
    {
        MOBILEYE_CMD_SYNC_VAL_T ret = {0,};

        if ( mobileye_evt_cmd_parse__sync(argc, argv, &ret) != MOBILEYE_ADAS_RET_SUCCESS )
        {
            LOGE(LOG_TARGET, "$$ MEADAS >> SYNC parse FAIL \n");
            read_fail_cnt ++;
            evt_data->evt_code = eADAS_EVT__INVALID;
        }
        {
            read_fail_cnt = 0;
            LOGT(LOG_TARGET, "$$ MEADAS >> SYNC parse SUCCESS => sync_code [%d]\n", ret.sync_code);
            evt_data->evt_code = eADAS_EVT__NONE;
        }
    }
    else
        read_fail_cnt ++;

FINISH:
    // if (( read_fail_cnt % 5 ) == 0 )
        LOGT(LOG_TARGET, "$$ MEADAS >> ERR CNT :: [%d][%d]\n",  read_fail_cnt, MAX_ADAS_CHK_ERR);
    
    if ( read_fail_cnt > MAX_ADAS_CHK_ERR ) 
    {
        // todo something..
        LOGE(LOG_TARGET, "$$ MEADAS >> ERR CNT MAX !!! [%d][%d]\n",  read_fail_cnt, MAX_ADAS_CHK_ERR);
        read_fail_cnt = 0;
    }

    return 0;
}

