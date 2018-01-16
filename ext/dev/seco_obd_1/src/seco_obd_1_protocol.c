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
#include <base/devel.h>

#include "seco_obd_1.h"
#include "seco_obd_1_mgr.h"
#include "seco_obd_1_protocol.h"
#include "seco_obd_1_util.h"

//#define DEBUG_MSG_OBD_CMD_TA1
//#define DEBUG_MSG_OBD_CMD_TA2

int _seco_obd_uart_chk()
{
    int i = 0;
    
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (seco_obd_1_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}

	if (is_sec_obd_1_init() == OBD_RET_FAIL)
	{
		return OBD_CMD_UART_INIT_FAIL;
	}

    return OBD_RET_SUCCESS;
}

int _seco_obd_1_devide_argument(char* buff, int buff_len, char* argv[])
{
    unsigned char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
    
    char    *base = 0;
    int     t_argc = 0;
   // char*   t_argv[10] = {0,};
    
    int i = 0;
    
    if ( buff_len <= 0 )
        return 0;

    memcpy(ret_buff, buff, buff_len);
    memset(buff, 0x00, MAX_RET_BUFF_SIZE);
    
    base = (char*)buff;
    
    //t_argv[t_argc] = base;
    argv[t_argc] = base;
    t_argc++;
    
    for (i = 0 ; i < buff_len ; i++)
    {
        switch(ret_buff[i])
        {
            case ',':
                *base = '\0';
                //t_argv[t_argc] = base + 1;
                argv[t_argc] = base + 1;
                t_argc++;
                break;
            case ' ': // remove space..
                break;
            default:
                *base = ret_buff[i];
                break;
        }
        base++;
    }
    
    /*
    for( i = 0 ; i < t_argc ; i++)
    {
        printf("1: [%d]/[%d] => [%s]\r\n", i, t_argc, t_argv[i]);
    }
    */
    
    return t_argc;
}

static int _chk_obd_invalid_num_data(const char* data)
{
    int i = 0;
    int is_num = 0;

    int data_len = strlen(data);

    if ( (strcasecmp(data, "x") == 0) || (data_len == 0) || (strcmp(data, "*") == 0) )
    {
        return -1;
    }

    if ( (strstr(data, "x") != NULL) || (data_len == 0) || (strstr(data, "*") != NULL) )
    {
        return -1;
    }

    for (i = 0 ; i < data_len ; i++)
    {
        if ( ( ( data[i] >= 060 ) && ( data[i] <= 071 ) ) || (data[i] == '.') || (data[i] =='-') )
        {
            is_num = 1;
            continue;
        }
        else 
        {
            is_num = 0 ;
            break;
        }
    }

    if (is_num)
        return 1;
    else 
        return -1;
}
//#define DEBUG_MSG_CONVERT_MSG

static int _convert_obd_data_num(const char* data, const char offset_type, const char* off_set_val)
{
    float tmp_float_data = 0;
    float tmp_float_offset = 0;

    float offset_0_5 = 0.5;

    int return_val = -1;

    if ( _chk_obd_invalid_num_data(data) == -1)
    {
        #ifdef DEBUG_MSG_CONVERT_MSG
        printf(" >> convert float : input [%s] is not number\r\n", data);
        #endif
        return 0xffffffff;
    }

    if ( data != NULL )
        tmp_float_data = atof(data);
    
    if ( off_set_val != NULL)
        tmp_float_offset = atof(off_set_val);

    #ifdef DEBUG_MSG_CONVERT_MSG
    printf(" >> convert float : input [%f]\r\n", tmp_float_data);
    printf(" >> convert float : offset [%c] [%f]\r\n", offset_type, tmp_float_offset);
    #endif

    if ( offset_type == '+' )
    {
        tmp_float_data = tmp_float_data + tmp_float_offset;
    }
    else if ( offset_type == '-' )
    {
        tmp_float_data = tmp_float_data - tmp_float_offset;
    }
    else if ( offset_type == '*' )
    {
        tmp_float_data = tmp_float_data * tmp_float_offset;
    }
    else if ( offset_type == '/' )
    {
        if ( tmp_float_offset > 0 )
            tmp_float_data = tmp_float_data / tmp_float_offset;
        else
            devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    tmp_float_data = tmp_float_data + offset_0_5;
    
    return_val = (int)tmp_float_data;
    
    #ifdef DEBUG_MSG_CONVERT_MSG
    printf(" >> convert float : result [%d] \r\n", return_val);
    #endif

    return return_val;
}



int get_seco_obd_1_ver(char* buff)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
//    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
        {
            LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] obd init fail[%d]\r\n",__func__,__LINE__,obdchk_ret);
            return obdchk_ret;
        }
    }
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] start \r\n",__func__,__LINE__);
    
    read_cnt = seco_obd_1_write_cmd_resp("OBD+INF+", "VER", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);
    if ( read_cnt < 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }    
    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }

    /*
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }
    */
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] success : ver [%s]\r\n",__func__,__LINE__,argv[0]);
    printf("[OBD API] %s()-[%d] success : ver [%s]\r\n",__func__,__LINE__,argv[0]);
    if ( buff != NULL )
        strcpy(buff, argv[0]);

    return OBD_RET_SUCCESS;
}

