<<<<<<< HEAD
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <logd_rpc.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "alloc2_nettool.h"

#include "alloc2_pkt.h"
#include "alloc2_senario.h"

#include "allkey_bcm_1.h"
#include "alloc2_bcm_mgr.h"


// bcm 에서 오는 이벤트 처리
int mdm_bcm_evt_proc(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list)
{
	printf("%s() - start evt code [%d]\r\n", __func__, evt_code);
	printf("%s() - start evt code [%d]\r\n", __func__, evt_code);
	printf("%s() - start evt code [%d]\r\n", __func__, evt_code);
	int pkt_evt_code = 0;

	switch(evt_code)
	{
		case e_bcm_evt_driver_call:
		{
			break;
		}
		case e_bcm_evt_monitor_off_door_open:
		case e_bcm_evt_monitor_off_door_close:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat1_door_open, stat_1) )
			{
				pkt_evt_code = e_evt_code_door_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_door_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			
			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
        }
        case e_bcm_evt_monitor_off_trunk_open:
        case e_bcm_evt_monitor_off_trunk_close:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat1_door_open, stat_1) )
			{
				pkt_evt_code = e_evt_code_trunk_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_door_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			
			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
		}
		case e_bcm_evt_small_shock:
		{
			break;
		}
		case e_bcm_evt_big_shock:
		{
			break;
		}
		case e_bcm_evt_monitor_on_door_stat:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat1_door_open, stat_1) )
			{
				pkt_evt_code = e_evt_code_door_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_door_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}

			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
		}
		case e_bcm_evt_monitor_on_trunk_stat:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat2_trunk_open, stat_2) )
			{
				pkt_evt_code = e_evt_code_trunk_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_trunk_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}

			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
		}
		case e_bcm_evt_monitor_on_hood_stat:
		{
			allkey_bcm_1_chk_stat(e_bcm_stat1_hood_open, stat_1);
			break;
        }
        case e_bcm_evt_knocksensor_set_id_req:
        {
            unsigned short knock_sensor_id = 0;
            unsigned short knock_sensor_pass = 0;

            if ( get_bcm_knocksensor_val_id_pass(&knock_sensor_id, &knock_sensor_pass) > 0 )
            {
                LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> set knocksensor id => pkt evt code [%d] : id[0x%x], pass[0x%x]  \r\n", 
                                 evt_code, pkt_evt_code, knock_sensor_id, knock_sensor_pass);
                allkey_bcm_ctr__knocksensor_set_passwd_evt_proc(knock_sensor_pass);
                allkey_bcm_ctr__knocksensor_set_id_evt_proc(knock_sensor_id);
            }
            else
                LOGE(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> set knocksensor id fail => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
            
            break;
        }
        case e_bcm_evt_knocksensor_set_timestamp_req:
        {
            LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> set knocksensor timestamp => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
            allkey_bcm_ctr__knocksensor_set_modemtime_evt_proc();
            break;
        }
        default :
        //#ifdef BCM_EVT_DEGUG_LOG
        //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt proc fail", BCM_EVT_DBG_LOG_MAX_SIZE);
        //#endif
            LOGE(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> pkt evt code [%d] :: invalid\r\n", evt_code, pkt_evt_code);
            break;
	}
    
    LOGT(eSVC_MODEL, " --- [BCM EVT] - code [%d] => pkt evt code [%d] : end bye bye...\r\n", evt_code, pkt_evt_code);

	return ALLKEY_BCM_RET_SUCCESS;
}


