#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <base/devel.h>
#include <base/sender.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include "netcom.h"

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "kjtec_rfid_cmd.h"
#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_MODEL

#define WRITE_RFID_MAX_READY_WAIT_CNT   10
#define WRITE_RFID_MAX_FAIL_CNT         4

int kjtec_rfid_mgr__dev_init_chk(RFID_DEV_INFO_T* info)
{
	RFID_DEV_INFO_T cur_info;
    int time_stamp_int = 0;

    memset(&cur_info, 0x00, sizeof(cur_info));
    kjtec_rfid__dev_wakeup(&cur_info);

    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] DEV INIT CHK ++ \n");

    if ( cur_info.cmd_result != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] DEV INIT CHK :: FAIL WAKEUP CMD \n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( ( strcmp(cur_info.saved_timestamp, "0" ) == 0 ) ||
         ( strcmp(cur_info.saved_timestamp, "00" ) == 0 ) ||
         ( strcmp(cur_info.saved_timestamp, "000" ) == 0 ) ||
         ( strcmp(cur_info.saved_timestamp, "0000" ) == 0 ) ||
         ( strcmp(cur_info.saved_timestamp, "000000000000" ) == 0 ) ||
         ( atoi(cur_info.saved_timestamp) == 0 ) )
    {
        rfid_tool__env_set_all_clear(1);

        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] NEED TO INIT [%s]\n", cur_info.saved_timestamp);
        memcpy(info, &cur_info, sizeof(cur_info));

        return KJTEC_RFID_RET_SUCCESS;
    }

    if ( strlen(cur_info.saved_timestamp) != 12 )
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] DEV INIT CHK :: INVALID TIME STAMP \n");
        rfid_tool__env_set_all_clear(1);
        return KJTEC_RFID_RET_FAIL;
    }

    // 여기까지오면 12자리스트링을 모두갖고옴.
    rfid_tool__env_set_all_clear(0);
    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] DEV INIT CHK -- SUCCESS TIMESTAMP [%s] \n", cur_info.saved_timestamp);

    memcpy(info, &cur_info, sizeof(cur_info));

    return KJTEC_RFID_RET_SUCCESS;

}