int get_seco_obd_1_fueltype(char* buff)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
        {
            LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] obd init fail[%d]\r\n",__func__,__LINE__,obdchk_ret);
            return obdchk_ret;
        }
    }
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] start \r\n",__func__,__LINE__);
    
    read_cnt = seco_obd_1_write_cmd_resp("OBD+INF+", "VEH", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);
    if ( read_cnt < 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }    
    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }

    
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }
    
    // 1 : 제조사명
    // 2 : 년식
    // 3 : 유종
    // 4 : 배기량
    // 5 기통
    // 6 차명
    // 7 : 등급
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] success : fueltype [%s]\r\n",__func__,__LINE__, argv[2]);
    printf("[OBD API] %s()-[%d] success : fueltype [%s]\r\n",__func__,__LINE__, argv[2]);
    if ( buff != NULL )
        strcpy(buff, argv[2]);

    return OBD_RET_SUCCESS;
}


int get_seco_obd_1_serial(char* buff)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
//    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
        {
            LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] obd init fail[%d]\r\n",__func__,__LINE__,obdchk_ret);
            return obdchk_ret;
        }
    }
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] start \r\n",__func__,__LINE__);

    
    read_cnt = seco_obd_1_write_cmd_resp("OBD+INF+", "SRN", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);
        
    if ( read_cnt < 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }

    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] success : serial [%s]\r\n",__func__,__LINE__,argv[0]);
    printf("[OBD API] %s()-[%d] success : serial [%s]\r\n",__func__,__LINE__,argv[0]);

    if ( buff != NULL )
        strcpy(buff, argv[0]);
    return OBD_RET_SUCCESS;
}


int set_seco_obd_1_total_distance(int total_distance)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
    char cmd_data[128] ={0,};

	int error_code = 0;
    int read_cnt = 0;
    
//    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
        {
            LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] obd init fail[%d]\r\n",__func__,__LINE__,obdchk_ret);
            return obdchk_ret;
        }
    }
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] start \r\n",__func__,__LINE__);

    sprintf(cmd_data, "%d",total_distance);
    read_cnt = seco_obd_1_write_cmd_resp("OBD+INF+", "TDD", eCMD_TYPE_INPUT_VALUE, cmd_data, ret_buff, &error_code);
        
    if ( read_cnt < 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }

    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] success : serial [%s]\r\n",__func__,__LINE__,argv[0]);
    printf("[OBD API] %s()-[%d] success : serial [%s]\r\n",__func__,__LINE__,argv[0]);

    if ( strstr(argv[0],"OK") == NULL )
        return OBD_RET_FAIL;
    else
        return OBD_RET_SUCCESS;

}

int start_seco_obd_1_broadcast_msg(int interval_sec, char* factor_list)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
    char cmd_data[128] ={0,};

	int error_code = 0;
    int read_cnt = 0;
    
