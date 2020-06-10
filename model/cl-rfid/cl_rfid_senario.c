<<<<<<< HEAD

#include <stdio.h>
#include <unistd.h>

#include <netcom.h>
#include <base/devel.h>
#include <base/sender.h>

#include "logd/logd_rpc.h"

#ifdef USE_KJTEC_RFID
#include "kjtec_rfid_cmd.h"
#include "kjtec_rfid_tools.h"
#define KJTEC_CONN_DISCONN_CHK_CNT		3

RFID_FIRMWARE_VER_T g_ver_info;
RFID_DEV_INFO_T g_rfid_dev_info;
#endif

#ifdef USE_SUP_RFID
#include "sup_rfid_tools.h"
#endif


#include "cl_rfid_tools.h"

#define GET_RFID_USER_INTERVAL_SEC		60


#define MAIN_STAT_MSG_PRINT_INTERVAL 	5

int g_need_to_rfid_ver_chk = 0;
int g_need_to_rfid_info = 0;


#define RFID_CHK_INTERVAL				60
#define MAX_CHK_RFID_WRITE_FAIL_CNT 	1

#define LOG_TARGET eSVC_MODEL


void rfid_main_senario_init()
{
#ifdef USE_KJTEC_RFID
    kjtec_rfid__flush_data(1);

    init_kjtec_rfid();
	set_fwdown_target_ver(FW_DOWNLOAD_FILE_LAST_VER_NUM);
	rfid_tool__set_senario_stat(e_RFID_INIT); 

    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
#endif
#ifdef USE_SUP_RFID
    sup_rfid__mgr_init("/dev/ttyHSL1", 9600);
#endif
}