// 1초에 한번씩불린다.
void chk_allkey_bcm()
{
	static int chk_cnt = 0;
	static int fail_cnt = 0;

    static int last_door_lock = -1;

	int cmd_ret = ALLKEY_BCM_RET_FAIL;
    int pkt_evt_code = 0;
//	ALLKEY_BCM_1_INFO_T cur_allkey_bcm_info;

//	memset( &cur_allkey_bcm_info, 0x00, sizeof(ALLKEY_BCM_1_INFO_T) );
	LOGT(eSVC_MODEL, "[ALLKEY MGR] chk cnt [%d]/[%d]\r\n",chk_cnt,CHK_ALLKEY_BCM_INTERVAL);
	
	if ( chk_cnt++ > CHK_ALLKEY_BCM_INTERVAL)
	{
		//allkey_bcm_cmd__get_stat(&cur_allkey_bcm_info);
        ALLKEY_BCM_1_DEV_T dev_stat = {0,};
		cmd_ret = allkey_bcm_cmd__get_stat(&dev_stat);

        if ( last_door_lock == -1 )
            last_door_lock = dev_stat.door_lock_stat;

        if ( last_door_lock != dev_stat.door_lock_stat)
        {
            if ( dev_stat.door_lock_stat == 1 )
                pkt_evt_code = e_evt_code_door_lock;
            else
                pkt_evt_code = e_evt_code_door_unlock;
            
            sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));

            last_door_lock = dev_stat.door_lock_stat;
        }

		chk_cnt = 0;
	}
	else
	{
		return;
	}

	if ( cmd_ret == ALLKEY_BCM_RET_SUCCESS )
//	if ( cur_allkey_bcm_info.init_stat == 1 )
	{
		LOGI(eSVC_MODEL, "[ALLKEY MGR] obd chk success\r\n");
		if ( fail_cnt > CHK_ALLKEY_BCM_FAIL_CNT )
			devel_webdm_send_log("[ALLKEY MGR] allkey re-comm success\r\n");
		fail_cnt = 0;
		return;
	}
	
	fail_cnt++;
	
	LOGE(eSVC_MODEL, "[ALLKEY MGR] chk fail\r\n");

	if ( fail_cnt == CHK_ALLKEY_BCM_FAIL_CNT )
	{
		int evt_code = e_evt_code_dev_bcm_err;

		devel_webdm_send_log("[ALLKEY MGR] allkey comm fail\r\n");
		sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
		sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
	}

}





=======
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <logd_rpc.h>

#include <netcom.h>
#include <callback.h>
#include <config.h>

#include "alloc2_nettool.h"

#include "alloc2_pkt.h"
#include "alloc2_senario.h"

#include "allkey_bcm_1.h"
#include "alloc2_bcm_mgr.h"


// bcm 에서 오는 이벤트 처리
int mdm_bcm_evt_proc(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list)
{
	printf("%s() - start evt code [%d]\r\n", __func__, evt_code);
	printf("%s() - start evt code [%d]\r\n", __func__, evt_code);
	printf("%s() - start evt code [%d]\r\n", __func__, evt_code);
	int pkt_evt_code = 0;

	switch(evt_code)
	{
		case e_bcm_evt_driver_call:
		{
			break;
		}
		case e_bcm_evt_monitor_off_door_open:
		case e_bcm_evt_monitor_off_door_close:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat1_door_open, stat_1) )
			{
				pkt_evt_code = e_evt_code_door_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_door_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			
			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
        }
        case e_bcm_evt_monitor_off_trunk_open:
        case e_bcm_evt_monitor_off_trunk_close:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat1_door_open, stat_1) )
			{
				pkt_evt_code = e_evt_code_trunk_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_door_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			
			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
		}
		case e_bcm_evt_small_shock:
		{
			break;
		}
		case e_bcm_evt_big_shock:
		{
			break;
		}
		case e_bcm_evt_monitor_on_door_stat:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat1_door_open, stat_1) )
			{
				pkt_evt_code = e_evt_code_door_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_door_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> door close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}

			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
		}
		case e_bcm_evt_monitor_on_trunk_stat:
		{
			if ( allkey_bcm_1_chk_stat(e_bcm_stat2_trunk_open, stat_2) )
			{
				pkt_evt_code = e_evt_code_trunk_open;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk open => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}
			else
			{
				pkt_evt_code = e_evt_code_trunk_close;
				LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> trunk close => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
			}

			sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));
			break;
		}
		case e_bcm_evt_monitor_on_hood_stat:
		{
			allkey_bcm_1_chk_stat(e_bcm_stat1_hood_open, stat_1);
			break;
        }
        case e_bcm_evt_knocksensor_set_id_req:
        {
            unsigned short knock_sensor_id = 0;
            unsigned short knock_sensor_pass = 0;

            if ( get_bcm_knocksensor_val_id_pass(&knock_sensor_id, &knock_sensor_pass) > 0 )
            {
                LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> set knocksensor id => pkt evt code [%d] : id[0x%x], pass[0x%x]  \r\n", 
                                 evt_code, pkt_evt_code, knock_sensor_id, knock_sensor_pass);
                allkey_bcm_ctr__knocksensor_set_passwd_evt_proc(knock_sensor_pass);
                allkey_bcm_ctr__knocksensor_set_id_evt_proc(knock_sensor_id);
            }
            else
                LOGE(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> set knocksensor id fail => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
            
            break;
        }
        case e_bcm_evt_knocksensor_set_timestamp_req:
        {
            LOGT(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> set knocksensor timestamp => pkt evt code [%d] \r\n", evt_code, pkt_evt_code);
            allkey_bcm_ctr__knocksensor_set_modemtime_evt_proc();
            break;
        }
        default :
        //#ifdef BCM_EVT_DEGUG_LOG
        //    mds_api_write_time_and_log_maxsize(BCM_EVT_DBG_LOG_PATH, "evt proc fail", BCM_EVT_DBG_LOG_MAX_SIZE);
        //#endif
            LOGE(eSVC_MODEL, " +++ [BCM EVT] - code [%d] ==> pkt evt code [%d] :: invalid\r\n", evt_code, pkt_evt_code);
            break;
	}
    
    LOGT(eSVC_MODEL, " --- [BCM EVT] - code [%d] => pkt evt code [%d] : end bye bye...\r\n", evt_code, pkt_evt_code);

	return ALLKEY_BCM_RET_SUCCESS;
}


