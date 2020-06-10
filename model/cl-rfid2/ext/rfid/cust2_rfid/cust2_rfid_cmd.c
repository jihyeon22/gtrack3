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

#include <at/at_util.h>

#include "ext/rfid/cust2_rfid/cust2_rfid_tools.h"
#include "ext/rfid/cust2_rfid/cust2_rfid_cmd.h"
#include "ext/rfid/cust2_rfid/cust2_rfid_senario.h"


#define LOG_TARGET eSVC_MODEL

static int _rfid_fd = CUST2_RFID_INVAILD_FD;

pthread_t tid_cust2_rfid_thread;
static pthread_mutex_t cust2_rfid_mutex = PTHREAD_MUTEX_INITIALIZER;
static int _g_run_cust2_rfid_thread_run = 1;

int cust2_rfid__dev_rfid_req_clr();
int clear_main_watchdog();

static char _check_xor_sum(char* write_buff, int len)
{
	unsigned char val = 0;
	int i;
	
	for(i = 0; i < len ; i++)
	{
		val = val ^ write_buff[i];
	}
	
	return val;
}

static int _cust2_rfid_dev_init()
{
    int max_chk_cnt = CUST2_RFID_MAX_CHK_DEV_CNT;
    int ret_val = CUST2_RFID_RET_FAIL;

    while(max_chk_cnt--)
    {
        if ( _rfid_fd == CUST2_RFID_INVAILD_FD )
            _rfid_fd = mds_api_init_uart(CUST2_RFID_UART_DEVNAME, CUST2_RFID_UART_BAUDRATE);
        
        if ( _rfid_fd > 0 ) 
        {
            ret_val = CUST2_RFID_RET_SUCCESS;
            break;
        }
        else
        {
            ret_val = CUST2_RFID_RET_FAIL;
            _rfid_fd = CUST2_RFID_INVAILD_FD;
        }
    }
    return ret_val;
}