void rfid_main_senario()
{
    static int rfid_main_loop_cnt = 0;
    static int main_rfid_chk_cnt = 0;
    
#ifdef USE_KJTEC_RFID
    static 	int rfid_read_fail_cnt = 0;
    static 	int rfid_write_user_data_fail_cnt = 0;

	// ------------------------------------------------------
    // 시나리오시작
    // -----------------------------------------------------
    if ( ( rfid_main_loop_cnt % MAIN_STAT_MSG_PRINT_INTERVAL ) == 0 )
        LOGI(LOG_TARGET, "[MAIN] pkt_stat [%s], rfid chk [%d]/[%d] \n", rfid_tool__get_senario_stat_str(), main_rfid_chk_cnt, rfid_tool__env_get_rfid_chk_interval()  );

    if (g_need_to_rfid_ver_chk == 0)
    {
        if ( kjtec_rfid__firmware_ver_info(&g_ver_info) == KJTEC_RFID_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[MAIN] kjtec version info [%s]\n", g_ver_info.data_result  );
            devel_webdm_send_log("RFID FW VER [%s]", g_ver_info.data_result);
            g_need_to_rfid_ver_chk = 1;
        }
        else
            LOGE(LOG_TARGET, "[MAIN] kjtec version info fail\n");
    }

    // 1. rfid 단말을 확인한다.
    if ( rfid_tool__get_senario_stat() == e_RFID_INIT )
    {
        g_need_to_rfid_info = 0;

        if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
        {
            rfid_tool__set_senario_stat(e_NEED_TO_RFID_USER_CHK);
            LOGE(LOG_TARGET, "[MAIN] RFID INIT success\r\n");
            devel_webdm_send_log("KJTEC CONN : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
            rfid_read_fail_cnt = 0;
            rfid_write_user_data_fail_cnt = 0;
            rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
        }
        else
        {
            rfid_tool__set_senario_stat(e_RFID_INIT);
            LOGE(LOG_TARGET, "[MAIN] RFID INIT FAIL [%d]/[%d]\r\n",rfid_read_fail_cnt, KJTEC_CONN_DISCONN_CHK_CNT);
            // 너무 자주보내면안됨.
            if ( rfid_read_fail_cnt++ == KJTEC_CONN_DISCONN_CHK_CNT )
                devel_webdm_send_log("KJTEC CONN FAIL!! 1 ");
            rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_NOK);
        }
    }

    // 2. 승객리스트를 웹에서 받아온다.
    if ( rfid_tool__get_senario_stat() == e_NEED_TO_RFID_USER_CHK )
    {
        rfid_write_user_data_fail_cnt = 0;
        g_need_to_rfid_info = 0;

        if ( rfid_tool__env_get_all_clear() == 1 )
            sender_add_data_to_buffer(PACKET_TYPE_HTTP_GET_PASSENGER_LIST, "0", ePIPE_2);
        else
            sender_add_data_to_buffer(PACKET_TYPE_HTTP_GET_PASSENGER_LIST, g_rfid_dev_info.saved_timestamp, ePIPE_2);

    }

    // 3. 모두받아왔다면, 승객을 rfid 에 넣는다.
    if ( rfid_tool__get_senario_stat() == e_RFID_DOWNLOAD_END )
    {
        int ret = kjtec_rfid_mgr__write_user_info(); // block! // 한참걸릴수있다.

        // 다운로드 성공했으니, 새로운 정보로 갱신한다.
        if ( ret == KJTEC_RFID_RET_SUCCESS )
        {
            RFID_DB_INFO_T result;
            memset(&result, 0x00, sizeof(result));
            kjtec_rfid__rfid_db_info(&result);
            // 새로 얻어와야 하기때문에.
            //memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
            
            if ( result.cmd_result == KJTEC_RFID_RET_SUCCESS )
            {
                g_rfid_dev_info.total_passenger_cnt = result.db_cnt;
                strcpy(g_rfid_dev_info.saved_timestamp,result.db_date);
                
                printf("db info [%d] / cnt [%d] / date [%s]\r\n", result.cmd_result, result.db_cnt, result.db_date);

                g_need_to_rfid_info = 0;
                devel_webdm_send_log("[MAIN] USER DOWN OK 1 : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
            }
            else
            {
                g_need_to_rfid_info = 1;
                devel_webdm_send_log("[MAIN] USER DOWN OK 2 : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
            }

            rfid_write_user_data_fail_cnt = 0;

        }
        else
        {
            LOGE(LOG_TARGET, "[MAIN] RFID WRITE FAIL [%d]/[%d]\r\n",rfid_write_user_data_fail_cnt, MAX_CHK_RFID_WRITE_FAIL_CNT);
            devel_webdm_send_log("USER DATA WRITE FAIL!! [%d]", rfid_write_user_data_fail_cnt);
            sleep(30);
            if ( ++rfid_write_user_data_fail_cnt > MAX_CHK_RFID_WRITE_FAIL_CNT ) 
            {
                devel_webdm_send_log("USER DATA WRITE FAIL!!");
                //rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_FAIL);
                rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS);
            }
        }

        g_need_to_rfid_ver_chk = 1;
    }

    if ( g_need_to_rfid_info == 1 )
    {
        RFID_DB_INFO_T result;
        memset(&result, 0x00, sizeof(result));
        kjtec_rfid__rfid_db_info(&result);
        if ( result.cmd_result == KJTEC_RFID_RET_SUCCESS )
        {
            g_rfid_dev_info.total_passenger_cnt = result.db_cnt;
            strcpy(g_rfid_dev_info.saved_timestamp,result.db_date);
            
            printf("db info [%d] / cnt [%d] / date [%s]\r\n", result.cmd_result, result.db_cnt, result.db_date);
            g_need_to_rfid_info = 0;
            devel_webdm_send_log("[MAIN] RFID DEV INFO : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
        }
        /*
        if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
        {
            
            
        }
        */
    }

    // 4. 모두 write도 성공했으니, 승객정보를 주기적으로 요청한다.
    if ( rfid_tool__get_senario_stat() == e_RFID_USER_INFO_CHK_READY ) 
    {
        if ( main_rfid_chk_cnt > rfid_tool__env_get_rfid_chk_interval() )
        {
            kjtec_rfid__dev_rfid_req();
            main_rfid_chk_cnt = 0;
        }
    }

    // rfid 살아있는지, 연결되어있는지 체크한다.
    if ( ( rfid_main_loop_cnt % RFID_CHK_INTERVAL ) == 0 )
    {

        // 정상동작중에만 체크한다.
        if ( ( rfid_tool__get_senario_stat() > e_RFID_INIT ) && ( rfid_tool__get_senario_stat() <= e_RFID_USER_INFO_CHK_READY ) )
        {
            int dev_stat = KJTEC_RFID_RET_SUCCESS;

            dev_stat = kjtec_rfid_mgr__alive_dev();

            // 연결 해제 됐다가, 다시 연결된경우.
            if ( ( dev_stat == KJTEC_RFID_RET_SUCCESS ) && ( rfid_read_fail_cnt > KJTEC_CONN_DISCONN_CHK_CNT ) )
            {
                rfid_read_fail_cnt = 0;
                rfid_tool__set_senario_stat(e_RFID_INIT);
                LOGI(LOG_TARGET, "[CONN CHK] KJTEC DISCONN -> CONN !!\r\n");
                devel_webdm_send_log("[CONN CHK] KJTEC DISCONN -> CONN !!");

                rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
            }
            // 정상연결중
            else if ( ( dev_stat == KJTEC_RFID_RET_SUCCESS ) && ( rfid_read_fail_cnt <= KJTEC_CONN_DISCONN_CHK_CNT ) )
            {
                rfid_read_fail_cnt = 0;
                LOGI(LOG_TARGET, "[CONN CHK] KJTEC CONN -> NORMAL STAT !!\r\n");

                rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
            }
            // 연결이상.
            else
            {
                LOGE(LOG_TARGET, "KJTEC CONN FAIL [%d]/[%d]\n", rfid_read_fail_cnt, KJTEC_CONN_DISCONN_CHK_CNT);
                if ( rfid_read_fail_cnt++ == KJTEC_CONN_DISCONN_CHK_CNT )
                    devel_webdm_send_log("[CONN CHK] KJTEC CONN FAIL!! 2 ");
                
                rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_NOK);
            }
        }

    }

    if ( rfid_tool__get_senario_stat() == e_RFID_FIRMWARE_DOWNLOAD_START )
    {
        char fw_filename[FW_NAME_MAX_LEN] = {0,};
        get_fwdown_target_path(fw_filename);
        kjtec_rfid_mgr__download_fw(fw_filename);
        rfid_tool__set_senario_stat(e_RFID_INIT); // 처음부터 루틴을 다시 태우도록
    }
#endif

    rfid_main_loop_cnt ++;
    main_rfid_chk_cnt++;
=======

#include <stdio.h>
#include <unistd.h>

#include <netcom.h>
#include <base/devel.h>
#include <base/sender.h>

#include "logd/logd_rpc.h"

#ifdef USE_KJTEC_RFID
#include "kjtec_rfid_cmd.h"
#include "kjtec_rfid_tools.h"
#define KJTEC_CONN_DISCONN_CHK_CNT		3

RFID_FIRMWARE_VER_T g_ver_info;
RFID_DEV_INFO_T g_rfid_dev_info;
#endif

#ifdef USE_SUP_RFID
#include "sup_rfid_tools.h"
#endif


#include "cl_rfid_tools.h"

#define GET_RFID_USER_INTERVAL_SEC		60


#define MAIN_STAT_MSG_PRINT_INTERVAL 	5

int g_need_to_rfid_ver_chk = 0;
int g_need_to_rfid_info = 0;


#define RFID_CHK_INTERVAL				60
#define MAX_CHK_RFID_WRITE_FAIL_CNT 	1

#define LOG_TARGET eSVC_MODEL


void rfid_main_senario_init()
{
#ifdef USE_KJTEC_RFID
    kjtec_rfid__flush_data(1);

    init_kjtec_rfid();
	set_fwdown_target_ver(FW_DOWNLOAD_FILE_LAST_VER_NUM);
	rfid_tool__set_senario_stat(e_RFID_INIT); 

    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
#endif
#ifdef USE_SUP_RFID
    sup_rfid__mgr_init("/dev/ttyHSL1", 9600);
#endif
}


void rfid_main_senario()
{
    static int rfid_main_loop_cnt = 0;
    static int main_rfid_chk_cnt = 0;
    
#ifdef USE_KJTEC_RFID
    static 	int rfid_read_fail_cnt = 0;
    static 	int rfid_write_user_data_fail_cnt = 0;

	// ------------------------------------------------------
    // 시나리오시작
    // -----------------------------------------------------
    if ( ( rfid_main_loop_cnt % MAIN_STAT_MSG_PRINT_INTERVAL ) == 0 )
        LOGI(LOG_TARGET, "[MAIN] pkt_stat [%s], rfid chk [%d]/[%d] \n", rfid_tool__get_senario_stat_str(), main_rfid_chk_cnt, rfid_tool__env_get_rfid_chk_interval()  );

    if (g_need_to_rfid_ver_chk == 0)
    {
        if ( kjtec_rfid__firmware_ver_info(&g_ver_info) == KJTEC_RFID_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[MAIN] kjtec version info [%s]\n", g_ver_info.data_result  );
            devel_webdm_send_log("RFID FW VER [%s]", g_ver_info.data_result);
            g_need_to_rfid_ver_chk = 1;
        }
        else
            LOGE(LOG_TARGET, "[MAIN] kjtec version info fail\n");
    }

    // 1. rfid 단말을 확인한다.
    if ( rfid_tool__get_senario_stat() == e_RFID_INIT )
    {
        g_need_to_rfid_info = 0;

        if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
        {
            rfid_tool__set_senario_stat(e_NEED_TO_RFID_USER_CHK);
            LOGE(LOG_TARGET, "[MAIN] RFID INIT success\r\n");
            devel_webdm_send_log("KJTEC CONN : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
            rfid_read_fail_cnt = 0;
            rfid_write_user_data_fail_cnt = 0;
            rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
        }
        else
        {
            rfid_tool__set_senario_stat(e_RFID_INIT);
            LOGE(LOG_TARGET, "[MAIN] RFID INIT FAIL [%d]/[%d]\r\n",rfid_read_fail_cnt, KJTEC_CONN_DISCONN_CHK_CNT);
            // 너무 자주보내면안됨.
            if ( rfid_read_fail_cnt++ == KJTEC_CONN_DISCONN_CHK_CNT )
                devel_webdm_send_log("KJTEC CONN FAIL!! 1 ");
            rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_NOK);
        }
    }

    // 2. 승객리스트를 웹에서 받아온다.
    if ( rfid_tool__get_senario_stat() == e_NEED_TO_RFID_USER_CHK )
    {
        rfid_write_user_data_fail_cnt = 0;
        g_need_to_rfid_info = 0;

        if ( rfid_tool__env_get_all_clear() == 1 )
            sender_add_data_to_buffer(PACKET_TYPE_HTTP_GET_PASSENGER_LIST, "0", ePIPE_2);
        else
            sender_add_data_to_buffer(PACKET_TYPE_HTTP_GET_PASSENGER_LIST, g_rfid_dev_info.saved_timestamp, ePIPE_2);

    }

    // 3. 모두받아왔다면, 승객을 rfid 에 넣는다.
    if ( rfid_tool__get_senario_stat() == e_RFID_DOWNLOAD_END )
    {
        int ret = kjtec_rfid_mgr__write_user_info(); // block! // 한참걸릴수있다.

        // 다운로드 성공했으니, 새로운 정보로 갱신한다.
        if ( ret == KJTEC_RFID_RET_SUCCESS )
        {
            RFID_DB_INFO_T result;
            memset(&result, 0x00, sizeof(result));
            kjtec_rfid__rfid_db_info(&result);
            // 새로 얻어와야 하기때문에.
            //memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
            
            if ( result.cmd_result == KJTEC_RFID_RET_SUCCESS )
            {
                g_rfid_dev_info.total_passenger_cnt = result.db_cnt;
                strcpy(g_rfid_dev_info.saved_timestamp,result.db_date);
                
                printf("db info [%d] / cnt [%d] / date [%s]\r\n", result.cmd_result, result.db_cnt, result.db_date);

                g_need_to_rfid_info = 0;
                devel_webdm_send_log("[MAIN] USER DOWN OK 1 : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
            }
            else
            {
                g_need_to_rfid_info = 1;
                devel_webdm_send_log("[MAIN] USER DOWN OK 2 : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
            }

            rfid_write_user_data_fail_cnt = 0;

        }
        else
        {
            LOGE(LOG_TARGET, "[MAIN] RFID WRITE FAIL [%d]/[%d]\r\n",rfid_write_user_data_fail_cnt, MAX_CHK_RFID_WRITE_FAIL_CNT);
            devel_webdm_send_log("USER DATA WRITE FAIL!! [%d]", rfid_write_user_data_fail_cnt);
            sleep(30);
            if ( ++rfid_write_user_data_fail_cnt > MAX_CHK_RFID_WRITE_FAIL_CNT ) 
            {
                devel_webdm_send_log("USER DATA WRITE FAIL!!");
                //rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_FAIL);
                rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS);
            }
        }

        g_need_to_rfid_ver_chk = 1;
    }

    if ( g_need_to_rfid_info == 1 )
    {
        RFID_DB_INFO_T result;
        memset(&result, 0x00, sizeof(result));
        kjtec_rfid__rfid_db_info(&result);
        if ( result.cmd_result == KJTEC_RFID_RET_SUCCESS )
        {
            g_rfid_dev_info.total_passenger_cnt = result.db_cnt;
            strcpy(g_rfid_dev_info.saved_timestamp,result.db_date);
            
            printf("db info [%d] / cnt [%d] / date [%s]\r\n", result.cmd_result, result.db_cnt, result.db_date);
            g_need_to_rfid_info = 0;
            devel_webdm_send_log("[MAIN] RFID DEV INFO : model [%s], user cnt [%d], time [%s]", g_rfid_dev_info.model_no , g_rfid_dev_info.total_passenger_cnt , g_rfid_dev_info.saved_timestamp );
        }
        /*
        if ( kjtec_rfid_mgr__dev_init_chk(&g_rfid_dev_info) == KJTEC_RFID_RET_SUCCESS )
        {
            
            
        }
        */
    }

    // 4. 모두 write도 성공했으니, 승객정보를 주기적으로 요청한다.
    if ( rfid_tool__get_senario_stat() == e_RFID_USER_INFO_CHK_READY ) 
    {
        if ( main_rfid_chk_cnt > rfid_tool__env_get_rfid_chk_interval() )
        {
            kjtec_rfid__dev_rfid_req();
            main_rfid_chk_cnt = 0;
        }
    }

    // rfid 살아있는지, 연결되어있는지 체크한다.
    if ( ( rfid_main_loop_cnt % RFID_CHK_INTERVAL ) == 0 )
    {

        // 정상동작중에만 체크한다.
        if ( ( rfid_tool__get_senario_stat() > e_RFID_INIT ) && ( rfid_tool__get_senario_stat() <= e_RFID_USER_INFO_CHK_READY ) )
        {
            int dev_stat = KJTEC_RFID_RET_SUCCESS;

            dev_stat = kjtec_rfid_mgr__alive_dev();

            // 연결 해제 됐다가, 다시 연결된경우.
            if ( ( dev_stat == KJTEC_RFID_RET_SUCCESS ) && ( rfid_read_fail_cnt > KJTEC_CONN_DISCONN_CHK_CNT ) )
            {
                rfid_read_fail_cnt = 0;
                rfid_tool__set_senario_stat(e_RFID_INIT);
                LOGI(LOG_TARGET, "[CONN CHK] KJTEC DISCONN -> CONN !!\r\n");
                devel_webdm_send_log("[CONN CHK] KJTEC DISCONN -> CONN !!");

                rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
            }
            // 정상연결중
            else if ( ( dev_stat == KJTEC_RFID_RET_SUCCESS ) && ( rfid_read_fail_cnt <= KJTEC_CONN_DISCONN_CHK_CNT ) )
            {
                rfid_read_fail_cnt = 0;
                LOGI(LOG_TARGET, "[CONN CHK] KJTEC CONN -> NORMAL STAT !!\r\n");

                rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_OK);
            }
            // 연결이상.
            else
            {
                LOGE(LOG_TARGET, "KJTEC CONN FAIL [%d]/[%d]\n", rfid_read_fail_cnt, KJTEC_CONN_DISCONN_CHK_CNT);
                if ( rfid_read_fail_cnt++ == KJTEC_CONN_DISCONN_CHK_CNT )
                    devel_webdm_send_log("[CONN CHK] KJTEC CONN FAIL!! 2 ");
                
                rfid_tool__set_rifd_dev_stat(RFID_CONN_STAT_NOK);
            }
        }

    }

    if ( rfid_tool__get_senario_stat() == e_RFID_FIRMWARE_DOWNLOAD_START )
    {
        char fw_filename[FW_NAME_MAX_LEN] = {0,};
        get_fwdown_target_path(fw_filename);
        kjtec_rfid_mgr__download_fw(fw_filename);
        rfid_tool__set_senario_stat(e_RFID_INIT); // 처음부터 루틴을 다시 태우도록
    }
#endif

    rfid_main_loop_cnt ++;
    main_rfid_chk_cnt++;
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}