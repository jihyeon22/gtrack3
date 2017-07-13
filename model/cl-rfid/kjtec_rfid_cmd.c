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

static int _rfid_fd = KJTEC_RFID_INVAILD_FD;

pthread_t tid_kjtec_rfid_thread;
static pthread_mutex_t kjtec_rfid_mutex = PTHREAD_MUTEX_INITIALIZER;
static int _g_run_kjtec_rfid_thread_run = 1;

static RFID_DEV_INFO_T                  g_rfid_dev_info;
static RIFD_PASSENGER_DATA_INFO_T       g_rfid_passenger_data_info;
static RIFD_CHK_READY_T                 g_rfid_chk_ready;
static RIFD_DATA_ALL_CLR_T              g_rfid_all_clr;
static RFID_SAVE_PASSENGER_DATA_T       g_rfid_save_passenger_data;

static int _parse_cmd__wakeup(const char* buf);
static int _parse_cmd__passenger_data(const char* buf);
static int _parse_cmd__chk_ready(const char* buf);
static int _parse_cmd__data_all_clr(const char* buf);
static int _parse_cmd__save_passenger_data(const char* buf);
static int _parse_cmd__get_passenger_data(const char* buf);

int kjtec_rfid__dev_rfid_req_clr();

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
static void _kjtec_rfid_cmd_proc(char* buff, int buff_len)
{
    int code = 0;
    char tmp_str1[512] = {0,};
    char tmp_str2[512] = {0,};

    if ( buff_len == 4 )
    {
        if ( ( buff[0] == 0x10 ) &&
             ( buff[1] == 0x10 ) &&
             ( buff[2] == 0x06 ) &&
             ( buff[3] == 0x00 ))
        {
           // printf(" >> [KJTEC RFID] recv ack success \r\n");
        }
        else
        {
           // printf(" >> [KJTEC RFID] recv ack fail\r\n");
        }
    }

    if (buff_len <= 5)
        return;

    // chk header..
    if ( ( buff[0] != 0x10 ) &&
         ( buff[1] != 0x10 ) &&
         ( buff[2] != 0xff ))
    {
        return;
    }

    // get data
    code = buff[4];
    memcpy(&tmp_str1, buff + 5,buff_len -5 -1);

    mds_api_remove_char(tmp_str1, tmp_str2, 512, ' ');
/*
    printf("  >> [KJTEC RFID] parse cmd resp ++ \r\n");
    printf("  >> seqno [%d]\r\n", buff[3]);
    printf("  >> code [0x%x]\r\n", buff[4]);
    printf("  >> data len [%d]\r\n", buff[5]);

    printf("  >> data  [%s]\r\n", tmp_str2);
    printf("  >> [KJTEC RFID] parse cmd resp -- \r\n");
*/
    switch ( code )
    {
        case RFID_CMD_ID_RESP___INIT_WAKEUP:
        {
            _parse_cmd__wakeup(tmp_str2);
            break;
        }
        case RFID_CMD_ID_RESP__PASSENGER_DATA_INFO:
        {
            _parse_cmd__passenger_data(tmp_str2);
            break;
        }
        case RFID_CMD_ID_RESP__CHK_READY:
        {
            _parse_cmd__chk_ready(tmp_str2);
            break;
        }
        case RFID_CMD_ID_RESP__DATA_ALL_CLR:
        {
            _parse_cmd__data_all_clr(tmp_str2);
            break;
        }
        case RFID_CMD_ID_RESP__SAVE_PASSENGER_DATA:
        case 0x33:
        {
            _parse_cmd__save_passenger_data(tmp_str2);
            break;
        }
        case RFID_CMD_ID_RESP__GET_PASSENGER_DATA:
        {
            //char* test_str = "+[1:[11111,1,20170706091207,0],2:[22222,1,20170706092202,0],3:[3333,1,20170706092202,0],4:[4444,1,20170706092202,0],]";

            sleep(1);
            //if ( _parse_cmd__get_passenger_data(tmp_str2) == KJTEC_RFID_RET_SUCCESS )
            if ( _parse_cmd__get_passenger_data(tmp_str2) == KJTEC_RFID_RET_SUCCESS )
                kjtec_rfid__dev_rfid_req_clr();
            break;
        }
        default:
        {
            break;
        }
    }

    
}

