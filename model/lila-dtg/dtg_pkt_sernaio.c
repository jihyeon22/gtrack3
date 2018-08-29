#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <board/board_system.h>

#include <base/config.h>
#include <base/gpstool.h>
#include <base/devel.h>

#include <at/at_util.h>

#include <util/tools.h>
#include <util/list.h>

#include <logd/logd_rpc.h>

#include <board/power.h>
#include <wrapper/dtg_log.h>
#include <wrapper/dtg_convtools.h>
#include <dtg_gtrack_tool.h>
#include <tacom_choyoung_protocol.h>

#include "callback.h"
#include "config.h"
#include "netcom.h"

#include "dtg_pkt_senario.h"
#include "lila_packet.h"
#include "lila_tools.h"

#define LOG_TARGET eSVC_MODEL



int lila_dtg_init()
{
    char* tmp_buff;
    int max_init_wait = 60;

    int ret_bufflen = 0;

    tacom_std_hdr_t std_hdr;
    tacom_std_data_t std_data;

    while(max_init_wait --)
    {
        cy_send_cmd_force_send(CY_DTG_CMD__DTG_DEV_INFO);

        ret_bufflen = taco_gtrack_tool__get_current_data(&tmp_buff);

        if ( ret_bufflen <= 0 )
        {
            LOGI(LOG_TARGET, "lila_dtg_init is fail... 1 retry...\r\n");
            sleep(1);
            continue;
        }

        if (taco_gtrack_tool__conv_dtg_current_data(tmp_buff, ret_bufflen, &std_hdr, &std_data) > 0)
        {
            LOGT(LOG_TARGET, "lila_dtg_init is success!!!!!...\r\n");
            lila_dtg__set_current_dtg_data(&std_hdr, &std_data);

            free(tmp_buff);

            break;
        }

        free(tmp_buff);

        sleep(1);
    }
    
    if ( max_init_wait < 0 )
        devel_webdm_send_log("DTG INIT CONN FAIL");

    return 0;
}


int lila_dtg__pkt_sernaio()
{
    /*
	int report_interval = 180; //3 min
	int collect_interval = 10;

    int condition_send = 0;
*/
    static int run_cnt = 0;
    int dtg_remain_cnt = taco_gtrack_tool__get_remain_cnt();
    int lila_dtg_data_total_cnt = get_lila_dtg_data_count();
/*
    report_interval = get_dtg_report_period();
    collect_interval = get_dtg_create_period();

    condition_send = report_interval/collect_interval;

    if(condition_send <= 0)
        condition_send = 1;
*/
    LOGT(LOG_TARGET, "dtg pkt senario : dtg data [%d] / dtg pkt [%d] \r\n", dtg_remain_cnt, lila_dtg_data_total_cnt);
    
    if ( dtg_remain_cnt >= LILA_DTG_PKT_BULK_CHK_CNT )
        lila_dtg__convert_dtg_data_to_pkt();

    //if ( lila_dtg_data_total_cnt >= (LILA_DTG_PKT_BULK_CHK_CNT * LILA_DTG_DTG_DATA_ACCUMUL))
    if (( run_cnt++ % 40 ) == 0 ) 
    {
        if ( lila_dtg_data_total_cnt >= (LILA_DTG_PKT_BULK_CHK_CNT * LILA_DTG_DTG_DATA_ACCUMUL))
        {
            lila_dtg__send_dtg_data();
            run_cnt = 1;
        }
        
    }

    
        

    return 0;
}



int lila_dtg__key_stat()
{
    tacom_std_data_t std_taco_data = {0,};
    int key_stat = 0;
    taco_gtrack_tool__get_cur_std_data(&std_taco_data);
    key_stat = std_taco_data.key_stat;;
    // printf(" bizincar_dtg__key_stat() => [%d]\r\n", key_stat);
    LOGI(LOG_TARGET, "%s > dtg key_stat [%d]\n", __func__, key_stat);
    return key_stat;
}

