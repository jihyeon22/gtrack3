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

#include "base/watchdog.h"
#include "netcom.h"

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "kjtec_rfid_cmd.h"
#include "kjtec_rfid_tools.h"
#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_MODEL

#define WRITE_RFID_MAX_READY_WAIT_CNT   10
#define WRITE_RFID_MAX_FAIL_CNT         4

int clear_main_watchdog();

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
        clear_main_watchdog();// 오래걸릴수있으므로 WATCHDOG CLEAR

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

        if ( ( ( total_frame_len + one_frame_len ) > (RFID_CMD_PASSENGER_STR_MAX_LEN-3) ) || ( remain_frame_cnt <= 0) )
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

            //printf(" type [%d] / write cmd [%s] / [%i]\r\n", pkt_frame_flag, tmp_frame_str1, i);
            clear_main_watchdog();

            if ( kjtec_rfid__dev_write_rfid_data(pkt_frame_flag, tmp_frame_str1) == KJTEC_RFID_RET_SUCCESS )
            {
                LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE SUCCESS -> SEQ [%d], [%d]/[%d] : write total [%d]  \r\n" ,seq_no, remain_frame_cnt, total_frame_len, i);
                printf( "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE SUCCESS -> SEQ [%d], [%d]/[%d] : write total [%d]  \r\n" ,seq_no, remain_frame_cnt, total_frame_len, i);

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
                LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE FAIL -> SEQ [%d] , [%d]/[%d] : write total [%d]  \r\n",seq_no, write_cmd_max_fail_cnt, MAX_WRITE_FAIL_RETRY_CNT, i);
                printf( "[KJTEC-RFID TOOL] WRITE USER INFO : WRITE FAIL -> SEQ [%d] , [%d]/[%d] : write total [%d]  \r\n",seq_no, write_cmd_max_fail_cnt, MAX_WRITE_FAIL_RETRY_CNT, i);
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
/*
    if ( ( rfid_tool__env_get_all_clear() == 1 ) && ( rfid_tool__user_info_total_cnt() == 0 ))
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
        }

        rfid_tool__set_senario_stat(e_NEED_TO_RFID_USER_CHK);
        return KJTEC_RFID_RET_SUCCESS;
    }
*/
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
    if ( e_RFID_DOWNLOAD_START != rfid_tool__get_senario_stat())
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

static _download_one_pkt(char* buff, int buff_len)
{
    int write_retry_cnt = 5; // 여러번 해봤자 헛수고;;;
    int write_one_pkt_ret = KJTEC_RFID_RET_FAIL;

    while (write_retry_cnt--)
    {
        write_one_pkt_ret = kjtec_rfid__firmware_write_one_pkt (buff, buff_len);
        if ( write_one_pkt_ret == KJTEC_RFID_RET_SUCCESS )
            return KJTEC_RFID_RET_SUCCESS;
        usleep(10000);
    }

    return KJTEC_RFID_RET_FAIL;
}

int kjtec_rfid_mgr__chk_need_to_fw_download()
{
    int max_fw_read_retry = 10;
    int get_fw_ver_success = 0;
    RFID_FIRMWARE_VER_T cur_ver_info;

    memset(&cur_ver_info, 0x00, sizeof(cur_ver_info));

    while(max_fw_read_retry--)
    {
        if ( kjtec_rfid__firmware_ver_info(&cur_ver_info) == KJTEC_RFID_RET_SUCCESS )
        {
            LOGI(LOG_TARGET, "[FWDOWN] kjtec version info [%s]\n", cur_ver_info.data_result  );
            devel_webdm_send_log("FW DOWN CHK => [%s]", cur_ver_info.data_result);
            get_fw_ver_success = 1;
            break;
        }
        else
            sleep(1);
    }

    if (get_fw_ver_success == 1)
    {
        char req_fwver[FW_NAME_MAX_LEN] = {0,};
        char req_fwver_str[FW_NAME_MAX_LEN] = {0,};

        get_fwdown_target_ver(req_fwver);

        if ( strcmp(req_fwver, "t13n") == 0 )
            strcpy(req_fwver_str, "1.3.0NBUS-t13");
        else if ( strcmp(req_fwver, "t13n") == 0 )
            strcpy(req_fwver_str, "1.3.0NBUS-t13");
        else if ( strcmp(req_fwver, "t13h") == 0 )
            strcpy(req_fwver_str, "1.3.0HBUS-t13");
        else if ( strcmp(req_fwver, "131n") == 0 )
            strcpy(req_fwver_str, "1.3.1NBUS");
        else if ( strcmp(req_fwver, "132n") == 0 )
            strcpy(req_fwver_str, "1.3.2NBUS");
        else if ( strcmp(req_fwver, "132n") == 0 )
            strcpy(req_fwver_str, "1.3.2NBUS");
        else if ( strcmp(req_fwver, "134A") == 0 )
            strcpy(req_fwver_str, "1.3.4ABUS");
        else if ( strcmp(req_fwver, "134H") == 0 )
            strcpy(req_fwver_str, "1.3.4HBUS");
        else
            sprintf(req_fwver_str,"1.3.0.BusSR-%s", req_fwver);
        
        printf("cur_ver_info.data_result [%s] / req_fwver_str [%s]\r\n",cur_ver_info.data_result, req_fwver_str);

        if ( strcmp(cur_ver_info.data_result, req_fwver_str) != 0 )
        {
            if ( strcmp(cur_ver_info.data_result, "1.3.0.BusSR-t8") == 0 )
                set_fwdown_chksum_offset(0);
            else if ( strcmp(cur_ver_info.data_result, "1.3.0.BusSR-t9") == 0 )
                set_fwdown_chksum_offset(0);
            else 
                set_fwdown_chksum_offset(6);
            
            devel_webdm_send_log("FW DOWN CHK => NEED TO FW DOWN / CHKSUM [%d] => [%s]",get_fwdown_chksum_offset(),req_fwver);

            return 1;
        }
        else
        {
            devel_webdm_send_log("FW DOWN CHK => ALREADY LASTEST : DOWN SKIP");
            kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN CHK => ALREADY LASTEST : DOWN SKIP");
            return -1;
        }
    }
    else
    {
        devel_webdm_send_log("FW DOWN CHK => FAIL , DOWN SKIP");
        kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN CHK => FAIL , DOWN SKIP");
        return -1;
    }

}


int kjtec_rfid_mgr__download_fw(char* path)
{
    int file_size = mds_api_get_file_size(path) - FW_DOWNLOAD_OFFSET_BYTE; // offset calc
    char read_buff[RFID_CMD_FIRMWARE_ONE_PKT_SIZE_BYTE] = {0,};

    int ret_val = 0;

    FILE *fp = NULL;
    int read_size = 0;
    int toal_write_size = 0;

    int write_one_pkt_ret = KJTEC_RFID_RET_FAIL;

    int watchdog_cnt = 0;

    if ( file_size <= 0)
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> TARGET FILE NOT EXIST [%s] / file_size [%d] \r\n", path, file_size);
        //kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN -> FAIL");
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }

    if ( kjtec_rfid_mgr__chk_need_to_fw_download() != 1)
    {
        rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_END);
        return KJTEC_RFID_RET_SUCCESS;
    }

    LOGT(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> START [%s] / file_size [%d] \r\n", path, file_size);
    
    devel_webdm_send_log("RFID FIRM DOWN START [%s] / file_size [%d] \r\n", path, file_size);
    {
        char send_msg[512] = {0,};
        char ver_str[FW_NAME_MAX_LEN] = {0,};
        get_fwdown_target_ver(ver_str);
        sprintf(send_msg, "FW DOWN -> START :: target ver [%s] ",ver_str);
        kjtec_rfid_mgr__download_sms_noti_msg(send_msg);
    }


    rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_ING);
    if ( file_size <= 0 )
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> ERR CASE 1 \r\n");
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }

    if (kjtec_rfid__firmware_write_start(file_size) != KJTEC_RFID_RET_SUCCESS)
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> ERR CASE 2 \r\n");
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }
    
    fp = fopen(path, "r");
    if(fp == NULL)
    {
        LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> ERR CASE 3 \r\n");
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }

    // offset move...
    fseek(fp, FW_DOWNLOAD_OFFSET_BYTE, SEEK_CUR);

    while(1)
    {
        memset(&read_buff, 0x00, RFID_CMD_FIRMWARE_ONE_PKT_SIZE_BYTE);
        
        write_one_pkt_ret = KJTEC_RFID_RET_FAIL;

        read_size = fread(read_buff, 1, RFID_CMD_FIRMWARE_ONE_PKT_SIZE_BYTE, fp);

        LOGI(LOG_TARGET, "[KJTEC-RFID TOOL] >> DOWNLOAD -> read [%d] / write[%d] / total [%d] \r\n" ,read_size, toal_write_size, file_size);

        if (toal_write_size == file_size )
        {
            devel_webdm_send_log("RFID FIRM DOWN WRITE SUCCESS\r\n");
            kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN -> SUCCESS");
            break;
        }

        if ( read_size <= 0 )
        {
            LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> READ FAIL? - 4 \r\n");
            devel_webdm_send_log("RFID FIRM DOWN FAIL 1 \r\n", path, file_size);
            break;
        }
        
        ret_val = _download_one_pkt(read_buff, read_size);
        if ( ret_val == KJTEC_RFID_RET_FAIL ) 
        {
            LOGE(LOG_TARGET, "[KJTEC-RFID TOOL] FW DOWNLOAD -> ERR CASE 5 \r\n");
            devel_webdm_send_log("RFID FIRM DOWN FAIL 2 \r\n", path, file_size);
            break;
        }

        toal_write_size += read_size;

        if ( watchdog_cnt++ & 10 )
            clear_main_watchdog();

    }