// =============================================================
// kjtec rfid 커맨드 내리기
// =============================================================
static int _kjtec_rfid_cmd(int auto_lock, unsigned char cmd_code, char* cmd_data, void* recv_data)
{
    int uart_ret = 0;
    int ret_val = 0;

    char kjtec_rfid_send_cmd[1024] = {0,};
    char kjtec_rfid_recv_data[1024] = {0,};
    
    int read_len = 0;
//    int to_read = sizeof(kjtec_rfid_recv_data);
    int read_retry_cnt = KJTEC_RFID_UART_READ_RETRY_CNT;
    
    int cmd_write_total_len = 0;
    int cmd_write_cmd_len = 0;

    if ( auto_lock )
    {
        //printf(" >> kjtec rfid cmd mutex lock\r\n");
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
    //printf("\r\n[KJTEC RFID] write cmd [0x%x]\r\n", cmd_code);
    // data len
    kjtec_rfid_send_cmd[cmd_write_total_len++] = 0x00;  // 5

    cmd_write_cmd_len = strlen(cmd_data);

    strncpy(kjtec_rfid_send_cmd + cmd_write_total_len, cmd_data, cmd_write_cmd_len);


    cmd_write_total_len += cmd_write_cmd_len;

    kjtec_rfid_send_cmd[5] = cmd_write_cmd_len; // data len idx ==> 5 // hard coding

    // chk sum
    kjtec_rfid_send_cmd[cmd_write_total_len++] = _check_xor_sum(cmd_data, cmd_write_cmd_len);
    // do not ret chk
    mds_api_uart_write(_rfid_fd, kjtec_rfid_send_cmd, cmd_write_total_len);

   // printf("================================ rfid data write [%d]\r\n", cmd_write_total_len);
    // mds_api_debug_hexdump_buff(kjtec_rfid_send_cmd, cmd_write_total_len);

    while(read_retry_cnt--)
    {
	    uart_ret =  mds_api_uart_read(_rfid_fd, kjtec_rfid_recv_data + read_len,  sizeof(kjtec_rfid_recv_data) - read_len, KJTEC_RFID_UART_READ_TIMEOUT);
        // printf("========>>>> uart_ret [%d]\r\n", uart_ret);
        //if ( uart_ret <= 0 )
        //    continue;

        read_len += uart_ret;
        if (read_len > 9 )
            break;
    }

    // printf(">>rfid data read cmd data +++++++++++++++++++++++++ [%d]\r\n", read_len);

    // mds_api_debug_hexdump_buff(kjtec_rfid_recv_data, read_len);
    
    // chk ack
    if ( read_len >= 4 )
    {
        if ( ( kjtec_rfid_recv_data[0] == 0x10 ) &&
             ( kjtec_rfid_recv_data[1] == 0x10 ) &&
             ( kjtec_rfid_recv_data[2] == 0x06 ) &&
             ( kjtec_rfid_recv_data[3] == 0x00 ))
        {
            //printf("[KJTEC RFID] recv ack success \r\n");
        }
        else
        {
            //printf("[KJTEC RFID] recv ack fail\r\n");
        }
    }
    

    // printf(">> rfid data read cmd data ------------------------- [%d]\r\n", read_len);
//    kjtec_rfid_recv_data[5]; // data len
//    kjtec_rfid_recv_data[6]; // error list
//    kjtec_rfid_recv_data[7]; // endof frame

FINISH:
    if ( auto_lock )
    {
        //printf(" >> kjtec rfid cmd mutex unlock\r\n");
        pthread_mutex_unlock(&kjtec_rfid_mutex);
    }

    if ( read_len > 9 )
    {
    //    printf("[KJTEC RFID] read cmd ret [%d] ++++++++++++++++++++ \r\n", read_len);
        _kjtec_rfid_cmd_proc(kjtec_rfid_recv_data+4, read_len-4 );
    //    printf("[KJTEC RFID] read cmd ret [%d] ++++++++++++++++++++ \r\n", read_len);
    }


    return ret_val;
}


// ==============================================================
// read thread
// ==============================================================
void kjtec_rfid__read_thread(void)
{
    unsigned char rfid_recv_data[128] = {0,};
//    unsigned char cmd_recv_buff[16] = {0,};

    int uart_ret = 0;

    while(_g_run_kjtec_rfid_thread_run)
    {
        if ( _kjtec_rfid_dev_init() == KJTEC_RFID_RET_FAIL )
        {
            sleep(1);
            continue;
        }

        //printf(" >> kjtec rfid thread mutex lock\r\n");
        pthread_mutex_lock(&kjtec_rfid_mutex);

        memset(rfid_recv_data, 0x00, 128);
        uart_ret =  mds_api_uart_read(_rfid_fd, rfid_recv_data,  sizeof(rfid_recv_data), KJTEC_RFID_UART_READ_TIMEOUT);

        //printf(" >> kjtec rfid thread  mutex unlock\r\n");
        pthread_mutex_unlock(&kjtec_rfid_mutex);

        if ( uart_ret > 0 )
        {
            //printf("[KJTEC RFID] read thread [%d] ++++++++++++++++++++ \r\n", uart_ret);
            // mds_api_debug_hexdump_buff(rfid_recv_data, uart_ret);
            _kjtec_rfid_cmd_proc(rfid_recv_data, uart_ret );
            //printf("[KJTEC RFID] read thread [%d] -------------------- \r\n", uart_ret);
        }
        else
        {
            //printf("[KJTEC RFID] bcm cannot read anything...\r\n");
            ;
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

} 


//.....
//.....
//.....





// ----------------------------------------------------------------------
// parser : 승객데이터
// ----------------------------------------------------------------------
static int _parse_cmd__passenger_data(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;

    char buffer[512] = {0,};
    char tmp_str[512] = {0,};
//    char tmp_str2[512] = {0,};

    strcpy(buffer, buf);

    mds_api_remove_char(buffer, tmp_str, 512, '/');
    memset(&buffer,0x00, 512);
    strcpy(buffer, tmp_str);

    mds_api_remove_char(buffer, tmp_str, 512, ':');
    memset(&buffer,0x00, 512);
    strcpy(buffer, tmp_str);

    mds_api_remove_char(buffer, tmp_str, 512, '.');
    memset(&buffer,0x00, 512);
    strcpy(buffer, tmp_str);

    //printf("%s() -> [%s]\r\n", __func__, buffer);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);

    strcpy(g_rfid_passenger_data_info.saved_timestamp, tr+2);

    // printf("%s()(%d) -> g_rfid_passenger_data_info.saved_timestamp [%s]\r\n", __func__, __LINE__, g_rfid_passenger_data_info.saved_timestamp);

    g_rfid_passenger_data_info.cmd_result = KJTEC_RFID_RET_SUCCESS;

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
    char  cmd_data_buff[512] = {0,};

    int max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 2;

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

    write_len += sprintf(cmd_data_buff+write_len, "%s", "01222605817");
    write_len += sprintf(cmd_data_buff+write_len, "%s", ",");
    write_len += sprintf(cmd_data_buff+write_len, "%s", time_str);
    
    //printf("[KJTEC RFID] write cmd [%s] \r\n", __func__);

    memset(&g_rfid_dev_info, 0x00, sizeof(g_rfid_dev_info));
    memset(&g_rfid_passenger_data_info, 0x00, sizeof(g_rfid_passenger_data_info));
    g_rfid_dev_info.cmd_result = -1;
    g_rfid_passenger_data_info.cmd_result = -1;
    
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__INIT_WAKEUP, cmd_data_buff, NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 2;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_dev_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            //printf("[KJTEC RFID] write cmd 1 [%s] -> resp success\r\n", __func__);
            break;
        }
        //printf("[KJTEC RFID] write cmd 1 [%s] -> resp wait...\r\n", __func__);
        usleep(50000);
    }

    if ( g_rfid_dev_info.cmd_result  != KJTEC_RFID_RET_SUCCESS )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 3 \r\n");
        printf( "[KJTEC RFID] SEND CMD :: WAKEUP - FAIL 3 \r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 2;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_passenger_data_info.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            // printf("[KJTEC RFID] write cmd 2 [%s] -> resp success\r\n", __func__);
            break;
        }
        //printf("[KJTEC RFID] write cmd 2 [%s] -> resp wait...\r\n", __func__);
        usleep(50000);
    }

    if ( g_rfid_passenger_data_info.cmd_result != KJTEC_RFID_RET_SUCCESS )
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
    //printf( "[KJTEC RFID] SEND CMD :: WAKEUP - SUCCESS [%s][%d][%s]\r\n",result->model_no, result->total_passenger_cnt, result->saved_timestamp);

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__wakeup(const char* buf)
{
    char *tr;
    char token_0[ ] = ",";
//    char token_1[ ] = "\r\n";
    char *temp_bp = NULL;
    
    char *p_cmd = NULL;
    char buffer[512] = {0,};
    
    memset(buffer, 0x00, sizeof(buffer));
    strcpy(buffer, buf);
    //printf("%s() -> [%s]\r\n", __func__, buffer);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);
    strcpy(g_rfid_dev_info.model_no,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);
    g_rfid_dev_info.total_passenger_cnt = atoi(tr);

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

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__CHK_READY, "Are you ready?", NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 4;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_chk_ready.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
        //    printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
     //   printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        usleep(50000);
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
    //printf( "[KJTEC RFID] SEND CMD :: ARE YOU READY? - SUCCESS [%d][%d]\r\n", result->cmd_result, result->data_result );


    return KJTEC_RFID_RET_SUCCESS;
    
}

static int _parse_cmd__chk_ready(const char* buf)
{
   // printf("%s() parse => [%s]\r\n", __func__, buf);

    if ( strstr(buf, "Ready") != NULL )
    {
    //    printf ("_parse_cmd__chk_ready is success\r\n");
        g_rfid_chk_ready.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
   //     printf ("_parse_cmd__chk_ready is fail\r\n");
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

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO\r\n");

    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr));
    g_rfid_all_clr.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__DATA_ALL_CLR, "Init Data", NULL);
    
    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 10;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_all_clr.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
            printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
        printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        sleep(1);
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
   // printf( "[KJTEC RFID] SEND CMD :: ALL CLR SAVED RFID USER INFO - SUCCESS [%d][%d]\r\n", result->cmd_result, result->data_result );

    return KJTEC_RFID_RET_SUCCESS;
}