// =========================================================================
// cust2 cmd 처리부분
// =========================================================================
static void _cust2_rfid_cmd_proc(char* data_buff, int buff_len)
{
    int code = 0;
    int read_idx = 0;

    int cmd_cnt = 0;

    char buff[RIFD_READ_BUFF_SIZE + 1] = {0,};

    char tmp_str1[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char tmp_str2[RIFD_READ_BUFF_SIZE + 1] = {0,};

    char tmp_str_bak[RIFD_READ_BUFF_SIZE + 1] = {0,};

    LOGT(LOG_TARGET, "[RFID CMD PROC] : recv \"%s\"\n",data_buff);

    printf("_cust2_rfid_cmd_proc is start!! -1 [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);

    if ( data_buff == NULL )
    {
        //devel_webdm_send_log("_cust2_rfid_cmd_proc - 1");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    if (( buff_len > RIFD_READ_BUFF_SIZE ) ||  ( buff_len <= 0 ))
    {
        //devel_webdm_send_log("_cust2_rfid_cmd_proc - 2");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    //-------------------------------------------------------------------------------
    // 스페이스제거
    //-------------------------------------------------------------------------------
    memset(&buff, 0x00, sizeof(buff));
    memcpy(&buff, data_buff, buff_len);
    cmd_cnt = buff_len;

    cmd_cnt = mds_api_remove_char(data_buff, buff, RIFD_READ_BUFF_SIZE, ' ');
    if ( cmd_cnt <= 0 )
    {
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    buff_len = cmd_cnt;

    // -------------------------------------------------------------------------------
    printf("_cust2_rfid_cmd_proc is start!! -2  [%s][%d] -> [%d]\r\n", data_buff, buff_len, __LINE__);
    
    if ( buff_len < 3 )
    {
        //devel_webdm_send_log("_cust2_rfid_cmd_proc - 3");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }
    
    //-------------------------------------------------------------------------------
    // 쓰레기값 제거
    //-------------------------------------------------------------------------------
    memset(&tmp_str1, 0x00, sizeof(tmp_str1));
    cmd_cnt = mds_api_remove_etc_char(data_buff, tmp_str1, cmd_cnt);
    if ( cmd_cnt <= 0 )
    {
        //devel_webdm_send_log("_cust2_rfid_cmd_proc - 6");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    cmd_cnt = mds_api_remove_char(tmp_str1, tmp_str2, RIFD_READ_BUFF_SIZE, ' ');
    if ( cmd_cnt <= 0 )
    {
        //devel_webdm_send_log("_cust2_rfid_cmd_proc - 7");
        printf("%s() - err : line [%d]\r\n", __func__, __LINE__);
        return;
    }

    { 
        printf("  >> [CUST2 RFID] parse cmd resp ++ \r\n");
        printf("  >> data  [%s] \r\n", tmp_str2 );
        printf("  >> [CUST2 RFID] parse cmd resp -- \r\n");
    }

    // -------------------------------------------------------------------------------

    if (strcmp(tmp_str2,"[01]") == 0 ) // init success;
    {
        cust2_rfid__set_senario(CUST2_RFID_SENARIO__INIT_SUCCESS);
    }
    else if (strcmp(tmp_str2,"[09]") == 0 ) // init fail..
    {

    }
    else
    {
        cust2_rfid__rfid_read_proc(tmp_str2);
    }
    // flush data..
    cust2_rfid__flush_data(0);
}

// =============================================================
// cust2 rfid 커맨드 내리기
// =============================================================
static int _cust2_rfid_cmd(int auto_lock, unsigned char cmd_code, char* cmd_data, int cmd_data_len, void* recv_data)
{
    int uart_ret = 0;
    int ret_val = 0;

    char cust2_rfid_send_cmd[RIFD_READ_BUFF_SIZE] = {0,};
    char cust2_rfid_recv_data[RIFD_READ_BUFF_SIZE] = {0,};

    char* tmp_recv_buff_p = NULL;
    
    int read_len = 0;
//    int to_read = sizeof(cust2_rfid_recv_data);
    int read_retry_cnt = CUST2_RFID_UART_READ_RETRY_CNT;
    
    int cmd_write_total_len = 0;
    int cmd_write_cmd_len = 0;

    int sleep_timing = CUST2_RFID_CMD__SLEEP_INTERAL_MIL * 1000;

    if ( auto_lock )
    {
        pthread_mutex_lock(&cust2_rfid_mutex);
    }

    if ( _cust2_rfid_dev_init() == CUST2_RFID_RET_FAIL )
    {
        ret_val = CUST2_RFID_RET_FAIL;
        goto FINISH;
    }
#if 0
    // make command : common header
    cust2_rfid_send_cmd[cmd_write_total_len++] = 0x10;  // 0
    cust2_rfid_send_cmd[cmd_write_total_len++] = 0x10;  // 1
    cust2_rfid_send_cmd[cmd_write_total_len++] = 0xff;  // 2

    // seq_no 
    cust2_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 3 

    // command code
    cust2_rfid_send_cmd[cmd_write_total_len++] = cmd_code;  // 4
    
    // data len
    cust2_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 5

    cmd_write_cmd_len = cmd_data_len;

    strncpy(cust2_rfid_send_cmd + cmd_write_total_len, cmd_data, cmd_write_cmd_len);

    cmd_write_total_len += cmd_write_cmd_len;

    cust2_rfid_send_cmd[5] = cmd_write_cmd_len; // data len idx ==> 5 // hard coding

    // chk sum
    cust2_rfid_send_cmd[cmd_write_total_len++] = _check_xor_sum(cmd_data, cmd_write_cmd_len);
#endif
    strcpy(cust2_rfid_send_cmd,cmd_data);
    cmd_write_total_len = cmd_data_len;
    // do not ret chk
    mds_api_uart_write(_rfid_fd, cust2_rfid_send_cmd, cmd_write_total_len);
    printf(" ------------ cust2 cmd send  +++ ------------------------\r\n");
    printf("   > send cmd : \"%s\"\r\n",cust2_rfid_send_cmd);
    printf("-----------------------------------------\r\n");
    mds_api_debug_hexdump_buff(cust2_rfid_send_cmd, cmd_write_total_len);
    printf(" ------------ cust2 cmd send  --- ------------------------\r\n");
#if 0
    // 승객데이터는 무조건 쓰래드에서 읽는걸로
    if ( cmd_code != RFID_CMD_ID_REQ__GET_PASSENGER_DATA)
    {
        while(read_retry_cnt--)
        {
            tmp_recv_buff_p = cust2_rfid_recv_data;

            uart_ret =  mds_api_uart_read(_rfid_fd, tmp_recv_buff_p + read_len,  (RIFD_READ_BUFF_SIZE - read_len - 1), CUST2_RFID_UART_READ_TIMEOUT);

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
#endif
FINISH:
    if ( auto_lock )
    {
        pthread_mutex_unlock(&cust2_rfid_mutex);
    }

    if ( read_len > 0 )
    {
        tmp_recv_buff_p = cust2_rfid_recv_data;
        printf(" ------ read data case 2 ++ ----------------------\r\n");
        mds_api_debug_hexdump_buff(tmp_recv_buff_p, read_len);
        _cust2_rfid_cmd_proc(tmp_recv_buff_p, read_len );
        printf(" ------ read data case 2 -- ----------------------\r\n");
    }

    // userdata 쓸때는 sleep 없다.
    if ( cmd_code == RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA )
        sleep_timing = 0;

    usleep(sleep_timing);

    return ret_val;
}


void cust2_rfid__flush_data(int timeout)
{
    unsigned char rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    int uart_ret = 0;
    // pthread_mutex_lock(&cust2_rfid_mutex);
    while(1)
    {
        if ( _cust2_rfid_dev_init() == CUST2_RFID_RET_FAIL )
            break;
        
        uart_ret = mds_api_uart_read(_rfid_fd, rfid_recv_data,  RIFD_READ_BUFF_SIZE - 1, timeout);
        if ( uart_ret <= 0 )
            break;
    
    }
    //pthread_mutex_unlock(&cust2_rfid_mutex);
}
// ==============================================================
// read thread
// ==============================================================
void cust2_rfid__read_thread(void)
{
    char rfid_recv_data[RIFD_READ_BUFF_SIZE + 1] = {0,};
    char* tmp_recv_buff_p = NULL;

    // flush data..
    cust2_rfid__flush_data(1);

    while(_g_run_cust2_rfid_thread_run)
    {
        int uart_ret = 0;
        int read_size = 0;

        if ( _cust2_rfid_dev_init() == CUST2_RFID_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        pthread_mutex_lock(&cust2_rfid_mutex);

        memset(rfid_recv_data, 0x00, RIFD_READ_BUFF_SIZE);

        while(1)
        {
            tmp_recv_buff_p = rfid_recv_data;

            uart_ret =  mds_api_uart_read2(_rfid_fd, tmp_recv_buff_p + read_size,  ( RIFD_READ_BUFF_SIZE - read_size - 1 ), CUST2_RFID_UART_READ_TIMEOUT2_USEC);

            if ( uart_ret > 0 )
            {
                read_size += uart_ret;
            }
            else
                break;

            if ( ( RIFD_READ_BUFF_SIZE - read_size - 1 ) <= 0 )
                break;
        }

        pthread_mutex_unlock(&cust2_rfid_mutex);

        if ( read_size > 0 )
        {
            
            tmp_recv_buff_p = rfid_recv_data;
            printf("----- read case 1 ++ --------\r\n");
            mds_api_debug_hexdump_buff(tmp_recv_buff_p, read_size);
            _cust2_rfid_cmd_proc(tmp_recv_buff_p, read_size );
            printf("----- read case 1 ++ --------\r\n");
        }
        else
        {
            sleep(1);
        }

        usleep(500); // sleep 없이 mutex lock 을 바로 걸면, 다른 쪽에서 치고들어오지 못한다. 그래서 강제고 쉬게함
    }
}

void init_cust2_rfid()
{
    _g_run_cust2_rfid_thread_run = 1;
    pthread_create(&tid_cust2_rfid_thread, NULL, cust2_rfid__read_thread, NULL);

} 

int cust2_rfid_cmd__device_init()
{
    // [00,01233334444,20180910130517]
    char recv_buff[1024] = {0,};
    char send_buff[1024] = {0,};
    int  send_buff_len = 0;

 	time_t t;
    struct tm *lt;
    char   time_str[26];

    char phonenum[AT_LEN_PHONENUM_BUFF] = {0};
    at_get_phonenum(phonenum, sizeof(phonenum));

    // get time
    t = time(NULL);
    lt = localtime(&t);

    sprintf(time_str, "%04d%02d%02d%02d%02d%02d", lt->tm_year+1900,
            lt->tm_mon+1,
            lt->tm_mday,
            lt->tm_hour,
            lt->tm_min,
            lt->tm_sec);

    send_buff_len = sprintf(send_buff, "[00,%s,%s]", phonenum, time_str);
    _cust2_rfid_cmd(0, 0, send_buff, send_buff_len, recv_buff);
    return CUST2_RFID_RET_SUCCESS;
}



int cust2_rfid_cmd__send_code(int code)
{
    // [00,01233334444,20180910130517]
    char recv_buff[1024] = {0,};
    char send_buff[1024] = {0,};
    int  send_buff_len = 0;

    send_buff_len = sprintf(send_buff, "[%d]", code);
    _cust2_rfid_cmd(0, 0, send_buff, send_buff_len, recv_buff);
    return CUST2_RFID_RET_SUCCESS;
}