FINISH:
    if ( fp != NULL )
        fclose(fp);

    g_need_to_rfid_ver_chk = 0;

    if ( ret_val == KJTEC_RFID_RET_FAIL ) 
    {
        devel_webdm_send_log("RFID FIRM DOWN FAIL 3 [%s] [%d] [%s] \r\n", path, file_size, path);

        if ( file_size > 0 )
            kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN -> FAIL");
        else
            kjtec_rfid_mgr__download_sms_noti_msg("FW DOWN -> FAIL : file not exist");
    }

    rfid_tool__set_senario_stat(e_RFID_FIRMWARE_DOWNLOAD_END);

    kjtec_rfid_mgr__download_sms_noti_enable(0,"01000000000");
    
    return ret_val;
}

static int _g_sms_noti_enable = 0;
static char _g_sms_noti_phonenum[128] = {0,};

int kjtec_rfid_mgr__download_sms_noti_enable(int enable, char* phone_num)
{
    _g_sms_noti_enable = enable;
    memset(_g_sms_noti_phonenum, 0x00, sizeof(_g_sms_noti_phonenum));
    strcpy(_g_sms_noti_phonenum, phone_num);
    return 0;
}

int kjtec_rfid_mgr__download_sms_noti_msg(char* msg)
{
    if ( _g_sms_noti_enable == 0 )
        return 0;

    at_send_sms(_g_sms_noti_phonenum, msg);
}


static int _g_fwdown_chksum_offset = 6;
int set_fwdown_chksum_offset(int offset)
{
    _g_fwdown_chksum_offset = offset;
}

int get_fwdown_chksum_offset()
{
    return _g_fwdown_chksum_offset;
}


static char _g_fwdown_taget_ver[FW_NAME_MAX_LEN] = {0,};
int set_fwdown_target_ver(char* buff)
{
    memset(_g_fwdown_taget_ver, 0x00, sizeof(_g_fwdown_taget_ver));
    strcpy(_g_fwdown_taget_ver, buff);
    return 0;
}

int get_fwdown_target_ver(char* buff)
{
    strcpy(buff, _g_fwdown_taget_ver);
    return 0;
}

int get_fwdown_target_path(char* ver)
{
    snprintf(ver, FW_NAME_MAX_LEN, "%s/rfid_fw_%s.bin", FW_DOWNLOAD_FILE_PATH,_g_fwdown_taget_ver);
}