static int _parse_cmd__data_all_clr(const char* buf)
{
  //  printf("%s() parse => [%s]\r\n", __func__, buf);
    if (( strstr(buf, "Success") != NULL ))
    {
  //      printf ("_parse_cmd__data_all_clr is success\r\n");
        g_rfid_all_clr.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
  //      printf ("_parse_cmd__data_all_clr is fail\r\n");
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
    //printf("[KJTEC RFID] write cmd [%s] \r\n", __func__);
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__GET_PASSENGER_DATA, "Bus Log Request", NULL);
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
    char buffer[512] = {0,};
    
    RFID_BOARDING_INFO_T tmp_boarding;

    memset(&tmp_boarding, 0x00, sizeof(tmp_boarding));

    memset(buffer, 0x00, sizeof(buffer));
    strcpy(buffer, buf);
    //printf("%s() -> [%s]\r\n", __func__, buffer);
    p_cmd = buffer;

    if ( p_cmd == NULL)
        return KJTEC_RFID_RET_FAIL;
    
    tr = strtok_r(p_cmd, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);
    strcpy(tmp_boarding.rfid_uid,tr);

    tr = strtok_r(NULL, token_0, &temp_bp);
    if(tr == NULL) return KJTEC_RFID_RET_FAIL;
    //printf("%s()(%d) -> [%s]\r\n", __func__, __LINE__, tr);
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
    int buf_len = strlen(buf);

    int prefix_cnt = 0;
    int suffix_cnt = 0;

    char real_data[512] = {0,};
    char one_data[64][128];
    int one_data_cnt = 0;

    char* tmp_p = NULL;
    char* tmp_one_start_p = NULL;
    
    int i = 0;

    LOGT(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: GET PASSENGER START \"%s\" \r\n", buf);
    
    if ( strstr(buf, "...") != NULL )
    {
        // rfid check interval change : end data.
        rfid_tool__env_set_rfid_chk_interval(RFID_CHK_DEFAULT_INTERVAL_SEC);
        return KJTEC_RFID_RET_SUCCESS;
    }
    
    memset(&one_data, 0x00, sizeof(one_data));

    if ( buf_len < 5 )
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 1\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    // check essential data..
    //buf[0];
    if (( buf[1] != '[' ) || (buf[buf_len-1] != ']'))
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 2\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    strncpy(real_data, buf+2, buf_len-3);
    //printf(" >> buf [%s]\r\n", buf);
    //printf(" >> real_data [%s]\r\n", real_data);
    
    // chk prefix
    tmp_p = real_data;
    while(1)
    {
      //  printf(" >> GET_PASSENGER_DATA_PREFIX_STR 1 target [%s]\r\n", tmp_p);
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
        // printf(" >> GET_PASSENGER_DATA_SUFFIX_STR 1 target [%s]\r\n", tmp_p);
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_SUFFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_SUFFIX_STR);
        suffix_cnt++ ;
    }

    // printf(" >> prefix cnt [%d] / suffix cnt [%d]\r\n" , prefix_cnt, suffix_cnt);
    if ( prefix_cnt != suffix_cnt)
    {
        LOGE(LOG_TARGET, "[KJTEC RFID] DATA PARSER :: FAIL case 3\r\n");
        return KJTEC_RFID_RET_FAIL;
    }

    // split one 
    tmp_p = real_data;
    buf_len = strlen(real_data);
    while(1)
    {
        // printf(" >> GET_PASSENGER_DATA_PREFIX_STR 2 target [%s]\r\n", tmp_p);
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_PREFIX_STR);
        if ( tmp_p == NULL )
            break;
        tmp_p += strlen(GET_PASSENGER_DATA_PREFIX_STR);

        tmp_one_start_p = tmp_p;

        // printf(" >> GET_PASSENGER_DATA_SUFFIX_STR 2 target [%s]\r\n", tmp_p);
        tmp_p = strstr(tmp_p, GET_PASSENGER_DATA_SUFFIX_STR);
        if ( tmp_p == NULL )
            break;
        
        strncpy(one_data[one_data_cnt], tmp_one_start_p, strlen(tmp_one_start_p) - strlen(tmp_p));
        // printf(" ---> one data : [%d] => [%s] \r\n", one_data_cnt, one_data[one_data_cnt]);
        one_data_cnt++;
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
    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__GET_PASSENGER_DATA_SUCCESS, "Bus Log Result,1,", NULL);
    return 0;
}