//    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
        {
            LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] obd init fail[%d]\r\n",__func__,__LINE__,obdchk_ret);
            return obdchk_ret;
        }
    }
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] start \r\n",__func__,__LINE__);
    
    sprintf(cmd_data, "%s",factor_list);
    read_cnt = seco_obd_1_write_cmd_resp("OBD+SBR+", "BMC", eCMD_TYPE_INPUT_VALUE, cmd_data, ret_buff, &error_code);
    //printf("obd serial return : [%s] / [%d]\r\n", ret_buff, read_cnt);
    
    if ( read_cnt < 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }
    /*
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }
    */
    
    if ( strcmp(argv[0],"OK") != 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] return val err [%s]\r\n",__func__,__LINE__,argv[0]);
        return OBD_RET_FAIL;
    }
   

    sprintf(cmd_data, "%d",interval_sec*1000);
    read_cnt = seco_obd_1_write_cmd_resp("OBD+SBR+", "BRT", eCMD_TYPE_INPUT_VALUE, cmd_data, ret_buff, &error_code);
    //printf("obd serial return : [%s] / [%d]\r\n", ret_buff, read_cnt);

    if ( read_cnt < 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }
    
    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }
    /*
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }
    */
    
    if ( strcmp(argv[0],"OK") != 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] return val err [%s]\r\n",__func__,__LINE__,argv[0]);
        return OBD_RET_FAIL;
    }

    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] success \r\n",__func__,__LINE__);
    printf("[OBD API] %s()-[%d] success \r\n",__func__,__LINE__);
    return OBD_RET_SUCCESS;
}

int stop_seco_obd_1_broadcast_msg()
{
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
//    unsigned char cmd_buff[MAX_RET_BUFF_SIZE] = {0,};
    char cmd_data[128] ={0,};

	int error_code = 0;
    int read_cnt = 0;
    
//    int total_fuel_usage = 0;

//    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
        {
            LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] obd init fail[%d]\r\n",__func__,__LINE__,obdchk_ret);
            return obdchk_ret;
        }
    }
    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] start \r\n",__func__,__LINE__);

    sprintf(cmd_data, "0");
    read_cnt = seco_obd_1_write_cmd_resp("OBD+SBR+", "BRP", eCMD_TYPE_INPUT_VALUE, cmd_data, ret_buff, &error_code);

    if ( read_cnt <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] send cmd fail [%d]\r\n",__func__,__LINE__,read_cnt);
        return OBD_RET_FAIL;
    }
    
    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if ( argc <= 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] devide cmd fail [%d]\r\n",__func__,__LINE__,argc);
        return OBD_RET_FAIL;
    }
    /*
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }
    */
    
    if ( strcmp(argv[0],"OK") != 0 )
    {
        LOGE(eSVC_MODEL,"[OBD API] %s()-[%d] return val err [%s]\r\n",__func__,__LINE__,argv[0]);
        return OBD_RET_FAIL;
    }
   

    LOGT(eSVC_MODEL,"[OBD API] %s()-[%d] success \r\n",__func__,__LINE__);
    printf("[OBD API] %s()-[%d] success \r\n",__func__,__LINE__);
    return OBD_RET_SUCCESS;
}




