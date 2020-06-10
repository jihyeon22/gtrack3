<<<<<<< HEAD
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
#include <at/at_util.h>

#include "kjtec_rfid_cmd.h"
#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_MODEL

static int _rfid_fd = KJTEC_RFID_INVAILD_FD;

pthread_t tid_kjtec_rfid_thread;
static pthread_mutex_t kjtec_rfid_mutex = PTHREAD_MUTEX_INITIALIZER;
static int _g_run_kjtec_rfid_thread_run = 1;

static RFID_DEV_INFO_T                  g_rfid_dev_info;
static RIFD_PASSENGER_DATA_INFO_T       g_rfid_passenger_data_info;
static RIFD_CHK_READY_T                 g_rfid_chk_ready;
static RIFD_DATA_ALL_CLR_T              g_rfid_all_clr;
static RFID_SAVE_PASSENGER_DATA_T       g_rfid_save_passenger_data;
static RFID_FIRMWARE_VER_T              g_rfid_firmware_ver;
static RFID_FIRMWARE_DOWN_PKT_T         g_rfid_firm_down_pkt;
static RFID_DB_INFO_T                   g_rfid_db_info;


static int _parse_cmd__wakeup(const char* buf);
static int _parse_cmd__passenger_data(const char* buf);
static int _parse_cmd__chk_ready(const char* buf);
static int _parse_cmd__data_all_clr(const char* buf);
static int _parse_cmd__save_passenger_data(const char* buf);
static int _parse_cmd__get_passenger_data(const char* buf);
static int _parse_cmd__firmware_ver_info(const char* buf);
static int _parse_cmd__rfid_db_info(const char* buf);
static int _parse_cmd__firmware_write_one_pkt(const char* buf);

int kjtec_rfid__dev_rfid_req_clr();
int clear_main_watchdog();

char _check_xor_sum(char* write_buff, int len)
{
	unsigned char val = 0;
	int i;
	
	for(i = 0; i < len ; i++)
	{
		val = val ^ write_buff[i];
	}
	
	return val;
}

static int _kjtec_rfid_dev_init()
{
    int max_chk_cnt = KJTEC_RFID_MAX_CHK_DEV_CNT;
    int ret_val = KJTEC_RFID_RET_FAIL;

    while(max_chk_cnt--)
    {
        if ( _rfid_fd == KJTEC_RFID_INVAILD_FD )
            _rfid_fd = mds_api_init_uart(KJTEC_RFID_UART_DEVNAME, KJTEC_RFID_UART_BAUDRATE);
        
        if ( _rfid_fd > 0 ) 
        {
            ret_val = KJTEC_RFID_RET_SUCCESS;
            break;
        }
        else
        {
            ret_val = KJTEC_RFID_RET_FAIL;
            _rfid_fd = KJTEC_RFID_INVAILD_FD;
        }
    }
    return ret_val;
}



