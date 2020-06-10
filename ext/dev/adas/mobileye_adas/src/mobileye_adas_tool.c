#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include <logd_rpc.h>
#include <mdsapi/mds_api.h>

#include "mobileye_adas.h"
#include "mobileye_adas_mgr.h"
#include "mobileye_adas_protocol.h"
#include "mobileye_adas_tool.h"
#include "mobileye_adas_uart_util.h"


int mobileye_adas__tool_device_argument(char* buff, int buff_len, char* argv[])
{
    unsigned char ret_buff[MAX_MOBILEYE_ADAS_RET_BUFF_SIZE] = {0,};
    
    char    *base = 0;
    int     t_argc = 0;
   // char*   t_argv[10] = {0,};
    
    int i = 0;
    
    if ( buff_len <= 0 )
        return 0;

    memcpy(ret_buff, buff, buff_len);
    memset(buff, 0x00, MAX_MOBILEYE_ADAS_RET_BUFF_SIZE);
    
    base = (char*)buff;
    
    //t_argv[t_argc] = base;
    argv[t_argc] = base;
    t_argc++;
    
    for (i = 0 ; i < buff_len ; i++)
    {
        switch(ret_buff[i])
        {
            case ',':
            case '*':   // argument
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
#if 0
{
	char *tr;
    char token_0[ ] = ":,";
    char *temp_bp = NULL;
    char *p_cmd = NULL;

    int ret_val = 0;

    char tmp_str[1024] ={0,};

    // char tmp_argv[20][128] = {0,};

    // remove space
	at_remove_char(buff, tmp_str, sizeof(tmp_str), ' ');
    
	p_cmd = tmp_str;

    tr = strtok_r(p_cmd, token_0, &temp_bp);
	while(tr != NULL)
	{
        printf("tr is [%s]\r\n", tr);
        strcpy(tmp_argv[ret_val], tr);
        ret_val ++;

        tr = strtok_r(NULL, token_0, &temp_bp);
	}

    return ret_val;
}
#endif

int mobileye_adas__tool_chk_invalid_num_data(const char* data)
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