static unsigned int _convert_DTC_result(char* dtc_str, int str_len)
{
    // 1-F_P0001
    char tmp_char[4] = {0,};
//    char buff[4] = {0,};
    
    unsigned int result_number = 0; // 4byte
    unsigned int shift_bit_size = 0;
    // -----------------------------
    unsigned int dtc_arg_1 = 0;  int dtc_arg_1_size_bit = 12; // 12bit
    unsigned int dtc_arg_2 = 0;  int dtc_arg_2_size_bit = 4; // 4bit
    unsigned int dtc_arg_3 = 0;  int dtc_arg_3_size_bit = 2; // 2bit
    unsigned int dtc_arg_4 = 0;  int dtc_arg_4_size_bit = 2; // 2bit
    unsigned int dtc_arg_5 = 0;  int dtc_arg_5_size_bit = 4; // 4bit
    unsigned int dtc_arg_6 = 0;  int dtc_arg_6_size_bit = 4; // 4bit
    unsigned int dtc_arg_7 = 0;  int dtc_arg_7_size_bit = 4; // 4bit

    if ( str_len != 9 )
        return 0xffffffff;
    
    printf("DTC CMD : parsing start [%s] / [%d]\r\n", dtc_str, str_len);

    // 1st argument : (1)-F_P0001
    memset(tmp_char, 0x00, 4);
    tmp_char[0] = dtc_str[0];
    dtc_arg_1 = atoi(tmp_char);
    dtc_arg_1 = dtc_arg_1 & 0x00000FFF;    // 12bit masking
    result_number = result_number | dtc_arg_1;
    shift_bit_size += dtc_arg_1_size_bit;

    printf("dtg parse result 1 ==> [0x%08x]\r\n", result_number);

    // 2nd argument : 1-(F)_P0001
    if ( dtc_str[2] == 'F' )
        dtc_arg_2 = 0x0F;
    else if ( dtc_str[2] == 'R' )
        dtc_arg_2 = 0;
    else
        dtc_arg_2 = 0xFF;
    dtc_arg_2 = dtc_arg_2 << shift_bit_size;    // 12 bit shift
    result_number = result_number | dtc_arg_2;
    shift_bit_size += dtc_arg_2_size_bit;

    printf("dtg parse result 2 ==> [0x%08x]\r\n", result_number);

    // 3rd argment : 1-F_(P)0001
     if ( dtc_str[4] == 'P' )
        dtc_arg_3 = 0;
    else if ( dtc_str[4] == 'C' )
        dtc_arg_3 = 1;
    else if ( dtc_str[4] == 'B' )
        dtc_arg_3 = 2;
    else if ( dtc_str[4] == 'U' )
        dtc_arg_3 = 3;
    dtc_arg_3 = dtc_arg_3 << shift_bit_size;    // 12 + 4 bit shift
    result_number = result_number | dtc_arg_3;
    shift_bit_size += dtc_arg_3_size_bit;

    printf("dtg parse result 3 ==> [0x%08x]\r\n", result_number);

    // 4th argment : 1-F_P(0)001
    memset(tmp_char, 0x00, 4);
    tmp_char[0] = dtc_str[5];
    dtc_arg_4 = atoi(tmp_char);
    dtc_arg_4 = dtc_arg_4 << shift_bit_size;    // 12 + 4 bit shift
    result_number = result_number | dtc_arg_4;
    shift_bit_size += dtc_arg_4_size_bit;

    printf("dtg parse result 4 ==> [0x%08x]\r\n", result_number);

    // 5th argment : 1-F_P0(0)01
    memset(tmp_char, 0x00, 4);
    tmp_char[0] = dtc_str[6];
    dtc_arg_5 = (int)strtol(tmp_char, NULL, 16);
    dtc_arg_5 = dtc_arg_5 << shift_bit_size;    // 12 + 4 bit shift
    result_number = result_number | dtc_arg_5;
    shift_bit_size += dtc_arg_5_size_bit;

    printf("dtg parse result 5 ==> [0x%08x]\r\n", result_number);

    // 6th argment : 1-F_P00(0)1
    memset(tmp_char, 0x00, 4);
    tmp_char[0] = dtc_str[7];
    dtc_arg_6 = (int)strtol(tmp_char, NULL, 16);
    dtc_arg_6 = dtc_arg_6 << shift_bit_size;    // 12 + 4 bit shift
    result_number = result_number | dtc_arg_6;
    shift_bit_size += dtc_arg_6_size_bit;

    printf("dtg parse result 6 ==> [0x%08x]\r\n", result_number);

    // 7th argment : 1-F_P000(1)
    memset(tmp_char, 0x00, 4);
    tmp_char[0] = dtc_str[8];
    dtc_arg_7 = (int)strtol(tmp_char, NULL, 16);
    dtc_arg_7 = dtc_arg_7 << shift_bit_size;    // 12 + 4 bit shift
    result_number = result_number | dtc_arg_7;
    shift_bit_size += dtc_arg_7_size_bit;

    printf("dtg parse result ==> [0x%08x]\r\n", result_number);

    return result_number;
}