int kjtec_rfid_mgr__write_to_dev_user_info(int all_erase)
{
    int total_user_cnt = rfid_tool__user_info_total_cnt();
    RFID_USER_INFO_T cur_user_info;

    char one_frame_str[RFID_CMD_PASSENGER_STR_MAX_LEN] = {0,};
    char total_frame_str[RFID_CMD_PASSENGER_STR_MAX_LEN] = {0,};

    int total_frame_len = 0;
    int one_frame_len = 0;

    int remain_frame_cnt = 0;
    int seq_no = 0;

    int i = 0;
    int write_cmd_max_fail_cnt = 0;

    remain_frame_cnt = total_user_cnt;
    
    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO [%d] - START \r\n", all_erase);

    if ( all_erase == 1 )
    {
        RIFD_DATA_ALL_CLR_T all_clr_result;
        memset(&all_clr_result, 0x00, sizeof(all_clr_result));
        if ( kjtec_rfid__dev_rfid_all_clear(&all_clr_result) == KJTEC_RFID_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : ALL ERASE SUCCESS \r\n");
        }
        else
        {
            // 모두지우는데 실패. 그러므로 다시 시도하기 위해서 리턴
            LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : ALL ERASE FAIL RETURN \r\n");
            return KJTEC_RFID_RET_FAIL;
        }

    }

    while(1)
    {
        if ( write_cmd_max_fail_cnt > MAX_WRITE_FAIL_RETRY_CNT )
        {
            return KJTEC_RFID_RET_FAIL;
        }

        memset(&cur_user_info, 0x00, sizeof(cur_user_info));
        rfid_tool__user_info_get(i, &cur_user_info);

        one_frame_len = 0;
        one_frame_len = sprintf(one_frame_str, "[%s,%d,%d,%d, ]",
                                                cur_user_info.rfid_uid,
                                                cur_user_info.day_limit,
                                                cur_user_info.is_use,
                                                //cur_user_info.boarding_cont
                                                0 );
/*
        printf(" ----------------------\r\n");
        printf(" >> target idx => [%d]\r\n",i);
        printf(" >> total_frame_len => [%d]\r\n",total_frame_len);
        printf(" >> one_frame_len => [%d]\r\n",one_frame_len);
        printf(" >> remain_frame_cnt => [%d]\r\n",remain_frame_cnt);
        printf(" >> seq_no => [%d]\r\n",seq_no);
        printf(" >> one_frame_str => [%s]\r\n",one_frame_str);
*/

        if ( ( ( total_frame_len + one_frame_len ) > RFID_CMD_PASSENGER_STR_MAX_LEN ) || ( remain_frame_cnt <= 0) )
        {
            int pkt_frame_type = 0;
            int pkt_frame_flag = 0;

            char tmp_frame_str1[RFID_CMD_PASSENGER_STR_MAX_LEN+RFID_CMD_PASSENGER_STR_PADDING] = {0,};
            char tmp_frame_str2[RFID_CMD_PASSENGER_STR_MAX_LEN] = {0,};

            if ( seq_no == 0 )
                pkt_frame_type = RFID_USER_INFO_FRAME__START;
            else  
                pkt_frame_type = RFID_USER_INFO_FRAME__BODY;
            
            // 마지막 프레임
            if ( remain_frame_cnt <= 0 )
            {
                if ( seq_no == 0 ) // 처음이자 마지막이니 한개프레임
                    pkt_frame_type = RFID_USER_INFO_FRAME__ONLY_ONE_PKT;
                else
                    pkt_frame_type = RFID_USER_INFO_FRAME__END;
            }

            // 전송시점에선 끝의 "," 문자를 제거한다.
            strncpy(tmp_frame_str2, total_frame_str, strlen(total_frame_str)-1);

            // 온전한 프레임으로 만든다.
            if ( pkt_frame_type == RFID_USER_INFO_FRAME__START )
            {
                pkt_frame_flag = 1;
                sprintf(tmp_frame_str1, "%s,",tmp_frame_str2);
            }
            else if ( pkt_frame_type == RFID_USER_INFO_FRAME__BODY )
            {
                pkt_frame_flag = 1;
                sprintf(tmp_frame_str1, "%s,",tmp_frame_str2);
            }
            else if ( pkt_frame_type == RFID_USER_INFO_FRAME__END )
            {
                pkt_frame_flag = 2;
                sprintf(tmp_frame_str1, "%s]",tmp_frame_str2);
            }
            else if ( pkt_frame_type == RFID_USER_INFO_FRAME__ONLY_ONE_PKT )
            {
                pkt_frame_flag = 2;
                sprintf(tmp_frame_str1, "[%s]",tmp_frame_str2);
            }

            if ( kjtec_rfid__dev_write_rfid_data(pkt_frame_flag, tmp_frame_str1) == KJTEC_RFID_RET_SUCCESS )
            {
                LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE SUCCESS -> SEQ [%d], [%d]/[%d] \r\n" ,seq_no, remain_frame_cnt, total_frame_len);
                printf( "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE SUCCESS -> SEQ [%d], [%d]/[%d] \r\n" ,seq_no, remain_frame_cnt, total_frame_len);

                total_frame_len = 0;
                memset(&total_frame_str, 0x00, sizeof(total_frame_str));
                seq_no ++;

                if ( remain_frame_cnt <= 0)
                {
                    break;
                }
                write_cmd_max_fail_cnt = 0;

            }
            else
            {
                LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE FAIL -> SEQ [%d] , [%d]/[%d] \r\n",seq_no, write_cmd_max_fail_cnt, MAX_WRITE_FAIL_RETRY_CNT);
                printf( "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE FAIL -> SEQ [%d] , [%d]/[%d] \r\n",seq_no, write_cmd_max_fail_cnt, MAX_WRITE_FAIL_RETRY_CNT);
                write_cmd_max_fail_cnt++;
                sleep(1);
                continue;
            }
        }
        else
        {
            total_frame_len +=  sprintf(total_frame_str + total_frame_len, "%s,", one_frame_str);
            //printf(" >>  total str => [%s]\r\n", total_frame_str);
            i++;
            remain_frame_cnt--;
        }
    }

    // 여기까지 왔으면 정상 다운로드 성공
    printf( "[KJTEC-RFID TOOL] WRITE USER INFO : ALL SUCCESS!!!! END FUNC\r\n");
    LOGT(LOG_TARGET,"[KJTEC-RFID TOOL] WRITE USER INFO : ALL SUCCESS!!!! END FUNC\r\n");
    return KJTEC_RFID_RET_SUCCESS;
    
}