// 1초에 한번씩불린다.
void chk_allkey_bcm()
{
	static int chk_cnt = 0;
	static int fail_cnt = 0;

    static int last_door_lock = -1;

	int cmd_ret = ALLKEY_BCM_RET_FAIL;
    int pkt_evt_code = 0;
//	ALLKEY_BCM_1_INFO_T cur_allkey_bcm_info;

//	memset( &cur_allkey_bcm_info, 0x00, sizeof(ALLKEY_BCM_1_INFO_T) );
	LOGT(eSVC_MODEL, "[ALLKEY MGR] chk cnt [%d]/[%d]\r\n",chk_cnt,CHK_ALLKEY_BCM_INTERVAL);
	
	if ( chk_cnt++ > CHK_ALLKEY_BCM_INTERVAL)
	{
		//allkey_bcm_cmd__get_stat(&cur_allkey_bcm_info);
        ALLKEY_BCM_1_DEV_T dev_stat = {0,};
		cmd_ret = allkey_bcm_cmd__get_stat(&dev_stat);

        if ( last_door_lock == -1 )
            last_door_lock = dev_stat.door_lock_stat;

        if ( last_door_lock != dev_stat.door_lock_stat)
        {
            if ( dev_stat.door_lock_stat == 1 )
                pkt_evt_code = e_evt_code_door_lock;
            else
                pkt_evt_code = e_evt_code_door_unlock;
            
            sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &pkt_evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,pkt_evt_code));

            last_door_lock = dev_stat.door_lock_stat;
        }

		chk_cnt = 0;
	}
	else
	{
		return;
	}

	if ( cmd_ret == ALLKEY_BCM_RET_SUCCESS )
//	if ( cur_allkey_bcm_info.init_stat == 1 )
	{
		LOGI(eSVC_MODEL, "[ALLKEY MGR] obd chk success\r\n");
		if ( fail_cnt > CHK_ALLKEY_BCM_FAIL_CNT )
			devel_webdm_send_log("[ALLKEY MGR] allkey re-comm success\r\n");
		fail_cnt = 0;
		return;
	}
	
	fail_cnt++;
	
	LOGE(eSVC_MODEL, "[ALLKEY MGR] chk fail\r\n");

	if ( fail_cnt == CHK_ALLKEY_BCM_FAIL_CNT )
	{
		int evt_code = e_evt_code_dev_bcm_err;

		devel_webdm_send_log("[ALLKEY MGR] allkey comm fail\r\n");
		sender_add_data_to_buffer(e_mdm_stat_evt_fifo, &evt_code, get_pkt_pipe_type(e_mdm_stat_evt_fifo,evt_code));
		sender_add_data_to_buffer(e_mdm_gps_info_fifo, NULL, get_pkt_pipe_type(e_mdm_gps_info_fifo,0));
	}

}





>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