typedef struct seco_cmd_ta1_debug_msg_str
{
    int idx;
    char* str;
}__attribute__((packed))SECO_CMD_TA1_DEBUG_MSG_STR_T;

SECO_CMD_TA1_DEBUG_MSG_STR_T ta1_debug_str[] =
{
    {eOBD_CMD_SRR_TA1_CLV , "Calculated LOAD Value"},
    {eOBD_CMD_SRR_TA1_MAP , "Intake Manifold absolute pressure"},
    {eOBD_CMD_SRR_TA1_RPM , "Engine RPM"},
    {eOBD_CMD_SRR_TA1_SPD , "Vehicle Speed Sensor"},
	{eOBD_CMD_SRR_TA1_MAF , "Air Flow Rate from Mass Air Flow Sensor"},
	{eOBD_CMD_SRR_TA1_TPA , "Absoulte Throttle Position"},
	{eOBD_CMD_SRR_TA1_BS1 , "Lambda"},
    {eOBD_CMD_SRR_TA1_BAV , "Control module voltage"},
    {eOBD_CMD_SRR_TA1_APD , "Accelerator Pedal Position"},
    {eOBD_CMD_SRR_TA1_EFR , "Engine Fuel Rate"},
    {eOBD_CMD_SRR_TA1_EAT , "Actual Engine"},
    {eOBD_CMD_SRR_TA1_CED , "Commanded EGR"},
    {eOBD_CMD_SRR_TA1_AED , "Actual EGR Duty"},
    {eOBD_CMD_SRR_TA1_EFT , "Engine Friction"},
    {eOBD_CMD_SRR_TA1_COT , "Engine Coolant Temperature"},
    {eOBD_CMD_SRR_TA1_ATS , "Intake Air Temperature"},
    {eOBD_CMD_SRR_TA1_TB1 , "Catalyst Temperature  bank 1"},
    {eOBD_CMD_SRR_TA1_EGR , "Commanded EGR"},
    {eOBD_CMD_SRR_TA1_EGE , "Commanded EGR error"},
    {eOBD_CMD_SRR_TA1_EST , "Time Since Engine Start"},
    {eOBD_CMD_SRR_TA1_BRO , "Barometic Pressure"},
    {eOBD_CMD_SRR_TA1_ABT , "Ambient air Temperature"}
};