// =========================================================================
// kjtec cmd 처리부분
// =========================================================================
static void _kjtec_rfid_cmd_proc(char* data_buff, int buff_len)
{
    int code = 0;
    int read_idx = 0;

    int cmd_cnt = 0;

    char buff[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char tmp_buff[RIFD_READ_BUFF_SIZE + 1] = {0,};

    char tmp_str1[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char tmp_str2[RIFD_READ_BUFF_SIZE + 1] = {0,};

    char tmp_str_bak[RIFD_READ_BUFF_SIZE + 1] = {0,};

    //printf("_kjtec_rfid_cmd_proc is start!! [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
    if ( data_buff == NULL )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 1");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    if (( buff_len > RIFD_READ_BUFF_SIZE ) ||  ( buff_len <= 0 ))
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 2");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //-------------------------------------------------------------------------------
    // 스페이스제거
    //-------------------------------------------------------------------------------
    memset(&buff, 0x00, sizeof(buff));
    memcpy(&buff, data_buff, buff_len);
    cmd_cnt = buff_len;
/*
    cmd_cnt = mds_api_remove_char(data_buff, buff, RIFD_READ_BUFF_SIZE, ' ');
    if ( cmd_cnt <= 0 )
    {
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    buff_len = cmd_cnt;
*/
    // -------------------------------------------------------------------------------
    //printf("_kjtec_rfid_cmd_proc is start!! [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
    if ( buff_len < 4 )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 3");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //if ( buff_len == 4 )
    {
        if ( ( buff[0] == 0x10 ) &&
             ( buff[1] == 0x10 ) &&
             ( buff[2] == 0x06 ) &&
             ( buff[3] == 0x00 ))
        {
           read_idx = 4;
        }
        else
        {
           read_idx = 0;
        }
    }

    if ( buff_len < (5+read_idx+1+1) )
    {
        // printf("kjtec error [%s](%s)\r\n",buff,buff_len);
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 4");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    // chk header..
    if ( ( buff[0+read_idx] != 0x10 ) &&
         ( buff[1+read_idx] != 0x10 ) &&
         ( buff[2+read_idx] != 0xff ))
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 5");
        //printf("_kjtec_rfid_cmd_proc is start!! [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    // get data

    code = buff[4 +read_idx];
    memcpy(&tmp_str1, buff + 5 + read_idx, buff_len -5 -1 - read_idx);

    //-------------------------------------------------------------------------------
    // 쓰레기값 제거
    //-------------------------------------------------------------------------------
    memset(&tmp_buff, 0x00, sizeof(tmp_buff));
    cmd_cnt = mds_api_remove_etc_char(tmp_str1, tmp_buff, cmd_cnt);
    if ( cmd_cnt <= 0 )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 6");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    memcpy(&tmp_str1, tmp_buff, cmd_cnt);
    memset(&tmp_buff, 0x00, sizeof(tmp_buff));

    cmd_cnt = mds_api_remove_char(tmp_str1, tmp_buff, RIFD_READ_BUFF_SIZE, ' ');
    if ( cmd_cnt <= 0 )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 7");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //if ( code != 0x51)
#if 0
    { 
        printf("  >> [KJTEC RFID] parse cmd resp ++ \r\n");
        printf("  >> seqno [%d]\r\n", buff[3 +read_idx]);
        printf("  >> code [0x%x]\r\n", buff[4 +read_idx]);
        printf("  >> data len [%d]\r\n", buff[5 +read_idx]);

        printf("  >> data  [%s]\r\n", tmp_buff);
        printf("  >> [KJTEC RFID] parse cmd resp -- \r\n");
    }
#endif

    // -------------------------------------------------------------------------------

    switch ( code )
    {
        case RFID_CMD_ID_RESP___INIT_WAKEUP:
        {
            _parse_cmd__wakeup(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__PASSENGER_DATA_INFO:
        {
            _parse_cmd__passenger_data(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__CHK_READY:
        {
            _parse_cmd__chk_ready(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__DATA_ALL_CLR:
        {
            _parse_cmd__data_all_clr(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__SAVE_PASSENGER_DATA:
        case 0x33:
        {
            _parse_cmd__save_passenger_data(tmp_buff);

            break;
        }
        case RFID_CMD_ID_RESP__GET_PASSENGER_DATA:
        {
            char tmp_str3[RIFD_READ_BUFF_SIZE] = {0,};
            static int passenger_parse_fail_cnt = 0;
            //char* test_str = "+[32:[223A05,1,20170901130801,1],33:[FB5305,1,20170901130802,1],34:[55C807,1,20170901130804,1],35:[023F81,1,20170901130805,1],]";
            //memset(&tmp_str2, 0x00, sizeof(tmp_str2));
            //strcpy(tmp_str2, test_str);
            // 씨발놈들 앞에 쓰레기문자가 올때가 있다. 
            // 대체 스펙서대로 되어있는게 한개도 없다. 어휴
            char* tmp_str_p_2 = strstr(tmp_buff,"[");
            //char* tmp_str_p_2 = "[13:[C019001626035001,1,20180129180246,0],14:[C019001625645001,1,20180129180247,1],15:[C019001626305001,1,20180129180249,1],16:[C019001624732001,1,20180129180256,0],],0],]    ";
            if ( ( tmp_str_p_2 != NULL ) && ( _parse_cmd__get_passenger_data(tmp_str_p_2) == KJTEC_RFID_RET_SUCCESS ) )
            {
                //usleep(10000);
                kjtec_rfid__dev_rfid_req_clr();
                passenger_parse_fail_cnt = 0;
            }
            else
            {
                passenger_parse_fail_cnt++;
                devel_webdm_send_log("passenger data parse fail => %s", tmp_str_p_2);
            }

            if ( passenger_parse_fail_cnt > RIFD_MAX_READ_USER_INFO_TRY_FAIL_CNT )   
            {
                devel_webdm_send_log("passenger data parse fail - 2");                
                kjtec_rfid__dev_rfid_req_clr();
            }
            

            break;
        }
        case RFID_CMD_ID_RESP__FIRMWARE_VER_INFO:
        {
            _parse_cmd__firmware_ver_info(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__RFID_DB_INFO:
        {
             _parse_cmd__rfid_db_info(tmp_buff);
            break;
        }
        case RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_WRITE_RET:
        {
            _parse_cmd__firmware_write_one_pkt(tmp_buff);
            break;
        }
        default:
        {
            break;
        }
    }
    // flush data..
    kjtec_rfid__flush_data(0);
}

// =============================================================
// kjtec rfid 커맨드 내리기
// =============================================================
static int _kjtec_rfid_cmd(int auto_lock, unsigned char cmd_code, char* cmd_data, int cmd_data_len, void* recv_data)
{
    int uart_ret = 0;
    int ret_val = 0;

    char kjtec_rfid_send_cmd[RIFD_READ_BUFF_SIZE] = {0,};
    char kjtec_rfid_recv_data[RIFD_READ_BUFF_SIZE] = {0,};

    char* tmp_recv_buff_p = NULL;
    
    int read_len = 0;
//    int to_read = sizeof(kjtec_rfid_recv_data);
    int read_retry_cnt = KJTEC_RFID_UART_READ_RETRY_CNT;
    
    int cmd_write_total_len = 0;
    int cmd_write_cmd_len = 0;

    int sleep_timing = KJTEC_RFID_CMD__SLEEP_INTERAL_MIL * 1000;

    if ( auto_lock )
    {
        pthread_mutex_lock(&kjtec_rfid_mutex);
    }

    if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
    {
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }
    
    // make command : common header
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x10;  // 0
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x10;  // 1
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0xff;  // 2

    // seq_no 
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 3 

    // command code
    kjtec_rfid_send_cmd[cmd_write_total_len++] = cmd_code;  // 4
    
    // data len
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 5

    cmd_write_cmd_len = cmd_data_len;

    strncpy(kjtec_rfid_send_cmd + cmd_write_total_len, cmd_data, cmd_write_cmd_len);

    cmd_write_total_len += cmd_write_cmd_len;

    kjtec_rfid_send_cmd[5] = cmd_write_cmd_len; // data len idx ==> 5 // hard coding

    // chk sum
    kjtec_rfid_send_cmd[cmd_write_total_len++] = _check_xor_sum(cmd_data, cmd_write_cmd_len);

    // do not ret chk
    mds_api_uart_write(_rfid_fd, kjtec_rfid_send_cmd, cmd_write_total_len);

    // mds_api_debug_hexdump_buff(kjtec_rfid_send_cmd, cmd_write_total_len);

    // 승객데이터는 무조건 쓰래드에서 읽는걸로
    if ( cmd_code != RFID_CMD_ID_REQ__GET_PASSENGER_DATA)
    {
        while(read_retry_cnt--)
        {
            tmp_recv_buff_p = kjtec_rfid_recv_data;

            uart_ret =  mds_api_uart_read(_rfid_fd, tmp_recv_buff_p + read_len,  (RIFD_READ_BUFF_SIZE - read_len - 1), KJTEC_RFID_UART_READ_TIMEOUT);

            if ( uart_ret > 0 )
            {
                read_len += uart_ret;
            }
            
            if ( read_len > 9 )
                break;

            if ( (RIFD_READ_BUFF_SIZE - read_len - 1) <= 0 )
                break;
            
        }
    }

FINISH:
    if ( auto_lock )
    {
        pthread_mutex_unlock(&kjtec_rfid_mutex);
    }

    if ( read_len > 9 )
    {
        tmp_recv_buff_p = kjtec_rfid_recv_data;
        _kjtec_rfid_cmd_proc(tmp_recv_buff_p+4, read_len-4 );
    }

    // userdata 쓸때는 sleep 없다.
    if ( cmd_code == RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA )
        sleep_timing = 0;

    usleep(sleep_timing);

    return ret_val;
}

// =============================================================
// kjtec rfid 커맨드 내리기
// =============================================================
static int _kjtec_rfid_cmd2(int auto_lock, unsigned char cmd_code, char* cmd_data, int cmd_data_len, int chksum_offset)
{
    int uart_ret = 0;
    int ret_val = 0;

    char kjtec_rfid_send_cmd[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char kjtec_rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    
    char* tmp_recv_buff_p = NULL;

    int read_len = 0;
//    int to_read = sizeof(kjtec_rfid_recv_data);
    int read_retry_cnt = KJTEC_RFID_UART_READ_RETRY_CNT;
    
    int cmd_write_total_len = 0;
    int cmd_write_cmd_len = 0;

    int sleep_timing = 0 * 1000;

    if ( auto_lock )
    {
        pthread_mutex_lock(&kjtec_rfid_mutex);
    }

    if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
    {
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }
    

    cmd_write_cmd_len = cmd_data_len;

    memcpy(&kjtec_rfid_send_cmd, cmd_data, cmd_data_len);

    cmd_write_total_len += cmd_write_cmd_len;

    kjtec_rfid_send_cmd[cmd_write_total_len++] = _check_xor_sum(cmd_data + chksum_offset, cmd_write_cmd_len - chksum_offset);

    mds_api_uart_write(_rfid_fd, kjtec_rfid_send_cmd, cmd_write_total_len);

    //mds_api_debug_hexdump_buff(kjtec_rfid_send_cmd, cmd_write_total_len);

    while(read_retry_cnt--)
    {
        tmp_recv_buff_p = kjtec_rfid_recv_data;

	    uart_ret =  mds_api_uart_read(_rfid_fd, tmp_recv_buff_p + read_len,  (RIFD_READ_BUFF_SIZE - read_len - 1), KJTEC_RFID_UART_READ_TIMEOUT);

        if ( uart_ret > 0 )
        {
            read_len += uart_ret;
        }
        
        if (strstr(kjtec_rfid_recv_data, "Download Result,0," ) != NULL)
            break;
        
        if (strstr(kjtec_rfid_recv_data, "Download Result,1," ) != NULL)
            break;

        if ( ( RIFD_READ_BUFF_SIZE - read_len - 1) <= 0 )
            break;
    }


FINISH:
    if ( auto_lock )
    {
        pthread_mutex_unlock(&kjtec_rfid_mutex);
    }

    if ( read_len > 9 )
    {
        tmp_recv_buff_p = kjtec_rfid_recv_data;
        _kjtec_rfid_cmd_proc(tmp_recv_buff_p, read_len );
    }

    return ret_val;
}

void kjtec_rfid__flush_data(int timeout)
{
    unsigned char rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    int uart_ret = 0;
    // pthread_mutex_lock(&kjtec_rfid_mutex);
    while(1)
    {
        if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
            break;
        
        uart_ret = mds_api_uart_read(_rfid_fd, rfid_recv_data,  RIFD_READ_BUFF_SIZE - 1, timeout);
        if ( uart_ret <= 0 )
            break;
    
    }
    //pthread_mutex_unlock(&kjtec_rfid_mutex);
}
// ==============================================================
// read thread
// ==============================================================
void kjtec_rfid__read_thread(void)
{
    char rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char* tmp_recv_buff_p = NULL;

    // flush data..
    kjtec_rfid__flush_data(1);

    while(_g_run_kjtec_rfid_thread_run)
    {
        int uart_ret = 0;
        int read_size = 0;

        if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        pthread_mutex_lock(&kjtec_rfid_mutex);

        memset(rfid_recv_data, 0x00, RIFD_READ_BUFF_SIZE);

        while(1)
        {
            tmp_recv_buff_p = rfid_recv_data;

            uart_ret =  mds_api_uart_read2(_rfid_fd, tmp_recv_buff_p + read_size,  ( RIFD_READ_BUFF_SIZE - read_size - 1 ), KJTEC_RFID_UART_READ_TIMEOUT2_USEC);

            if ( uart_ret > 0 )
            {
                read_size += uart_ret;
            }
            else
                break;

            if ( ( RIFD_READ_BUFF_SIZE - read_size - 1 ) <= 0 )
                break;
        }

        pthread_mutex_unlock(&kjtec_rfid_mutex);

        if ( read_size > 0 )
        {
            // mds_api_debug_hexdump_buff(rfid_recv_data, uart_ret);
            tmp_recv_buff_p = rfid_recv_data;
            _kjtec_rfid_cmd_proc(tmp_recv_buff_p, read_size );
        }
        else
        {
            sleep(1);
        }

        usleep(500); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함
    }
}

void init_kjtec_rfid()
{
    _g_run_kjtec_rfid_thread_run = 1;
    pthread_create(&tid_kjtec_rfid_thread, NULL, kjtec_rfid__read_thread, NULL);

    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
    memset(&g_rfid_passenger_data_info, 0x00, sizeof(g_rfid_passenger_data_info) );
    memset(&g_rfid_chk_ready, 0x00, sizeof(g_rfid_chk_ready) );
    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr) );
    memset(&g_rfid_save_passenger_data, 0x00, sizeof(g_rfid_save_passenger_data) );
    memset(&g_rfid_firmware_ver, 0x00, sizeof(g_rfid_firmware_ver));
    memset(&g_rfid_firm_down_pkt, 0x00, sizeof(g_rfid_firm_down_pkt));
    memset(&g_rfid_db_info, 0x00, sizeof(g_rfid_db_info));


} 


// ----------------------------------------------------------------------
// parser : 승객데이터
// ----------------------------------------------------------------------
static int _parse_cmd__passenger_data(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    int char_cnt = 0;

    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    char tmp_str[RIFD_READ_BUFF_SIZE] = {0,};

    g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    strcpy(buffer, buf);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    char_cnt = mds_api_remove_char(buffer, tmp_str, sizeof(buffer), '/');
    if ( char_cnt <= 0 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    memset(&buffer, 0x00, sizeof(buffer));
    strcpy(buffer, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    char_cnt = mds_api_remove_char(buffer, tmp_str, sizeof(buffer), ':');
    if ( char_cnt <= 0 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    memset(&buffer,0x00, sizeof(buffer));
    strcpy(buffer, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    char_cnt = mds_api_remove_char(buffer, tmp_str, sizeof(buffer), '.');
    if ( char_cnt <= 0 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    memset(&buffer,0x00, sizeof(buffer));
    strcpy(buffer, tmp_str);

    p_cmd = buffer;

    if ( p_cmd == NULL)
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL)     
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL)
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    if ( strlen(tr) <= 2 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    strcpy(g_rfid_passenger_data_info.saved_timestamp, tr+2);

    g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
    g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_SUCCESS;

    return KJTEC_RFID_RET_SUCCESS;
}


// -------------------------------------------------------------------------
// cmd : dev wakeup
// -------------------------------------------------------------------------
int kjtec_rfid__dev_wakeup(RFID_DEV_INFO_T* result)
{
    int write_len = 0;
 	time_t t;
    struct tm *lt;
    char  cmd_data_buff[RIFD_READ_BUFF_SIZE] = {0,};
    char dev_phone_num[128] ={0,};

    int max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP\r\n");
	//struct tm* tm_ptr;

    if((t = time(NULL)) == -1) {
        perror("get_modem_time_tm() call error\r\n");
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 1 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 1 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if((lt = localtime(&t)) == NULL) {
        perror("get_modem_time_tm() call error\r\n");
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

	char   time_str[26];
    
	sprintf(time_str, "%02d%02d%02d%02d%02d%02d", lt->tm_year+1900,
					lt->tm_mon+1,
					lt->tm_mday,
					lt->tm_hour,
					lt->tm_min,
					lt->tm_sec);

    if ( lt->tm_year+1900 < 2016 )
    {
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 [%d]\r\n", lt->tm_year+1900);
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 [%d]\r\n", lt->tm_year+1900);
        return KJTEC_RFID_RET_FAIL;
    }

    if ( at_get_phonenum(dev_phone_num, sizeof(dev_phone_num)) == AT_RET_SUCCESS)
        write_len += sprintf(cmd_data_buff+write_len, "%s", dev_phone_num);
    else
        write_len += sprintf(cmd_data_buff+write_len, "%s", "01299998888");

//    write_len += sprintf(cmd_data_buff+write_len, "%s", "01222605817");
    write_len += sprintf(cmd_data_buff+write_len, "%s", ",");
    write_len += sprintf(cmd_data_buff+write_len, "%s", time_str);
    
    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info));
    memset(&g_rfid_passenger_data_info, 0x00, sizeof(g_rfid_passenger_data_info));

    g_rfid_dev_info.cmd_result = -1;
    g_rfid_passenger_data_info.cmd_result = -1;
    
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__INIT_WAKEUP, cmd_data_buff, strlen(cmd_data_buff), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_dev_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            //printf("[KJTEC RFID] write cmd 1 [%s] -> resp success\r\n", __func__);
            break;
        }
        //printf("[KJTEC RFID] write cmd 1 [%s] -> resp wait...\r\n", __func__);
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);

        if ( max_cmd_wait_time & 20 )
            clear_main_watchdog();
    }

    if ( ( g_rfid_dev_info.cmd_result != KJTEC_RFID_RET_SUCCESS ) && ( g_rfid_dev_info.data_result != KJTEC_RFID_RET_SUCCESS ))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 3 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 3 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_passenger_data_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd 2 [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd 2 [%s] -> resp wait...\r\n", __func__);
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( ( g_rfid_passenger_data_info.cmd_result != KJTEC_RFID_RET_SUCCESS ) && ( g_rfid_passenger_data_info.data_result != KJTEC_RFID_RET_SUCCESS ) )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 4 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 4 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    result->cmd_result = KJTEC_RFID_RET_SUCCESS;
    strcpy(result->model_no, g_rfid_dev_info.model_no);
    result->total_passenger_cnt = g_rfid_dev_info.total_passenger_cnt;
    strcpy(result->saved_timestamp, g_rfid_passenger_data_info.saved_timestamp);

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - SUCCESS [%s][%d][%s]\r\n",result->model_no, result->total_passenger_cnt, result->saved_timestamp);

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__wakeup(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    
    
    if ( buf == NULL)
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    memset(buffer, 0x00, sizeof(buffer));
    strcpy(buffer, buf);
    p_cmd = buffer;

    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) 
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    strcpy(g_rfid_dev_info.model_no,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) 
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) 
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    g_rfid_dev_info.total_passenger_cnt = atoi(tr);

    g_rfid_dev_info.data_result = KJTEC_RFID_RET_SUCCESS;
    g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;

    return KJTEC_RFID_RET_SUCCESS;
}


// -------------------------------------------------------------------------
// cmd : dev ready chk
// -------------------------------------------------------------------------

int kjtec_rfid__dev_ready_chk(RIFD_CHK_READY_T* result)
{
    int max_cmd_wait_time = 0;
    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ARE YOU READY?\r\n");

    memset(&g_rfid_chk_ready, 0x00, sizeof(g_rfid_chk_ready));
    g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__CHK_READY, "Are you ready?", strlen( "Are you ready?"), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_chk_ready.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_chk_ready.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ARE YOU READY? - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( g_rfid_chk_ready.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        result->cmd_result = g_rfid_chk_ready.cmd_result;
        result->data_result = g_rfid_chk_ready.data_result;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ARE YOU READY? - SUCCESS [%d][%d]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
    
}

static int _parse_cmd__chk_ready(const char* buf)
{
    g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_chk_ready.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL)
    {
        g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_SUCCESS;
    }
    
    if ( strstr(buf, "Ready") != NULL )
    {
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
}


// -------------------------------------------------------------------------
// cmd : dev rfid saved data all clear
// -------------------------------------------------------------------------

int kjtec_rfid__dev_rfid_all_clear(RIFD_DATA_ALL_CLR_T* result)
{
    int max_cmd_wait_time;
    int sleep_time_usec = 200000;

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO\r\n");

    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr));
    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__DATA_ALL_CLR, "Init Data", strlen("Init Data"), NULL);
    
    max_cmd_wait_time = 300; // 300 * 2sec = 10min
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_all_clr.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }

        LOGE(LOG_TARGET, "[KJTEC RFID] write cmd [%s] -> [%d] : resp wait... [%d] usec sleep\r\n", __func__, max_cmd_wait_time, sleep_time_usec);
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(sleep_time_usec);

        if ( max_cmd_wait_time & 20 )
            clear_main_watchdog();
    }

    if ( g_rfid_all_clr.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( g_rfid_all_clr.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        result->cmd_result = g_rfid_all_clr.cmd_result;
        result->data_result = g_rfid_all_clr.data_result;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO - SUCCESS [%d][%d]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__data_all_clr(const char* buf)
{
    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_all_clr.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_SUCCESS;
    }


    if (( strstr(buf, "Success") != NULL ))
    {
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
    
}


// -------------------------------------------------------------------------
// cmd : rfid user info request
// -------------------------------------------------------------------------

int kjtec_rfid__dev_rfid_req()
{
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: REQ TAGED RFID USER INFO\r\n");
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__GET_PASSENGER_DATA, "Bus Log Request", strlen("Bus Log Request"), NULL);
    return 0;
}

/* *****************************************************************************
// 승객 데이터 파싱부분
***************************************************************************** */
static int _get_boarding_info(const char* buf, RFID_BOARDING_INFO_T* boarding_info)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    
    RFID_BOARDING_INFO_T tmp_boarding;

    memset(&tmp_boarding, 0x00, sizeof(tmp_boarding));

    memset(buffer, 0x00, sizeof(buffer));
    strcpy(buffer, buf);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    strcpy(tmp_boarding.rfid_uid,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    tmp_boarding.boarding = 1; // 1 fix

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    strcpy(tmp_boarding.date,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    
    {
        int tmp_stat = atoi(tr); // rfid 에서 읽은값 :: 0 :등록사용자, 1:미등록사용자
        if ( tmp_stat == 0 )
            tmp_boarding.chk_result = 1; // 서버에 전송할 값 : 1 :등록사용자, 0:미등록사용자
        else
            tmp_boarding.chk_result = 0; // 서버에 전송할 값 : 1 :등록사용자, 0:미등록사용자
    }

    // 여기까지 왔으면 정상 파싱
    memcpy(boarding_info, &tmp_boarding, sizeof(tmp_boarding));
/*
    printf(" >> boarding_info->rfid_uid [%s]\r\n", boarding_info->rfid_uid );
    printf(" >> boarding_info->boarding [%d]\r\n", boarding_info->boarding );
    printf(" >> boarding_info->date [%s]\r\n", boarding_info->date );
    printf(" >> boarding_info->chk_result [%d]\r\n", boarding_info->chk_result );
*/
    return KJTEC_RFID_RET_SUCCESS;

}

static int _parse_cmd__get_passenger_data(const char* buf)
{
    int buf_len;

    int prefix_cnt = 0;
    int suffix_cnt = 0;

    char real_data[RIFD_READ_BUFF_SIZE] = {0,};
    char one_data[64][1024];
    int one_data_cnt = 0;

    char* tmp_p = NULL;
    char* tmp_one_start_p = NULL;
    
    int i = 0;

    if ( buf == NULL )
    {
        //devel_webdm_send_log("get passenger err - 1 ");
        return KJTEC_RFID_RET_FAIL;
    }

    buf_len = strlen(buf);
    
    if ( buf_len > 512 )
    {
        //devel_webdm_send_log("get passenger err - 2 ");
        return KJTEC_RFID_RET_FAIL;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: GET PASSENGER START [%d] \"%s\" \r\n", buf_len, buf);

    if ( strstr(buf, "...") != NULL )
    {
        // rfid check interval change : end data.
        rfid_tool__env_set_rfid_chk_interval(RFID_CHK_DEFAULT_INTERVAL_SEC);
        return KJTEC_RFID_RET_SUCCESS;
    }
    // devel_webdm_send_log("get passenger chk ++ ");
    memset(&one_data, 0x00, sizeof(one_data));

    if ( buf_len < 5 )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 1\r\n");
        //devel_webdm_send_log("get passenger err - %d", __LINE__);
        return KJTEC_RFID_RET_FAIL;
    }

    // check essential data..
    //buf[0];
    if (( buf[0] != '[' ) || (buf[buf_len-1] != ']'))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 2\r\n");
        //devel_webdm_send_log("get passenger err - %d", __LINE__);
        //return KJTEC_RFID_RET_FAIL;
    }

    strncpy(real_data, buf, buf_len);

    // chk prefix
    tmp_p = real_data;
    while(1)
    {
        if ( tmp_p == NULL )
            break;
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_PREFIX_STR);
        prefix_cnt++ ;
    }

    // chk prefix
    tmp_p = real_data;
    while(1)
    {
        if ( tmp_p == NULL )
            break;
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_SUFFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_SUFFIX_STR);
        suffix_cnt++ ;
    }

    if ( ( prefix_cnt != suffix_cnt ) || ( prefix_cnt<= 0 ) || ( suffix_cnt <= 0 ))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 3\r\n");
        //devel_webdm_send_log("get passenger err - %d", __LINE__);
        //return KJTEC_RFID_RET_FAIL;
    }

    // split one 
    tmp_p = real_data;
    buf_len = strlen(real_data);
    while(1)
    {
        if ( tmp_p == NULL )
            break;
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_one_start_p = tmp_p;

        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_SUFFIX_STR);
        if ( tmp_p == NULL )
            break;
        
        strncpy(one_data[one_data_cnt], tmp_one_start_p, strlen(tmp_one_start_p) - strlen(tmp_p));
        one_data_cnt++;
    }

    if ( one_data_cnt <= 0 )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 4\r\n");
        devel_webdm_send_log("get passenger err - %d", __LINE__);
        return KJTEC_RFID_RET_FAIL;
    }

    RFID_BOARDING_MGR_T boarding_list;
    int total_boarding_list = 0;
    memset(&boarding_list, 0x00, sizeof(boarding_list));

    // check data validation
    for ( i = 0 ; i < one_data_cnt ; i++ )
    {
        int char_cnt = 0;
        RFID_BOARDING_INFO_T tmp_boarding;
        
        char_cnt = mds_api_count_char(one_data[i], strlen(one_data[i]), GET_PASSENGER_DATA_ARGUMENT_SPLIT_CHAR);
        if ( char_cnt != (GET_PASSENGER_DATA_ARGUMENT_CNT-1) )
        {
            LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 4 [%d]/[%d]\r\n", char_cnt, (GET_PASSENGER_DATA_ARGUMENT_CNT-1));
            devel_webdm_send_log("get passenger err - %d", __LINE__);
            return KJTEC_RFID_RET_FAIL;
        }
        // printf(" ---> one data : [%d] => [%s] \r\n", i, one_data[i]);
        if ( _get_boarding_info(one_data[i], &tmp_boarding) == KJTEC_RFID_RET_FAIL)
            continue;
        
        memcpy(&boarding_list.boarding_info[total_boarding_list], &tmp_boarding, sizeof(tmp_boarding));
        LOGI(LOG_TARGET,"---------------------------\r\n");
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].rfid_uid [%s]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].rfid_uid );
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].boarding [%d]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].boarding );
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].date [%s]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].date );
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].chk_result [%d]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].chk_result );
        total_boarding_list++;
    }

    boarding_list.rfid_boarding_idx = total_boarding_list;

    sender_add_data_to_buffer(PACKET_TYPE_HTTP_SET_BOARDING_LIST, &boarding_list, ePIPE_2);

    // rfid check interval change : more data.
    rfid_tool__env_set_rfid_chk_interval(RFID_CHK_READ_MORE_INTERVAL_SEC);

    LOGT(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: GET PASSENGER SUCCESS AND SEND TO PKT \r\n");
    return KJTEC_RFID_RET_SUCCESS;
}