int kjtec_rfid_mgr__write_user_info()
{
    RIFD_CHK_READY_T dev_ready;
    RFID_DEV_INFO_T cur_info;
    int max_ready_retry = WRITE_RFID_MAX_READY_WAIT_CNT;
    int ret;
    static int fail_cnt = 0;

    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA ++ \r\n");

    memset(&dev_ready, 0x00, sizeof(dev_ready));
    memset(&cur_info, 0x00, sizeof(cur_info));

    // 기존에 FAIL 났던것을 체크한다.
    /*
    if ( fail_cnt > WRITE_RFID_MAX_FAIL_CNT) 
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - max fail cnt [%d]/[%d] \r\n", fail_cnt, WRITE_RFID_MAX_FAIL_CNT);
        printf( "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - max fail cnt [%d]/[%d] \r\n", fail_cnt, WRITE_RFID_MAX_FAIL_CNT);
        return KJTEC_RFID_RET_FAIL;
    }*/

    if ( rfid_tool__get_senario_stat() != e_RFID_DOWNLOAD_END )
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - SENARIO INVAILD [%d]\r\n", rfid_tool__get_senario_stat());
        printf( "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - SENARIO INVAILD [%d]\r\n", rfid_tool__get_senario_stat());
        fail_cnt++;
        return KJTEC_RFID_RET_FAIL;
    }

    if ( rfid_tool__user_info_total_cnt() == 0 )
    {
        rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS);
        LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA -- SUCCESS : NO NEED TO WRITE \r\n");
        return KJTEC_RFID_RET_SUCCESS;
    }

    // 1. 먼저 wakeup 패킷을 보낸다.
    kjtec_rfid__dev_wakeup(&cur_info);
    if ( cur_info.cmd_result != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - WAKEUP CMD CMD FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    // 2. ready 인지 체크한다.
    while(max_ready_retry--)
    {
        memset(&dev_ready, 0x00, sizeof(dev_ready));
        ret = kjtec_rfid__dev_ready_chk(&dev_ready);

        if ( dev_ready.data_result == 1 )
            break;

        sleep(2);
    }

    if ( dev_ready.data_result != 1 ) 
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - READY CHK FAIL\r\n");
        fail_cnt++;
        sleep(2);
        return KJTEC_RFID_RET_FAIL;
    }

    

    // 3. 실제 write 한다.
    if ( (rfid_tool__user_info_total_cnt() > 0) )
    {
        int all_clr_flag = rfid_tool__env_get_all_clear();

        LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : START :: USER CNT [%d] / ALLCLR [%d] \r\n", rfid_tool__user_info_total_cnt(), all_clr_flag);

        // 1. 시나리오를 start 로 변경
        rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_START);
        ret = kjtec_rfid_mgr__write_to_dev_user_info(all_clr_flag);

        if ( ret == KJTEC_RFID_RET_FAIL )
        {
            // 2. 시나리오 fail 재시도.
            rfid_tool__set_senario_stat(e_RFID_DOWNLOAD_END);
            LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : FAIL - RFID WRITE FAIL \r\n");
        }
        else
        {
            // 3. 시나리오 성공 
            rfid_tool__set_senario_stat(e_RFID_USER_INFO_WRITE_TO_DEV_SUCCESS);
        }
    }
    
    if ( ret == KJTEC_RFID_RET_FAIL )
    {
        fail_cnt++;
        return KJTEC_RFID_RET_FAIL;
    }

    fail_cnt = 0;
    rfid_tool__env_set_rfid_chk_interval(RFID_CHK_DEFAULT_INTERVAL_SEC);

    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER DATA : ALL SUCCESS END!! \r\n");
    return KJTEC_RFID_RET_SUCCESS;
}


int kjtec_rfid_mgr__clr_all_user_data()
{
    rfid_tool__env_set_all_clear(1);
    rfid_tool__set_senario_stat(e_NEED_TO_RFID_USER_CHK);

    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] RFID ALL CLR DATA :: REWRITE \r\n");

    return KJTEC_RFID_RET_SUCCESS;
}


int kjtec_rfid_mgr__alive_dev()
{
    RIFD_CHK_READY_T dev_ready;
    int ret = 0;

    memset(&dev_ready, 0x00, sizeof(dev_ready));
    ret = kjtec_rfid__dev_ready_chk(&dev_ready);

    if ( dev_ready.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] CHK ALIVE :: OK \r\n");
        return KJTEC_RFID_RET_SUCCESS;
    }
    else
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] CHK ALIVE :: FAIL \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

}