int get_seco_obd_cmd_ta1(SECO_CMD_DATA_SRR_TA1_T* p_ta1_buff)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
    int i =0;
    char* argv[30] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
            return obdchk_ret;
    }
    
    if ( p_ta1_buff == NULL )
        return OBD_RET_FAIL;

    read_cnt = seco_obd_1_write_cmd_resp("OBD+SRR+", "TA1", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);
    //printf("obd cmd return : [%s] / [%d]\r\n", ret_buff, read_cnt);
    
    if ( read_cnt <= 0 )
    {
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
        return 0;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    if (( argc != eOBD_CMD_SRR_TA1_MAX_CNT ) || ( argc != OBD_CMD_SRR_TA1_MAX_CNT ))
    {
        printf("get cmd ta1 is invalid  : [%d]/[%d] \r\n", argc, eOBD_CMD_SRR_TA1_MAX_CNT );
        return OBD_CMD_RET_CMD_CNT_ERR;
    }

    for( i = 0 ; i < argc ; i++)
    {
        int convert_num_data = 0 ;
        
        #ifdef DEBUG_MSG_OBD_CMD_TA1
        printf("-----------------------------------------------------------------\r\n");
        printf("cmd ta1 [%s]  => [%s] \r\n", ta1_debug_str[i].str, argv[i] );
        #endif

        if ( i == eOBD_CMD_SRR_TA1_CLV ) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        //else if ( i == eOBD_CMD_SRR_TA1_MAP) {
        //}
        else if ( i == eOBD_CMD_SRR_TA1_RPM) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "4");
        }
        //else if ( i == eOBD_CMD_SRR_TA1_SPD) {
        //}
        else if ( i == eOBD_CMD_SRR_TA1_MAF) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "0.01");
        }
        else if ( i == eOBD_CMD_SRR_TA1_TPA) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        else if ( i == eOBD_CMD_SRR_TA1_BS1) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "32767.5");
        }
        else if ( i == eOBD_CMD_SRR_TA1_BAV) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "1000");
        }
        else if ( i == eOBD_CMD_SRR_TA1_APD) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        else if ( i == eOBD_CMD_SRR_TA1_EFR) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "20");
        }
        else if ( i == eOBD_CMD_SRR_TA1_EAT) {
            convert_num_data = _convert_obd_data_num(argv[i], '+', "125");
        }
        else if ( i == eOBD_CMD_SRR_TA1_CED) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        else if ( i == eOBD_CMD_SRR_TA1_AED) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        else if ( i == eOBD_CMD_SRR_TA1_EFT) {
            convert_num_data = _convert_obd_data_num(argv[i], '+', "125");
        }
        else if ( i == eOBD_CMD_SRR_TA1_COT) {
            convert_num_data = _convert_obd_data_num(argv[i], '/', "100");
            convert_num_data += 40;
        }
        else if ( i == eOBD_CMD_SRR_TA1_ATS) {
            convert_num_data = _convert_obd_data_num(argv[i], '/', "100");
            convert_num_data += 40;
        }
        else if ( i == eOBD_CMD_SRR_TA1_TB1) {
            convert_num_data = _convert_obd_data_num(argv[i], '+', "40");
            convert_num_data *= 10;
        }
        //else if ( i == eOBD_CMD_SRR_TA1_EST) {
        //}
        else if ( i == eOBD_CMD_SRR_TA1_BRO) {
            convert_num_data = _convert_obd_data_num(argv[i], '/', "100");
        }
        else if ( i == eOBD_CMD_SRR_TA1_ABT) {
            convert_num_data = _convert_obd_data_num(argv[i], '/', "100");
            convert_num_data += 40;
        }
        else if ( i == eOBD_CMD_SRR_TA1_EGR) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        else if ( i == eOBD_CMD_SRR_TA1_EGE) {
            convert_num_data = _convert_obd_data_num(argv[i], '*', "2.55");
        }
        else {
            convert_num_data = _convert_obd_data_num(argv[i], ' ', NULL);
        }

        

        p_ta1_buff->obd_data[i].idx = i;

        if ( convert_num_data >= 0)
        {
            //printf(" >> return data case1 :: [%d] \r\n", convert_num_data);
            p_ta1_buff->obd_data[i].data = convert_num_data;
        }
        else
        {
            //printf(" >> return data case2 :: [%d] \r\n", convert_num_data);
            p_ta1_buff->obd_data[i].data = 0xffffffff;
        }
    }

    return OBD_RET_SUCCESS;
}

// #define DEBUG_MSG_OBD_CMD_TA2
int get_seco_obd_cmd_ta2(SECO_CMD_DATA_SRR_TA2_T* p_ta2_buff)
{   
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
    int i =0;
    char* argv[30] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
            return OBD_RET_FAIL;
    }
    
    if ( p_ta2_buff == NULL )
        return OBD_RET_FAIL;

    read_cnt = seco_obd_1_write_cmd_resp("OBD+SRR+", "TA2", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);
    //printf("obd serial return : [%s] / [%d]\r\n", ret_buff, read_cnt);
    if ( read_cnt <= 0 )
    {
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
        return 0;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    
    if  ( ( argc != eOBD_CMD_SRR_TA2_MAX_CNT ) || ( argc != OBD_CMD_SRR_TA2_MAX_CNT ))
    {
        printf("get cmd ta2 is invalid  : [%d]/[%d] \r\n", argc, eOBD_CMD_SRR_TA2_MAX_CNT );
        return OBD_CMD_RET_CMD_CNT_ERR;
    }

    for( i = 0 ; i < argc ; i++)
    {
        int convert_num_data = 0 ;
        // test code ...
        // p_ta2_buff->obd_data[i].data = _convert_DTC_result("1-F_B011F", strlen("1-F_P0001"));
        // printf("dtc result signed int val [0x%08x]\r\n", p_ta2_buff->obd_data[i].data );
        
        convert_num_data = _convert_obd_data_num(argv[i], ' ' , NULL);
        
        
        p_ta2_buff->obd_data[i].idx = i;
        p_ta2_buff->obd_data[i].data = 0xffffffff;

        if ( convert_num_data >= 0)
        {
            //printf(" >> return data case1 :: [%d] \r\n", convert_num_data);
            // 
            if ( i == eOBD_CMD_SRR_TA2_DTC )
            {
                p_ta2_buff->obd_data[i].data = 0; 
                p_ta2_buff->obd_data[i].data = _convert_DTC_result(argv[i], strlen(argv[i]));
            }
            else
                p_ta2_buff->obd_data[i].data = convert_num_data;
        }

    }

    return OBD_RET_SUCCESS;
}