// -------------------------------------------------------------------------
// cmd : rfid user info request
// -------------------------------------------------------------------------
int kjtec_rfid__dev_rfid_req_clr()
{
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: REQ TAGED RFID USER INFO -> CLR RESP\r\n");
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__GET_PASSENGER_DATA_SUCCESS, "Bus Log Result,1,", strlen("Bus Log Result,1,"), NULL);
    return 0;
}


// -------------------------------------------------------------------------
// cmd : write rfid data.
// -------------------------------------------------------------------------
int kjtec_rfid__dev_write_rfid_data(int flag, char* rfid_user_str)
{
    int max_cmd_wait_time;
    int sleep_time_usec = 100000;

    char cmd_str[RFID_CMD_PASSENGER_STR_MAX_LEN] = {0,};

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO\r\n");

    if (rfid_user_str == NULL)
        return KJTEC_RFID_RET_FAIL;
        
    printf("flag [%d] rfid_user_str [%d]/[%d] is ==> \"%s\" \r\n", flag, strlen(rfid_user_str),RFID_CMD_PASSENGER_STR_MAX_LEN, rfid_user_str);

    if ( strlen(rfid_user_str) > RFID_CMD_PASSENGER_STR_MAX_LEN )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO - FAIL CASE 99\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    cmd_str[0] = flag;

    sprintf(cmd_str + 1, "%s", rfid_user_str);

    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr));
    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA, cmd_str, strlen(cmd_str), NULL);

    if ( flag == 2 ) // flag 2 :: RFID_USER_INFO_FRAME__END / RFID_USER_INFO_FRAME__ONLY_ONE_PKT
        max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 20;
    else
        max_cmd_wait_time = 30;

    while(max_cmd_wait_time--)
    {
        if ( g_rfid_save_passenger_data.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        LOGE(LOG_TARGET, "[KJTEC RFID] write cmd [%s] flag [%d] ->  resp wait... [%d]/[%d] \r\n", __func__, flag, sleep_time_usec, max_cmd_wait_time );
        printf( "[KJTEC RFID] write cmd [%s] flag [%d] ->  resp wait... [%d]/[%d] \r\n", __func__, flag, sleep_time_usec, max_cmd_wait_time );
        usleep(sleep_time_usec);

        if ( sleep_time_usec <  2000000 )
            sleep_time_usec = sleep_time_usec + 100000;
        
        if ( max_cmd_wait_time & 10 )
            clear_main_watchdog();
    }

    if ( g_rfid_save_passenger_data.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO - SUCCESS \r\n");
    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__save_passenger_data(const char* buf)
{
    g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
        g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_SUCCESS;
        return KJTEC_RFID_RET_SUCCESS;
    }

    if ( strstr(buf, "DataResult,1,") != NULL )
    {
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
  
}



// -------------------------------------------------------------------------
// cmd : dev firmware ver info
// -------------------------------------------------------------------------

int kjtec_rfid__firmware_ver_info(RFID_FIRMWARE_VER_T* result)
{
    int max_cmd_wait_time = 0;
    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: CHK VER?\r\n");

    memset(&g_rfid_firmware_ver, 0x00, sizeof(g_rfid_firmware_ver));
    g_rfid_firmware_ver.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__FIRMWARE_VER_INFO, "FW Version?", strlen("FW Version?"), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_firmware_ver.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_firmware_ver.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: CHK VER? - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( g_rfid_firmware_ver.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        result->cmd_result = g_rfid_firmware_ver.cmd_result;
        strcpy(result->data_result, g_rfid_firmware_ver.data_result);
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: CHK VER? - SUCCESS [%d][%s]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__firmware_ver_info(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};

    RFID_FIRMWARE_VER_T tmp_version;

    if ( buf == NULL )
        return KJTEC_RFID_RET_FAIL;

    printf("%s() parse => [%s]\r\n", __func__, buf);
    //FWversionis,1.3.0.BusSR-t5, 

    if ( strstr(buf, "FWversionis") == NULL )
    {
    //    printf ("_parse_cmd__chk_ready is success\r\n");
        strcpy(g_rfid_firmware_ver.data_result, "FAIL");
        return KJTEC_RFID_RET_FAIL;
    }

    memset(&tmp_version, 0x00, sizeof(tmp_version));

    memset(buffer, 0x00, sizeof(buffer));

    strcpy(buffer, buf);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    strcpy(tmp_version.data_result, tr);
    
    tmp_version.cmd_result = KJTEC_RFID_RET_SUCCESS;
    // 여기까지 왔으면 정상 파싱
    memcpy(&g_rfid_firmware_ver, &tmp_version, sizeof(tmp_version));
/*
    printf(" >> boarding_info->rfid_uid [%s]\r\n", boarding_info->rfid_uid );
    printf(" >> boarding_info->boarding [%d]\r\n", boarding_info->boarding );
    printf(" >> boarding_info->date [%s]\r\n", boarding_info->date );
    printf(" >> boarding_info->chk_result [%d]\r\n", boarding_info->chk_result );
*/
    return KJTEC_RFID_RET_SUCCESS;
}

// -------------------------------------------------------------------------
// cmd : dev firmware ver info
// -------------------------------------------------------------------------

int kjtec_rfid__rfid_db_info(RFID_DB_INFO_T* result)
{
    int max_cmd_wait_time = 0;
    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: DB INFO?\r\n");

    memset(&g_rfid_db_info, 0x00, sizeof(g_rfid_db_info));
    g_rfid_db_info.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__RFID_DB_INFO, "dBInfo?", strlen("dBInfo?"), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_db_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_db_info.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: DB INFO? - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( ( g_rfid_db_info.cmd_result == KJTEC_RFID_RET_SUCCESS ) && ( g_rfid_db_info.data_result == KJTEC_RFID_RET_SUCCESS ) )
    {
        result->cmd_result = g_rfid_db_info.data_result;
        result->db_cnt = g_rfid_db_info.db_cnt;
        strcpy(result->db_date, g_rfid_db_info.db_date);
    }
    else
    {
        return KJTEC_RFID_RET_FAIL;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: DB INFO? - SUCCESS [%d], cnt [%d] , ver[%s]\r\n", result->cmd_result, result->db_cnt, result->db_date );

    return KJTEC_RFID_RET_SUCCESS;
    
}

static int _parse_cmd__rfid_db_info(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    char buffer2[128] = {0,};
    char tmp_str[128] = {0,};

    g_rfid_db_info.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    // printf("%s():%d parse => [%s]\r\n", __func__, __LINE__, buf);
    //FWversionis,1.3.0.BusSR-t5, 

    if ( strstr(buf, "EnRollCount") == NULL )
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    memset(buffer, 0x00, sizeof(buffer));

    strcpy(buffer, buf);
    p_cmd = buffer;

    if ( p_cmd == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    g_rfid_db_info.db_cnt = atoi(tr);
    
    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    strcpy(buffer2,tr);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    mds_api_remove_char(buffer2, tmp_str, sizeof(tmp_str), '/');
    memset(&buffer2,0x00, sizeof(buffer2));
    strcpy(buffer2, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    mds_api_remove_char(buffer2, tmp_str, sizeof(tmp_str), ':');
    memset(&buffer2,0x00, RIFD_READ_BUFF_SIZE);
    strcpy(buffer2, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    mds_api_remove_char(buffer2, tmp_str, sizeof(tmp_str), '.');
    memset(&buffer2,0x00, RIFD_READ_BUFF_SIZE);
    strcpy(buffer2, tmp_str);

    if ( strlen(buffer2) > 2 )
    {
        strcpy(g_rfid_db_info.db_date, buffer2+2);
        //printf("%s():%d parse => [%s]\r\n", __func__, __LINE__, buffer2);
    }
    else
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
    }
    
    g_rfid_db_info.data_result = KJTEC_RFID_RET_SUCCESS;
    g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;

    return KJTEC_RFID_RET_SUCCESS;
}



int kjtec_rfid__firmware_write_start(int size)
{
    char cmd_write[RIFD_READ_BUFF_SIZE] = {0,};
    int max_cmd_wait_time = 0;
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM WRITE START\r\n");
    printf( "[KJTEC RFID] SEND CMD :: FIRM WRITE START\r\n");

    sprintf(cmd_write, "%s,%d,", "FW Down Load", size);

    memset(&g_rfid_firm_down_pkt, 0x00, sizeof(g_rfid_firm_down_pkt));
    g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_START, cmd_write, strlen(cmd_write), NULL);


    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_firm_down_pkt.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_firm_down_pkt.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM WRITE START - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM WRITE START - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    printf( "[KJTEC RFID] SEND CMD :: FIRM WRITE START - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    return g_rfid_firm_down_pkt.data_result;
}

int kjtec_rfid__firmware_write_one_pkt(char* buff, int buff_len)
{
    int max_cmd_wait_time;
    int result;
    if ( buff_len > RFID_CMD_FIRMWARE_ONE_PKT_SIZE_BYTE )
        return KJTEC_RFID_RET_FAIL;

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT \r\n");
    printf( "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT \r\n");

    memset(&g_rfid_firm_down_pkt, 0x00, sizeof(g_rfid_firm_down_pkt));
    g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd2(1, RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_ONE_PKT, buff, buff_len, get_fwdown_chksum_offset());

    max_cmd_wait_time = 1;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_firm_down_pkt.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(10000);
    }

    if ( g_rfid_firm_down_pkt.data_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    printf( "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    return g_rfid_firm_down_pkt.data_result;
    
}

static int _parse_cmd__firmware_write_one_pkt(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};

    RFID_FIRMWARE_DOWN_PKT_T tmp_result;

    //FWversionis,1.3.0.BusSR-t5, 

    if ( strstr(buf, "Result") == NULL )
    {
    //    printf ("_parse_cmd__chk_ready is success\r\n");
        g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    memset(&tmp_result, 0x00, sizeof(tmp_result));

    memset(buffer, 0x00, sizeof(buffer));

    strcpy(buffer, buf);
    
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    
    g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_SUCCESS;

    if ( atoi(tr) == 0 ) // fail
    {
        g_rfid_firm_down_pkt.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    
    g_rfid_firm_down_pkt.data_result = KJTEC_RFID_RET_SUCCESS;

    // 여기까지 왔으면 정상 파싱
/*
    printf(" >> boarding_info->rfid_uid [%s]\r\n", boarding_info->rfid_uid );
    printf(" >> boarding_info->boarding [%d]\r\n", boarding_info->boarding );
    printf(" >> boarding_info->date [%s]\r\n", boarding_info->date );
    printf(" >> boarding_info->chk_result [%d]\r\n", boarding_info->chk_result );
*/
    return KJTEC_RFID_RET_SUCCESS;
}
=======
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
#include <at/at_util.h>

#include "kjtec_rfid_cmd.h"
#include "cl_rfid_tools.h"

#define LOG_TARGET eSVC_MODEL

static int _rfid_fd = KJTEC_RFID_INVAILD_FD;

pthread_t tid_kjtec_rfid_thread;
static pthread_mutex_t kjtec_rfid_mutex = PTHREAD_MUTEX_INITIALIZER;
static int _g_run_kjtec_rfid_thread_run = 1;

static RFID_DEV_INFO_T                  g_rfid_dev_info;
static RIFD_PASSENGER_DATA_INFO_T       g_rfid_passenger_data_info;
static RIFD_CHK_READY_T                 g_rfid_chk_ready;
static RIFD_DATA_ALL_CLR_T              g_rfid_all_clr;
static RFID_SAVE_PASSENGER_DATA_T       g_rfid_save_passenger_data;
static RFID_FIRMWARE_VER_T              g_rfid_firmware_ver;
static RFID_FIRMWARE_DOWN_PKT_T         g_rfid_firm_down_pkt;
static RFID_DB_INFO_T                   g_rfid_db_info;


static int _parse_cmd__wakeup(const char* buf);
static int _parse_cmd__passenger_data(const char* buf);
static int _parse_cmd__chk_ready(const char* buf);
static int _parse_cmd__data_all_clr(const char* buf);
static int _parse_cmd__save_passenger_data(const char* buf);
static int _parse_cmd__get_passenger_data(const char* buf);
static int _parse_cmd__firmware_ver_info(const char* buf);
static int _parse_cmd__rfid_db_info(const char* buf);
static int _parse_cmd__firmware_write_one_pkt(const char* buf);

int kjtec_rfid__dev_rfid_req_clr();
int clear_main_watchdog();

char _check_xor_sum(char* write_buff, int len)
{
	unsigned char val = 0;
	int i;
	
	for(i = 0; i < len ; i++)
	{
		val = val ^ write_buff[i];
	}
	
	return val;
}

static int _kjtec_rfid_dev_init()
{
    int max_chk_cnt = KJTEC_RFID_MAX_CHK_DEV_CNT;
    int ret_val = KJTEC_RFID_RET_FAIL;

    while(max_chk_cnt--)
    {
        if ( _rfid_fd == KJTEC_RFID_INVAILD_FD )
            _rfid_fd = mds_api_init_uart(KJTEC_RFID_UART_DEVNAME, KJTEC_RFID_UART_BAUDRATE);
        
        if ( _rfid_fd > 0 ) 
        {
            ret_val = KJTEC_RFID_RET_SUCCESS;
            break;
        }
        else
        {
            ret_val = KJTEC_RFID_RET_FAIL;
            _rfid_fd = KJTEC_RFID_INVAILD_FD;
        }
    }
    return ret_val;
}



// =========================================================================
// kjtec cmd 처리부분
// =========================================================================
static void _kjtec_rfid_cmd_proc(char* data_buff, int buff_len)
{
    int code = 0;
    int read_idx = 0;

    int cmd_cnt = 0;

    char buff[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char tmp_buff[RIFD_READ_BUFF_SIZE + 1] = {0,};

    char tmp_str1[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char tmp_str2[RIFD_READ_BUFF_SIZE + 1] = {0,};

    char tmp_str_bak[RIFD_READ_BUFF_SIZE + 1] = {0,};

    //printf("_kjtec_rfid_cmd_proc is start!! [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
    if ( data_buff == NULL )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 1");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    if (( buff_len > RIFD_READ_BUFF_SIZE ) ||  ( buff_len <= 0 ))
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 2");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //-------------------------------------------------------------------------------
    // 스페이스제거
    //-------------------------------------------------------------------------------
    memset(&buff, 0x00, sizeof(buff));
    memcpy(&buff, data_buff, buff_len);
    cmd_cnt = buff_len;
/*
    cmd_cnt = mds_api_remove_char(data_buff, buff, RIFD_READ_BUFF_SIZE, ' ');
    if ( cmd_cnt <= 0 )
    {
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    buff_len = cmd_cnt;
*/
    // -------------------------------------------------------------------------------
    //printf("_kjtec_rfid_cmd_proc is start!! [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
    if ( buff_len < 4 )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 3");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //if ( buff_len == 4 )
    {
        if ( ( buff[0] == 0x10 ) &&
             ( buff[1] == 0x10 ) &&
             ( buff[2] == 0x06 ) &&
             ( buff[3] == 0x00 ))
        {
           read_idx = 4;
        }
        else
        {
           read_idx = 0;
        }
    }

    if ( buff_len < (5+read_idx+1+1) )
    {
        // printf("kjtec error [%s](%s)\r\n",buff,buff_len);
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 4");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    // chk header..
    if ( ( buff[0+read_idx] != 0x10 ) &&
         ( buff[1+read_idx] != 0x10 ) &&
         ( buff[2+read_idx] != 0xff ))
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 5");
        //printf("_kjtec_rfid_cmd_proc is start!! [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    // get data

    code = buff[4 +read_idx];
    memcpy(&tmp_str1, buff + 5 + read_idx, buff_len -5 -1 - read_idx);

    //-------------------------------------------------------------------------------
    // 쓰레기값 제거
    //-------------------------------------------------------------------------------
    memset(&tmp_buff, 0x00, sizeof(tmp_buff));
    cmd_cnt = mds_api_remove_etc_char(tmp_str1, tmp_buff, cmd_cnt);
    if ( cmd_cnt <= 0 )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 6");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    memcpy(&tmp_str1, tmp_buff, cmd_cnt);
    memset(&tmp_buff, 0x00, sizeof(tmp_buff));

    cmd_cnt = mds_api_remove_char(tmp_str1, tmp_buff, RIFD_READ_BUFF_SIZE, ' ');
    if ( cmd_cnt <= 0 )
    {
        //devel_webdm_send_log("_kjtec_rfid_cmd_proc - 7");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //if ( code != 0x51)
#if 0
    { 
        printf("  >> [KJTEC RFID] parse cmd resp ++ \r\n");
        printf("  >> seqno [%d]\r\n", buff[3 +read_idx]);
        printf("  >> code [0x%x]\r\n", buff[4 +read_idx]);
        printf("  >> data len [%d]\r\n", buff[5 +read_idx]);

        printf("  >> data  [%s]\r\n", tmp_buff);
        printf("  >> [KJTEC RFID] parse cmd resp -- \r\n");
    }
#endif

    // -------------------------------------------------------------------------------

    switch ( code )
    {
        case RFID_CMD_ID_RESP___INIT_WAKEUP:
        {
            _parse_cmd__wakeup(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__PASSENGER_DATA_INFO:
        {
            _parse_cmd__passenger_data(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__CHK_READY:
        {
            _parse_cmd__chk_ready(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__DATA_ALL_CLR:
        {
            _parse_cmd__data_all_clr(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__SAVE_PASSENGER_DATA:
        case 0x33:
        {
            _parse_cmd__save_passenger_data(tmp_buff);

            break;
        }
        case RFID_CMD_ID_RESP__GET_PASSENGER_DATA:
        {
            char tmp_str3[RIFD_READ_BUFF_SIZE] = {0,};
            static int passenger_parse_fail_cnt = 0;
            //char* test_str = "+[32:[223A05,1,20170901130801,1],33:[FB5305,1,20170901130802,1],34:[55C807,1,20170901130804,1],35:[023F81,1,20170901130805,1],]";
            //memset(&tmp_str2, 0x00, sizeof(tmp_str2));
            //strcpy(tmp_str2, test_str);
            // 씨발놈들 앞에 쓰레기문자가 올때가 있다. 
            // 대체 스펙서대로 되어있는게 한개도 없다. 어휴
            char* tmp_str_p_2 = strstr(tmp_buff,"[");
            //char* tmp_str_p_2 = "[13:[C019001626035001,1,20180129180246,0],14:[C019001625645001,1,20180129180247,1],15:[C019001626305001,1,20180129180249,1],16:[C019001624732001,1,20180129180256,0],],0],]    ";
            if ( ( tmp_str_p_2 != NULL ) && ( _parse_cmd__get_passenger_data(tmp_str_p_2) == KJTEC_RFID_RET_SUCCESS ) )
            {
                //usleep(10000);
                kjtec_rfid__dev_rfid_req_clr();
                passenger_parse_fail_cnt = 0;
            }
            else
            {
                passenger_parse_fail_cnt++;
                devel_webdm_send_log("passenger data parse fail => %s", tmp_str_p_2);
            }

            if ( passenger_parse_fail_cnt > RIFD_MAX_READ_USER_INFO_TRY_FAIL_CNT )   
            {
                devel_webdm_send_log("passenger data parse fail - 2");                
                kjtec_rfid__dev_rfid_req_clr();
            }
            

            break;
        }
        case RFID_CMD_ID_RESP__FIRMWARE_VER_INFO:
        {
            _parse_cmd__firmware_ver_info(tmp_buff);
            break;
        }
        case RFID_CMD_ID_RESP__RFID_DB_INFO:
        {
             _parse_cmd__rfid_db_info(tmp_buff);
            break;
        }
        case RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_WRITE_RET:
        {
            _parse_cmd__firmware_write_one_pkt(tmp_buff);
            break;
        }
        default:
        {
            break;
        }
    }
    // flush data..
    kjtec_rfid__flush_data(0);
}

// =============================================================
// kjtec rfid 커맨드 내리기
// =============================================================
static int _kjtec_rfid_cmd(int auto_lock, unsigned char cmd_code, char* cmd_data, int cmd_data_len, void* recv_data)
{
    int uart_ret = 0;
    int ret_val = 0;

    char kjtec_rfid_send_cmd[RIFD_READ_BUFF_SIZE] = {0,};
    char kjtec_rfid_recv_data[RIFD_READ_BUFF_SIZE] = {0,};

    char* tmp_recv_buff_p = NULL;
    
    int read_len = 0;
//    int to_read = sizeof(kjtec_rfid_recv_data);
    int read_retry_cnt = KJTEC_RFID_UART_READ_RETRY_CNT;
    
    int cmd_write_total_len = 0;
    int cmd_write_cmd_len = 0;

    int sleep_timing = KJTEC_RFID_CMD__SLEEP_INTERAL_MIL * 1000;

    if ( auto_lock )
    {
        pthread_mutex_lock(&kjtec_rfid_mutex);
    }

    if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
    {
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }
    
    // make command : common header
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x10;  // 0
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x10;  // 1
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0xff;  // 2

    // seq_no 
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 3 

    // command code
    kjtec_rfid_send_cmd[cmd_write_total_len++] = cmd_code;  // 4
    
    // data len
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 5

    cmd_write_cmd_len = cmd_data_len;

    strncpy(kjtec_rfid_send_cmd + cmd_write_total_len, cmd_data, cmd_write_cmd_len);

    cmd_write_total_len += cmd_write_cmd_len;

    kjtec_rfid_send_cmd[5] = cmd_write_cmd_len; // data len idx ==> 5 // hard coding

    // chk sum
    kjtec_rfid_send_cmd[cmd_write_total_len++] = _check_xor_sum(cmd_data, cmd_write_cmd_len);

    // do not ret chk
    mds_api_uart_write(_rfid_fd, kjtec_rfid_send_cmd, cmd_write_total_len);

    // mds_api_debug_hexdump_buff(kjtec_rfid_send_cmd, cmd_write_total_len);

    // 승객데이터는 무조건 쓰래드에서 읽는걸로
    if ( cmd_code != RFID_CMD_ID_REQ__GET_PASSENGER_DATA)
    {
        while(read_retry_cnt--)
        {
            tmp_recv_buff_p = kjtec_rfid_recv_data;

            uart_ret =  mds_api_uart_read(_rfid_fd, tmp_recv_buff_p + read_len,  (RIFD_READ_BUFF_SIZE - read_len - 1), KJTEC_RFID_UART_READ_TIMEOUT);

            if ( uart_ret > 0 )
            {
                read_len += uart_ret;
            }
            
            if ( read_len > 9 )
                break;

            if ( (RIFD_READ_BUFF_SIZE - read_len - 1) <= 0 )
                break;
            
        }
    }

FINISH:
    if ( auto_lock )
    {
        pthread_mutex_unlock(&kjtec_rfid_mutex);
    }

    if ( read_len > 9 )
    {
        tmp_recv_buff_p = kjtec_rfid_recv_data;
        _kjtec_rfid_cmd_proc(tmp_recv_buff_p+4, read_len-4 );
    }

    // userdata 쓸때는 sleep 없다.
    if ( cmd_code == RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA )
        sleep_timing = 0;

    usleep(sleep_timing);

    return ret_val;
}

// =============================================================
// kjtec rfid 커맨드 내리기
// =============================================================
static int _kjtec_rfid_cmd2(int auto_lock, unsigned char cmd_code, char* cmd_data, int cmd_data_len, int chksum_offset)
{
    int uart_ret = 0;
    int ret_val = 0;

    char kjtec_rfid_send_cmd[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char kjtec_rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    
    char* tmp_recv_buff_p = NULL;

    int read_len = 0;
//    int to_read = sizeof(kjtec_rfid_recv_data);
    int read_retry_cnt = KJTEC_RFID_UART_READ_RETRY_CNT;
    
    int cmd_write_total_len = 0;
    int cmd_write_cmd_len = 0;

    int sleep_timing = 0 * 1000;

    if ( auto_lock )
    {
        pthread_mutex_lock(&kjtec_rfid_mutex);
    }

    if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
    {
        ret_val = KJTEC_RFID_RET_FAIL;
        goto FINISH;
    }
    

    cmd_write_cmd_len = cmd_data_len;

    memcpy(&kjtec_rfid_send_cmd, cmd_data, cmd_data_len);

    cmd_write_total_len += cmd_write_cmd_len;

    kjtec_rfid_send_cmd[cmd_write_total_len++] = _check_xor_sum(cmd_data + chksum_offset, cmd_write_cmd_len - chksum_offset);

    mds_api_uart_write(_rfid_fd, kjtec_rfid_send_cmd, cmd_write_total_len);

    //mds_api_debug_hexdump_buff(kjtec_rfid_send_cmd, cmd_write_total_len);

    while(read_retry_cnt--)
    {
        tmp_recv_buff_p = kjtec_rfid_recv_data;

	    uart_ret =  mds_api_uart_read(_rfid_fd, tmp_recv_buff_p + read_len,  (RIFD_READ_BUFF_SIZE - read_len - 1), KJTEC_RFID_UART_READ_TIMEOUT);

        if ( uart_ret > 0 )
        {
            read_len += uart_ret;
        }
        
        if (strstr(kjtec_rfid_recv_data, "Download Result,0," ) != NULL)
            break;
        
        if (strstr(kjtec_rfid_recv_data, "Download Result,1," ) != NULL)
            break;

        if ( ( RIFD_READ_BUFF_SIZE - read_len - 1) <= 0 )
            break;
    }


FINISH:
    if ( auto_lock )
    {
        pthread_mutex_unlock(&kjtec_rfid_mutex);
    }

    if ( read_len > 9 )
    {
        tmp_recv_buff_p = kjtec_rfid_recv_data;
        _kjtec_rfid_cmd_proc(tmp_recv_buff_p, read_len );
    }

    return ret_val;
}

void kjtec_rfid__flush_data(int timeout)
{
    unsigned char rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    int uart_ret = 0;
    // pthread_mutex_lock(&kjtec_rfid_mutex);
    while(1)
    {
        if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
            break;
        
        uart_ret = mds_api_uart_read(_rfid_fd, rfid_recv_data,  RIFD_READ_BUFF_SIZE - 1, timeout);
        if ( uart_ret <= 0 )
            break;
    
    }
    //pthread_mutex_unlock(&kjtec_rfid_mutex);
}
// ==============================================================
// read thread
// ==============================================================
void kjtec_rfid__read_thread(void)
{
    char rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char* tmp_recv_buff_p = NULL;

    // flush data..
    kjtec_rfid__flush_data(1);

    while(_g_run_kjtec_rfid_thread_run)
    {
        int uart_ret = 0;
        int read_size = 0;

        if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        pthread_mutex_lock(&kjtec_rfid_mutex);

        memset(rfid_recv_data, 0x00, RIFD_READ_BUFF_SIZE);

        while(1)
        {
            tmp_recv_buff_p = rfid_recv_data;

            uart_ret =  mds_api_uart_read2(_rfid_fd, tmp_recv_buff_p + read_size,  ( RIFD_READ_BUFF_SIZE - read_size - 1 ), KJTEC_RFID_UART_READ_TIMEOUT2_USEC);

            if ( uart_ret > 0 )
            {
                read_size += uart_ret;
            }
            else
                break;

            if ( ( RIFD_READ_BUFF_SIZE - read_size - 1 ) <= 0 )
                break;
        }

        pthread_mutex_unlock(&kjtec_rfid_mutex);

        if ( read_size > 0 )
        {
            // mds_api_debug_hexdump_buff(rfid_recv_data, uart_ret);
            tmp_recv_buff_p = rfid_recv_data;
            _kjtec_rfid_cmd_proc(tmp_recv_buff_p, read_size );
        }
        else
        {
            sleep(1);
        }

        usleep(500); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함
    }
}

void init_kjtec_rfid()
{
    _g_run_kjtec_rfid_thread_run = 1;
    pthread_create(&tid_kjtec_rfid_thread, NULL, kjtec_rfid__read_thread, NULL);

    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info) );
    memset(&g_rfid_passenger_data_info, 0x00, sizeof(g_rfid_passenger_data_info) );
    memset(&g_rfid_chk_ready, 0x00, sizeof(g_rfid_chk_ready) );
    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr) );
    memset(&g_rfid_save_passenger_data, 0x00, sizeof(g_rfid_save_passenger_data) );
    memset(&g_rfid_firmware_ver, 0x00, sizeof(g_rfid_firmware_ver));
    memset(&g_rfid_firm_down_pkt, 0x00, sizeof(g_rfid_firm_down_pkt));
    memset(&g_rfid_db_info, 0x00, sizeof(g_rfid_db_info));


} 


// ----------------------------------------------------------------------
// parser : 승객데이터
// ----------------------------------------------------------------------
static int _parse_cmd__passenger_data(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    int char_cnt = 0;

    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    char tmp_str[RIFD_READ_BUFF_SIZE] = {0,};

    g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    strcpy(buffer, buf);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    char_cnt = mds_api_remove_char(buffer, tmp_str, sizeof(buffer), '/');
    if ( char_cnt <= 0 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    memset(&buffer, 0x00, sizeof(buffer));
    strcpy(buffer, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    char_cnt = mds_api_remove_char(buffer, tmp_str, sizeof(buffer), ':');
    if ( char_cnt <= 0 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    memset(&buffer,0x00, sizeof(buffer));
    strcpy(buffer, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    char_cnt = mds_api_remove_char(buffer, tmp_str, sizeof(buffer), '.');
    if ( char_cnt <= 0 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    memset(&buffer,0x00, sizeof(buffer));
    strcpy(buffer, tmp_str);

    p_cmd = buffer;

    if ( p_cmd == NULL)
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL)     
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL)
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    if ( strlen(tr) <= 2 )
    {
        g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    strcpy(g_rfid_passenger_data_info.saved_timestamp, tr+2);

    g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
    g_rfid_passenger_data_info.data_result = KJTEC_RFID_RET_SUCCESS;

    return KJTEC_RFID_RET_SUCCESS;
}


// -------------------------------------------------------------------------
// cmd : dev wakeup
// -------------------------------------------------------------------------
int kjtec_rfid__dev_wakeup(RFID_DEV_INFO_T* result)
{
    int write_len = 0;
 	time_t t;
    struct tm *lt;
    char  cmd_data_buff[RIFD_READ_BUFF_SIZE] = {0,};
    char dev_phone_num[128] ={0,};

    int max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP\r\n");
	//struct tm* tm_ptr;

    if((t = time(NULL)) == -1) {
        perror("get_modem_time_tm() call error\r\n");
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 1 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 1 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if((lt = localtime(&t)) == NULL) {
        perror("get_modem_time_tm() call error\r\n");
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

	char   time_str[26];
    
	sprintf(time_str, "%02d%02d%02d%02d%02d%02d", lt->tm_year+1900,
					lt->tm_mon+1,
					lt->tm_mday,
					lt->tm_hour,
					lt->tm_min,
					lt->tm_sec);

    if ( lt->tm_year+1900 < 2016 )
    {
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 [%d]\r\n", lt->tm_year+1900);
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 2 [%d]\r\n", lt->tm_year+1900);
        return KJTEC_RFID_RET_FAIL;
    }

    if ( at_get_phonenum(dev_phone_num, sizeof(dev_phone_num)) == AT_RET_SUCCESS)
        write_len += sprintf(cmd_data_buff+write_len, "%s", dev_phone_num);
    else
        write_len += sprintf(cmd_data_buff+write_len, "%s", "01299998888");

//    write_len += sprintf(cmd_data_buff+write_len, "%s", "01222605817");
    write_len += sprintf(cmd_data_buff+write_len, "%s", ",");
    write_len += sprintf(cmd_data_buff+write_len, "%s", time_str);
    
    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info));
    memset(&g_rfid_passenger_data_info, 0x00, sizeof(g_rfid_passenger_data_info));

    g_rfid_dev_info.cmd_result = -1;
    g_rfid_passenger_data_info.cmd_result = -1;
    
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__INIT_WAKEUP, cmd_data_buff, strlen(cmd_data_buff), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_dev_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            //printf("[KJTEC RFID] write cmd 1 [%s] -> resp success\r\n", __func__);
            break;
        }
        //printf("[KJTEC RFID] write cmd 1 [%s] -> resp wait...\r\n", __func__);
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);

        if ( max_cmd_wait_time & 20 )
            clear_main_watchdog();
    }

    if ( ( g_rfid_dev_info.cmd_result != KJTEC_RFID_RET_SUCCESS ) && ( g_rfid_dev_info.data_result != KJTEC_RFID_RET_SUCCESS ))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 3 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 3 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_passenger_data_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd 2 [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd 2 [%s] -> resp wait...\r\n", __func__);
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( ( g_rfid_passenger_data_info.cmd_result != KJTEC_RFID_RET_SUCCESS ) && ( g_rfid_passenger_data_info.data_result != KJTEC_RFID_RET_SUCCESS ) )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 4 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 4 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    result->cmd_result = KJTEC_RFID_RET_SUCCESS;
    strcpy(result->model_no, g_rfid_dev_info.model_no);
    result->total_passenger_cnt = g_rfid_dev_info.total_passenger_cnt;
    strcpy(result->saved_timestamp, g_rfid_passenger_data_info.saved_timestamp);

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - SUCCESS [%s][%d][%s]\r\n",result->model_no, result->total_passenger_cnt, result->saved_timestamp);

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__wakeup(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    
    
    if ( buf == NULL)
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    memset(buffer, 0x00, sizeof(buffer));
    strcpy(buffer, buf);
    p_cmd = buffer;

    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) 
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    strcpy(g_rfid_dev_info.model_no,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) 
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) 
    {
        g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_dev_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    g_rfid_dev_info.total_passenger_cnt = atoi(tr);

    g_rfid_dev_info.data_result = KJTEC_RFID_RET_SUCCESS;
    g_rfid_dev_info.cmd_result = KJTEC_RFID_RET_SUCCESS;

    return KJTEC_RFID_RET_SUCCESS;
}


// -------------------------------------------------------------------------
// cmd : dev ready chk
// -------------------------------------------------------------------------

int kjtec_rfid__dev_ready_chk(RIFD_CHK_READY_T* result)
{
    int max_cmd_wait_time = 0;
    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ARE YOU READY?\r\n");

    memset(&g_rfid_chk_ready, 0x00, sizeof(g_rfid_chk_ready));
    g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__CHK_READY, "Are you ready?", strlen( "Are you ready?"), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_chk_ready.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_chk_ready.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ARE YOU READY? - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( g_rfid_chk_ready.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        result->cmd_result = g_rfid_chk_ready.cmd_result;
        result->data_result = g_rfid_chk_ready.data_result;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ARE YOU READY? - SUCCESS [%d][%d]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
    
}

static int _parse_cmd__chk_ready(const char* buf)
{
    g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_chk_ready.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL)
    {
        g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_SUCCESS;
    }
    
    if ( strstr(buf, "Ready") != NULL )
    {
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_chk_ready.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
}


// -------------------------------------------------------------------------
// cmd : dev rfid saved data all clear
// -------------------------------------------------------------------------

int kjtec_rfid__dev_rfid_all_clear(RIFD_DATA_ALL_CLR_T* result)
{
    int max_cmd_wait_time;
    int sleep_time_usec = 200000;

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO\r\n");

    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr));
    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__DATA_ALL_CLR, "Init Data", strlen("Init Data"), NULL);
    
    max_cmd_wait_time = 300; // 300 * 2sec = 10min
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_all_clr.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }

        LOGE(LOG_TARGET, "[KJTEC RFID] write cmd [%s] -> [%d] : resp wait... [%d] usec sleep\r\n", __func__, max_cmd_wait_time, sleep_time_usec);
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(sleep_time_usec);

        if ( max_cmd_wait_time & 20 )
            clear_main_watchdog();
    }

    if ( g_rfid_all_clr.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( g_rfid_all_clr.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        result->cmd_result = g_rfid_all_clr.cmd_result;
        result->data_result = g_rfid_all_clr.data_result;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO - SUCCESS [%d][%d]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__data_all_clr(const char* buf)
{
    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_all_clr.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_SUCCESS;
    }


    if (( strstr(buf, "Success") != NULL ))
    {
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
    
}


// -------------------------------------------------------------------------
// cmd : rfid user info request
// -------------------------------------------------------------------------

int kjtec_rfid__dev_rfid_req()
{
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: REQ TAGED RFID USER INFO\r\n");
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__GET_PASSENGER_DATA, "Bus Log Request", strlen("Bus Log Request"), NULL);
    return 0;
}

/* *****************************************************************************
// 승객 데이터 파싱부분
***************************************************************************** */
static int _get_boarding_info(const char* buf, RFID_BOARDING_INFO_T* boarding_info)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    
    RFID_BOARDING_INFO_T tmp_boarding;

    memset(&tmp_boarding, 0x00, sizeof(tmp_boarding));

    memset(buffer, 0x00, sizeof(buffer));
    strcpy(buffer, buf);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    strcpy(tmp_boarding.rfid_uid,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    tmp_boarding.boarding = 1; // 1 fix

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    strcpy(tmp_boarding.date,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    
    {
        int tmp_stat = atoi(tr); // rfid 에서 읽은값 :: 0 :등록사용자, 1:미등록사용자
        if ( tmp_stat == 0 )
            tmp_boarding.chk_result = 1; // 서버에 전송할 값 : 1 :등록사용자, 0:미등록사용자
        else
            tmp_boarding.chk_result = 0; // 서버에 전송할 값 : 1 :등록사용자, 0:미등록사용자
    }

    // 여기까지 왔으면 정상 파싱
    memcpy(boarding_info, &tmp_boarding, sizeof(tmp_boarding));
/*
    printf(" >> boarding_info->rfid_uid [%s]\r\n", boarding_info->rfid_uid );
    printf(" >> boarding_info->boarding [%d]\r\n", boarding_info->boarding );
    printf(" >> boarding_info->date [%s]\r\n", boarding_info->date );
    printf(" >> boarding_info->chk_result [%d]\r\n", boarding_info->chk_result );
*/
    return KJTEC_RFID_RET_SUCCESS;

}

static int _parse_cmd__get_passenger_data(const char* buf)
{
    int buf_len;

    int prefix_cnt = 0;
    int suffix_cnt = 0;

    char real_data[RIFD_READ_BUFF_SIZE] = {0,};
    char one_data[64][1024];
    int one_data_cnt = 0;

    char* tmp_p = NULL;
    char* tmp_one_start_p = NULL;
    
    int i = 0;

    if ( buf == NULL )
    {
        //devel_webdm_send_log("get passenger err - 1 ");
        return KJTEC_RFID_RET_FAIL;
    }

    buf_len = strlen(buf);
    
    if ( buf_len > 512 )
    {
        //devel_webdm_send_log("get passenger err - 2 ");
        return KJTEC_RFID_RET_FAIL;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: GET PASSENGER START [%d] \"%s\" \r\n", buf_len, buf);

    if ( strstr(buf, "...") != NULL )
    {
        // rfid check interval change : end data.
        rfid_tool__env_set_rfid_chk_interval(RFID_CHK_DEFAULT_INTERVAL_SEC);
        return KJTEC_RFID_RET_SUCCESS;
    }
    // devel_webdm_send_log("get passenger chk ++ ");
    memset(&one_data, 0x00, sizeof(one_data));

    if ( buf_len < 5 )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 1\r\n");
        //devel_webdm_send_log("get passenger err - %d", __LINE__);
        return KJTEC_RFID_RET_FAIL;
    }

    // check essential data..
    //buf[0];
    if (( buf[0] != '[' ) || (buf[buf_len-1] != ']'))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 2\r\n");
        //devel_webdm_send_log("get passenger err - %d", __LINE__);
        //return KJTEC_RFID_RET_FAIL;
    }

    strncpy(real_data, buf, buf_len);

    // chk prefix
    tmp_p = real_data;
    while(1)
    {
        if ( tmp_p == NULL )
            break;
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_PREFIX_STR);
        prefix_cnt++ ;
    }

    // chk prefix
    tmp_p = real_data;
    while(1)
    {
        if ( tmp_p == NULL )
            break;
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_SUFFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_SUFFIX_STR);
        suffix_cnt++ ;
    }

    if ( ( prefix_cnt != suffix_cnt ) || ( prefix_cnt<= 0 ) || ( suffix_cnt <= 0 ))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 3\r\n");
        //devel_webdm_send_log("get passenger err - %d", __LINE__);
        //return KJTEC_RFID_RET_FAIL;
    }

    // split one 
    tmp_p = real_data;
    buf_len = strlen(real_data);
    while(1)
    {
        if ( tmp_p == NULL )
            break;
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_one_start_p = tmp_p;

        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_SUFFIX_STR);
        if ( tmp_p == NULL )
            break;
        
        strncpy(one_data[one_data_cnt], tmp_one_start_p, strlen(tmp_one_start_p) - strlen(tmp_p));
        one_data_cnt++;
    }

    if ( one_data_cnt <= 0 )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 4\r\n");
        devel_webdm_send_log("get passenger err - %d", __LINE__);
        return KJTEC_RFID_RET_FAIL;
    }

    RFID_BOARDING_MGR_T boarding_list;
    int total_boarding_list = 0;
    memset(&boarding_list, 0x00, sizeof(boarding_list));

    // check data validation
    for ( i = 0 ; i < one_data_cnt ; i++ )
    {
        int char_cnt = 0;
        RFID_BOARDING_INFO_T tmp_boarding;
        
        char_cnt = mds_api_count_char(one_data[i], strlen(one_data[i]), GET_PASSENGER_DATA_ARGUMENT_SPLIT_CHAR);
        if ( char_cnt != (GET_PASSENGER_DATA_ARGUMENT_CNT-1) )
        {
            LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 4 [%d]/[%d]\r\n", char_cnt, (GET_PASSENGER_DATA_ARGUMENT_CNT-1));
            devel_webdm_send_log("get passenger err - %d", __LINE__);
            return KJTEC_RFID_RET_FAIL;
        }
        // printf(" ---> one data : [%d] => [%s] \r\n", i, one_data[i]);
        if ( _get_boarding_info(one_data[i], &tmp_boarding) == KJTEC_RFID_RET_FAIL)
            continue;
        
        memcpy(&boarding_list.boarding_info[total_boarding_list], &tmp_boarding, sizeof(tmp_boarding));
        LOGI(LOG_TARGET,"---------------------------\r\n");
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].rfid_uid [%s]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].rfid_uid );
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].boarding [%d]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].boarding );
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].date [%s]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].date );
        LOGI(LOG_TARGET," >> boarding_list.boarding_info[%d].chk_result [%d]\r\n", total_boarding_list, boarding_list.boarding_info[total_boarding_list].chk_result );
        total_boarding_list++;
    }

    boarding_list.rfid_boarding_idx = total_boarding_list;

    sender_add_data_to_buffer(PACKET_TYPE_HTTP_SET_BOARDING_LIST, &boarding_list, ePIPE_2);

    // rfid check interval change : more data.
    rfid_tool__env_set_rfid_chk_interval(RFID_CHK_READ_MORE_INTERVAL_SEC);

    LOGT(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: GET PASSENGER SUCCESS AND SEND TO PKT \r\n");
    return KJTEC_RFID_RET_SUCCESS;
}





// -------------------------------------------------------------------------
// cmd : rfid user info request
// -------------------------------------------------------------------------
int kjtec_rfid__dev_rfid_req_clr()
{
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: REQ TAGED RFID USER INFO -> CLR RESP\r\n");
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__GET_PASSENGER_DATA_SUCCESS, "Bus Log Result,1,", strlen("Bus Log Result,1,"), NULL);
    return 0;
}


// -------------------------------------------------------------------------
// cmd : write rfid data.
// -------------------------------------------------------------------------
int kjtec_rfid__dev_write_rfid_data(int flag, char* rfid_user_str)
{
    int max_cmd_wait_time;
    int sleep_time_usec = 100000;

    char cmd_str[RFID_CMD_PASSENGER_STR_MAX_LEN] = {0,};

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO\r\n");

    if (rfid_user_str == NULL)
        return KJTEC_RFID_RET_FAIL;
        
    printf("flag [%d] rfid_user_str [%d]/[%d] is ==> \"%s\" \r\n", flag, strlen(rfid_user_str),RFID_CMD_PASSENGER_STR_MAX_LEN, rfid_user_str);

    if ( strlen(rfid_user_str) > RFID_CMD_PASSENGER_STR_MAX_LEN )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO - FAIL CASE 99\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    cmd_str[0] = flag;

    sprintf(cmd_str + 1, "%s", rfid_user_str);

    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr));
    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA, cmd_str, strlen(cmd_str), NULL);

    if ( flag == 2 ) // flag 2 :: RFID_USER_INFO_FRAME__END / RFID_USER_INFO_FRAME__ONLY_ONE_PKT
        max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 20;
    else
        max_cmd_wait_time = 30;

    while(max_cmd_wait_time--)
    {
        if ( g_rfid_save_passenger_data.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        LOGE(LOG_TARGET, "[KJTEC RFID] write cmd [%s] flag [%d] ->  resp wait... [%d]/[%d] \r\n", __func__, flag, sleep_time_usec, max_cmd_wait_time );
        printf( "[KJTEC RFID] write cmd [%s] flag [%d] ->  resp wait... [%d]/[%d] \r\n", __func__, flag, sleep_time_usec, max_cmd_wait_time );
        usleep(sleep_time_usec);

        if ( sleep_time_usec <  2000000 )
            sleep_time_usec = sleep_time_usec + 100000;
        
        if ( max_cmd_wait_time & 10 )
            clear_main_watchdog();
    }

    if ( g_rfid_save_passenger_data.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO - SUCCESS \r\n");
    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__save_passenger_data(const char* buf)
{
    g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
        g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_SUCCESS;
        return KJTEC_RFID_RET_SUCCESS;
    }

    if ( strstr(buf, "DataResult,1,") != NULL )
    {
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
  
}



// -------------------------------------------------------------------------
// cmd : dev firmware ver info
// -------------------------------------------------------------------------

int kjtec_rfid__firmware_ver_info(RFID_FIRMWARE_VER_T* result)
{
    int max_cmd_wait_time = 0;
    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: CHK VER?\r\n");

    memset(&g_rfid_firmware_ver, 0x00, sizeof(g_rfid_firmware_ver));
    g_rfid_firmware_ver.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__FIRMWARE_VER_INFO, "FW Version?", strlen("FW Version?"), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_firmware_ver.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_firmware_ver.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: CHK VER? - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( g_rfid_firmware_ver.cmd_result == KJTEC_RFID_RET_SUCCESS )
    {
        result->cmd_result = g_rfid_firmware_ver.cmd_result;
        strcpy(result->data_result, g_rfid_firmware_ver.data_result);
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: CHK VER? - SUCCESS [%d][%s]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__firmware_ver_info(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};

    RFID_FIRMWARE_VER_T tmp_version;

    if ( buf == NULL )
        return KJTEC_RFID_RET_FAIL;

    printf("%s() parse => [%s]\r\n", __func__, buf);
    //FWversionis,1.3.0.BusSR-t5, 

    if ( strstr(buf, "FWversionis") == NULL )
    {
    //    printf ("_parse_cmd__chk_ready is success\r\n");
        strcpy(g_rfid_firmware_ver.data_result, "FAIL");
        return KJTEC_RFID_RET_FAIL;
    }

    memset(&tmp_version, 0x00, sizeof(tmp_version));

    memset(buffer, 0x00, sizeof(buffer));

    strcpy(buffer, buf);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    strcpy(tmp_version.data_result, tr);
    
    tmp_version.cmd_result = KJTEC_RFID_RET_SUCCESS;
    // 여기까지 왔으면 정상 파싱
    memcpy(&g_rfid_firmware_ver, &tmp_version, sizeof(tmp_version));
/*
    printf(" >> boarding_info->rfid_uid [%s]\r\n", boarding_info->rfid_uid );
    printf(" >> boarding_info->boarding [%d]\r\n", boarding_info->boarding );
    printf(" >> boarding_info->date [%s]\r\n", boarding_info->date );
    printf(" >> boarding_info->chk_result [%d]\r\n", boarding_info->chk_result );
*/
    return KJTEC_RFID_RET_SUCCESS;
}

// -------------------------------------------------------------------------
// cmd : dev firmware ver info
// -------------------------------------------------------------------------

int kjtec_rfid__rfid_db_info(RFID_DB_INFO_T* result)
{
    int max_cmd_wait_time = 0;
    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: DB INFO?\r\n");

    memset(&g_rfid_db_info, 0x00, sizeof(g_rfid_db_info));
    g_rfid_db_info.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__RFID_DB_INFO, "dBInfo?", strlen("dBInfo?"), NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_db_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            break;
        }
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_db_info.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: DB INFO? - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    if ( ( g_rfid_db_info.cmd_result == KJTEC_RFID_RET_SUCCESS ) && ( g_rfid_db_info.data_result == KJTEC_RFID_RET_SUCCESS ) )
    {
        result->cmd_result = g_rfid_db_info.data_result;
        result->db_cnt = g_rfid_db_info.db_cnt;
        strcpy(result->db_date, g_rfid_db_info.db_date);
    }
    else
    {
        return KJTEC_RFID_RET_FAIL;
    }

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: DB INFO? - SUCCESS [%d], cnt [%d] , ver[%s]\r\n", result->cmd_result, result->db_cnt, result->db_date );

    return KJTEC_RFID_RET_SUCCESS;
    
}

static int _parse_cmd__rfid_db_info(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};
    char buffer2[128] = {0,};
    char tmp_str[128] = {0,};

    g_rfid_db_info.cmd_result = KJTEC_RFID_RET_FAIL;
    g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;

    if ( buf == NULL )
    {
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    // printf("%s():%d parse => [%s]\r\n", __func__, __LINE__, buf);
    //FWversionis,1.3.0.BusSR-t5, 

    if ( strstr(buf, "EnRollCount") == NULL )
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    memset(buffer, 0x00, sizeof(buffer));

    strcpy(buffer, buf);
    p_cmd = buffer;

    if ( p_cmd == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    
    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    g_rfid_db_info.db_cnt = atoi(tr);
    
    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL)
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }
    strcpy(buffer2,tr);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    mds_api_remove_char(buffer2, tmp_str, sizeof(tmp_str), '/');
    memset(&buffer2,0x00, sizeof(buffer2));
    strcpy(buffer2, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    mds_api_remove_char(buffer2, tmp_str, sizeof(tmp_str), ':');
    memset(&buffer2,0x00, RIFD_READ_BUFF_SIZE);
    strcpy(buffer2, tmp_str);

    memset(&tmp_str,0x00, sizeof(tmp_str));
    mds_api_remove_char(buffer2, tmp_str, sizeof(tmp_str), '.');
    memset(&buffer2,0x00, RIFD_READ_BUFF_SIZE);
    strcpy(buffer2, tmp_str);

    if ( strlen(buffer2) > 2 )
    {
        strcpy(g_rfid_db_info.db_date, buffer2+2);
        //printf("%s():%d parse => [%s]\r\n", __func__, __LINE__, buffer2);
    }
    else
    {
        strcpy(g_rfid_db_info.db_date, "0");
        g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_db_info.data_result = KJTEC_RFID_RET_FAIL;
    }
    
    g_rfid_db_info.data_result = KJTEC_RFID_RET_SUCCESS;
    g_rfid_db_info.cmd_result = KJTEC_RFID_RET_SUCCESS;

    return KJTEC_RFID_RET_SUCCESS;
}



int kjtec_rfid__firmware_write_start(int size)
{
    char cmd_write[RIFD_READ_BUFF_SIZE] = {0,};
    int max_cmd_wait_time = 0;
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM WRITE START\r\n");
    printf( "[KJTEC RFID] SEND CMD :: FIRM WRITE START\r\n");

    sprintf(cmd_write, "%s,%d,", "FW Down Load", size);

    memset(&g_rfid_firm_down_pkt, 0x00, sizeof(g_rfid_firm_down_pkt));
    g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_START, cmd_write, strlen(cmd_write), NULL);


    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_firm_down_pkt.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(KJTEC_RFID_CMD_RESP_WAIT_ONE_INTERVAL);
    }

    if ( g_rfid_firm_down_pkt.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM WRITE START - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM WRITE START - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    printf( "[KJTEC RFID] SEND CMD :: FIRM WRITE START - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    return g_rfid_firm_down_pkt.data_result;
}

int kjtec_rfid__firmware_write_one_pkt(char* buff, int buff_len)
{
    int max_cmd_wait_time;
    int result;
    if ( buff_len > RFID_CMD_FIRMWARE_ONE_PKT_SIZE_BYTE )
        return KJTEC_RFID_RET_FAIL;

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT \r\n");
    printf( "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT \r\n");

    memset(&g_rfid_firm_down_pkt, 0x00, sizeof(g_rfid_firm_down_pkt));
    g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd2(1, RFID_CMD_ID_REQ__FIRMWARE_DOWNLOAD_ONE_PKT, buff, buff_len, get_fwdown_chksum_offset());

    max_cmd_wait_time = 1;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_firm_down_pkt.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(10000);
    }

    if ( g_rfid_firm_down_pkt.data_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT - FAIL\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    
    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    printf( "[KJTEC RFID] SEND CMD :: FIRM DOWN => ONE PKT - SUCCESS [%d][%d]\r\n", g_rfid_firm_down_pkt.cmd_result, g_rfid_firm_down_pkt.data_result );
    return g_rfid_firm_down_pkt.data_result;
    
}

static int _parse_cmd__firmware_write_one_pkt(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[RIFD_READ_BUFF_SIZE] = {0,};

    RFID_FIRMWARE_DOWN_PKT_T tmp_result;

    //FWversionis,1.3.0.BusSR-t5, 

    if ( strstr(buf, "Result") == NULL )
    {
    //    printf ("_parse_cmd__chk_ready is success\r\n");
        g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_SUCCESS;
        g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    memset(&tmp_result, 0x00, sizeof(tmp_result));

    memset(buffer, 0x00, sizeof(buffer));

    strcpy(buffer, buf);
    
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    
    g_rfid_firm_down_pkt.cmd_result = KJTEC_RFID_RET_SUCCESS;

    if ( atoi(tr) == 0 ) // fail
    {
        g_rfid_firm_down_pkt.data_result = KJTEC_RFID_RET_FAIL;
        return KJTEC_RFID_RET_FAIL;
    }

    
    g_rfid_firm_down_pkt.data_result = KJTEC_RFID_RET_SUCCESS;

    // 여기까지 왔으면 정상 파싱
/*
    printf(" >> boarding_info->rfid_uid [%s]\r\n", boarding_info->rfid_uid );
    printf(" >> boarding_info->boarding [%d]\r\n", boarding_info->boarding );
    printf(" >> boarding_info->date [%s]\r\n", boarding_info->date );
    printf(" >> boarding_info->chk_result [%d]\r\n", boarding_info->chk_result );
*/
    return KJTEC_RFID_RET_SUCCESS;
}
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