// -------------------------------------------------------------------------
// cmd : write rfid data.
// -------------------------------------------------------------------------
int kjtec_rfid__dev_write_rfid_data(int flag, char* rfid_user_str)
{
    int max_cmd_wait_time;

    char cmd_str[RFID_CMD_PASSENGER_STR_MAX_LEN] = {0,};

    LOGT(LOG_TARGET, "[KJTEC RFID] SEND CMD :: WRITE RFID USER INFO\r\n");

    // printf("[KJTEC RFID] write rfid data [%s] \r\n", __func__);
    //printf("flag [%d] rfid_user_str [%d]/[%d] is ==> \"%s\" \r\n", flag, strlen(rfid_user_str),RFID_CMD_PASSENGER_STR_MAX_LEN, rfid_user_str);

    if ( strlen(rfid_user_str) > RFID_CMD_PASSENGER_STR_MAX_LEN )
        return KJTEC_RFID_RET_FAIL;

    cmd_str[0] = flag;

    sprintf(cmd_str + 1, "%s", rfid_user_str);

    //printf("[KJTEC RFID] write cmd [%s] \r\n", __func__);

    memset(&g_rfid_all_clr, 0x00, sizeof(g_rfid_all_clr));
    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_FAIL;

    _kjtec_rfid_cmd(1, RFID_CMD_ID_REQ__SAVE_PASSENGER_DATA, cmd_str, NULL);

    max_cmd_wait_time = KJTEC_RFID_CMD_RESP_WAIT_TIME * 20;
    while(max_cmd_wait_time--)
    {
        if ( g_rfid_save_passenger_data.cmd_result  == KJTEC_RFID_RET_SUCCESS )
        {
        //    printf("[KJTEC RFID] write cmd [%s] -> resp success\r\n", __func__);
            break;
        }
       //  printf("[KJTEC RFID] write cmd [%s] -> resp wait...\r\n", __func__);
        sleep(1);
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

    if ( strstr(buf, "DataResult,1,") != NULL )
    {
        //printf ("_parse_cmd__save_passenger_data is success\r\n");
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_SUCCESS;
    }
    else 
    {
       // printf ("_parse_cmd__save_passenger_data is fail\r\n");
        g_rfid_save_passenger_data.data_result = KJTEC_RFID_RET_FAIL;
    }

    g_rfid_save_passenger_data.cmd_result = KJTEC_RFID_RET_SUCCESS;
    return KJTEC_RFID_RET_SUCCESS;
  
}



#if 0
void main()
{
    

    int ret = 0;



    {
        RFID_DEV_INFO_T dev_info = {0,};
        ret = kjtec_rfid__dev_wakeup(&dev_info);
        if ( ret == KJTEC_RFID_RET_SUCCESS )
        {
            printf("kjtec_rfid__dev_wakeup :: ret [%d]\r\n",ret);
            printf("kjtec_rfid__dev_wakeup :: dev_info.cmd_result [%d]\r\n",dev_info.cmd_result);
            printf("kjtec_rfid__dev_wakeup :: dev_info.model_no [%s]\r\n",dev_info.model_no);
            printf("kjtec_rfid__dev_wakeup :: dev_info.total_passenger_cnt [%d]\r\n",dev_info.total_passenger_cnt);
            printf("kjtec_rfid__dev_wakeup :: dev_info.saved_timestamp [%s]\r\n",dev_info.saved_timestamp);
        }
    }

    {
        RIFD_CHK_READY_T dev_ready = {0,};

        ret = kjtec_rfid__dev_ready_chk(&dev_ready);
        if ( ret == KJTEC_RFID_RET_SUCCESS )
        {
            printf("kjtec_rfid__dev_ready_chk :: ret [%d]\r\n",ret);
            printf("kjtec_rfid__dev_ready_chk :: dev_ready.cmd_result [%d]\r\n",dev_ready.cmd_result);
            printf("kjtec_rfid__dev_ready_chk :: dev_ready.data_result [%d]\r\n", dev_ready.data_result);
        }
    }

    {
        RIFD_DATA_ALL_CLR_T rfid_data_all_clr = {0,};

        ret = kjtec_rfid__dev_rfid_all_clear(&rfid_data_all_clr);
        if ( ret == KJTEC_RFID_RET_SUCCESS )
        {
            printf("kjtec_rfid__dev_rfid_all_clear :: ret [%d]\r\n",ret);
            printf("kjtec_rfid__dev_rfid_all_clear :: rfid_data_all_clr.cmd_result [%d]\r\n",rfid_data_all_clr.cmd_result);
            printf("kjtec_rfid__dev_rfid_all_clear :: rfid_data_all_clr.data_result [%d]\r\n", rfid_data_all_clr.data_result);
        }
    }

    kjtec_rfid__dev_write_rfid_data();




    while(1)
    {
        kjtec_rfid__dev_rfid_req();
        sleep(20);
    }
}

#endif