int set_obd_auto_poweroff_sec(int sec)
{
// OBD+SRR+TDD=38419278
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
    char cmd_buff[MAX_RET_BUFF_SIZE] = {0,};

	int error_code = 0;
    int read_cnt = 0;
    
//    int total_fuel_usage = 0;

    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
            return obdchk_ret;
    }
    
    sprintf(cmd_buff,"ENB,25000,%d", sec);
    read_cnt = seco_obd_1_write_cmd_resp("OBD+INF+", "PWR", eCMD_TYPE_INPUT_VALUE, cmd_buff, ret_buff, &error_code);

    if ( read_cnt <= 0 )
        return 0;

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }

    return 0;

    // printf("obd total distance :: [%d]\r\n"
}



static int _get_obd_total_distance()
{
// OBD+SRR+TDD=38419278
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
    int total_distance = 0;

    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
            return obdchk_ret;
    }
    
    read_cnt = seco_obd_1_write_cmd_resp("OBD+SRR+", "TDD", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);

    if ( read_cnt <= 0 )
    {
        devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
        return 0;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }

    total_distance = atoi(argv[0]);

    printf("obd total distance [%d]\r\n", total_distance);

    return total_distance;

    // printf("obd total distance :: [%d]\r\n"
}


int get_obd_total_distance()
{
    int ret_val = 0;
    int max_retry = MAX_RETRY_GET_TRIP_DATA_FACTOR;
   
    while(max_retry--)
    {
         ret_val = _get_obd_total_distance();
         if ( ret_val > 0 )
            break;
        usleep(500);
    }

    return ret_val;
}



static int _get_obd_total_fuel_usage()
{
// OBD+SRR+TDD=38419278
    char ret_buff[MAX_RET_BUFF_SIZE] = {0,};
	int error_code = 0;
    int read_cnt = 0;
    
    int total_fuel_usage = 0;

    int i =0;
    char* argv[20] = {0,};
    int argc = 0;
    // common obd check ...
    {
        int obdchk_ret = 0;
        obdchk_ret = _seco_obd_uart_chk();
        if (obdchk_ret != OBD_RET_SUCCESS)
            return obdchk_ret;
    }
    
    read_cnt = seco_obd_1_write_cmd_resp("OBD+SRR+", "FCT", eCMD_TYPE_GET_VALUE, NULL, ret_buff, &error_code);

    if ( read_cnt <= 0 )
    {
        devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
        return 0;
    }

    argc = _seco_obd_1_devide_argument(ret_buff, read_cnt, argv);
    
    for( i = 0 ; i < argc ; i++)
    {
        printf("devide value : [%d]/[%d] => [%s]\r\n", i, argc, argv[i]);
    }

    total_fuel_usage = atoi(argv[0]);

    printf("obd total fuel usage [%d]\r\n", total_fuel_usage);

    return total_fuel_usage;

    // printf("obd total distance :: [%d]\r\n"
}

int get_obd_total_fuel_usage()
{
    int ret_val = 0;
    int max_retry = MAX_RETRY_GET_TRIP_DATA_FACTOR;
   
    while(max_retry--)
    {
         ret_val = _get_obd_total_fuel_usage();
         if ( ret_val > 0 )
            break;
        usleep(500);
    }

    return ret_val;
}



int get_obd_vehicle_info(SECO_CMD_DATA_VEHICLE_INFO_T* info)
{
    return -1;
}

