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


#include "katech-obd.h"
#include "katech-obd-util.h"


int chk_obd_port()
{
	int i = 0;
	
	// obd uart �� ����������, �ٽ� ����.
	// ������ ���⸦ �õ��ϸ�, ���� fail �ϰ��� fail �� ����
	for(i = 0; i < MAX_OBD_UART_INIT_TRY_CNT ; i++)
	{
		if (katech_obd_init() == OBD_RET_SUCCESS)
			break;
		sleep(1);
	}
	
	if (is_katech_obd_init() == OBD_RET_FAIL)
	{
		return OBD_CMD_UART_INIT_FAIL;
	}
	
	return OBD_RET_SUCCESS;
}

int get_obd_info()
{
	unsigned char ret_buff[512] = {0,};
	int error_code = 0;
	int read_cnt = 0;
	
	
	// chk & open
	if ( chk_obd_port() != OBD_RET_SUCCESS)
		return OBD_CMD_UART_INIT_FAIL;
	
	read_cnt = katech_obd_write_cmd_resp("RI", NULL, 0, ret_buff, &error_code);
    
    //printf("%s() :: ret_buff is [%s]\r\n", __func__, ret_buff);
    
    if ( error_code == OBD_ERROR_CODE__SUCCESS)
    {
        //printf("%s :: SUCCESS :: error code [%d] / read_cnt [%d]\r\n", __func__, error_code, read_cnt);
        return OBD_RET_SUCCESS;
    }
    else
    {
        //printf("%s :: FAIL :: error code [%d] / read_cnt [%d]\r\n",__func__,  error_code, read_cnt);
        return OBD_RET_FAIL;
    }
}

int get_obd_data_cnt(int* data1_cnt, int* data2_cnt)
{
	unsigned char ret_buff[512] = {0,};
	int error_code = 0;
	int read_cnt = 0;
	
    char token[ ] = ",|";
	char *pAtcmd = NULL;
	char *tr;
    char *temp_bp = NULL;
    
	// chk & open
	if ( chk_obd_port() != OBD_RET_SUCCESS)
		return OBD_CMD_UART_INIT_FAIL;
	
	read_cnt = katech_obd_write_cmd_resp("RC", NULL, 0, ret_buff, &error_code);
    
    //printf("%s() :: ret_buff is [%s]\r\n", __func__, ret_buff);
    
    
    
    if ( error_code == OBD_ERROR_CODE__SUCCESS)
    {
        //printf("%s :: SUCCESS :: error code [%d] / read_cnt [%d]\r\n",  __func__, error_code, read_cnt);
        pAtcmd = ret_buff;
        
        tr = strtok_r(pAtcmd, token, &temp_bp);
		if(tr == NULL)
			return OBD_RET_FAIL;
        //printf("tr -1 [%s]\r\n", tr);
        
        tr = strtok_r(NULL, token, &temp_bp);
		if(tr == NULL)
				return OBD_RET_FAIL;
        //printf("tr -2 [%s]\r\n", tr);
        *data1_cnt = atoi(tr);
        
		tr = strtok_r(NULL, token, &temp_bp);
		if(tr == NULL)
				return OBD_RET_FAIL;
        //printf("tr -3 [%s]\r\n", tr);
        
        tr = strtok_r(NULL, token, &temp_bp);
		if(tr == NULL)
				return OBD_RET_FAIL;
        //printf("tr -4 [%s]\r\n", tr);
        *data2_cnt = atoi(tr);
        
        printf("obd data cnt [%d],[%d]\r\n", *data1_cnt, *data2_cnt);
        return OBD_RET_SUCCESS;
    }
    else
    {
        //printf("%s :: FAIL :: error code [%d] / read_cnt [%d]\r\n",  __func__, error_code, read_cnt);
        return OBD_RET_FAIL;
    }
}

// *********************************************************
// 1. ������ ��û
// *********************************************************
int get_obd_data_1(unsigned char obd_buff[200])
{
	unsigned char ret_buff[512] = {0,};
	int error_code = 0;
	int read_cnt = 0;
	
	
	// chk & open
	if ( chk_obd_port() != OBD_RET_SUCCESS)
		return OBD_CMD_UART_INIT_FAIL;
	
	read_cnt = katech_obd_write_cmd_resp("R1", NULL, 0, ret_buff, &error_code);
    
    //printf("%s() :: ret_buff is [%s]\r\n", __func__, ret_buff);
    
    if ( error_code == OBD_ERROR_CODE__SUCCESS)
    {
        //printf("%s :: SUCCESS :: error code [%d] / read_cnt [%d]\r\n", __func__, error_code, read_cnt);
        memcpy(obd_buff, ret_buff, 200);
        return OBD_RET_SUCCESS;
    }
    else
    {
        //printf("%s :: FAIL :: error code [%d] / read_cnt [%d]\r\n",__func__,  error_code, read_cnt);
        return OBD_RET_FAIL;
    }
}

int delete_obd_data_1()
{
	unsigned char ret_buff[512] = {0,};
	int error_code = 0;
	int read_cnt = 0;
	
	
	// chk & open
	if ( chk_obd_port() != OBD_RET_SUCCESS)
		return OBD_CMD_UART_INIT_FAIL;
	
	read_cnt = katech_obd_write_cmd_resp("D1", NULL, 0, ret_buff, &error_code);
    
    //printf("%s() :: ret_buff is [%s]\r\n", __func__, ret_buff);
    
    if (( error_code == OBD_ERROR_CODE__SUCCESS ) ||  ( error_code == OBD_ERROR_CODE__NO_DATA_RET ) )
    {
        //printf("%s :: SUCCESS :: error code [%d] / read_cnt [%d]\r\n", __func__, error_code, read_cnt);
        return OBD_RET_SUCCESS;
    }
    else
    {
        printf("%s :: FAIL :: error code [%d] / read_cnt [%d]\r\n",__func__,  error_code, read_cnt);
        return OBD_RET_FAIL;
    }
}

int get_seco_obd_cmd_ta1(unsigned char obd_buff[100])
{
	unsigned char ret_buff[512] = {0,};
	int error_code = 0;
	int read_cnt = 0;
	int i;
	
	// chk & open
	if ( chk_obd_port() != OBD_RET_SUCCESS)
		return OBD_CMD_UART_INIT_FAIL;
	
	read_cnt = katech_obd_write_cmd_resp("R2", NULL, 0, ret_buff, &error_code);
    
    //printf("%s() :: ret_buff is [%s]\r\n", __func__, ret_buff);
    
    /*
    for(i = 0 ; i < 100 ; i ++)
    {
        printf("obd read [%d] ==> [0x%02x]\r\n", i, ret_buff[i]);
    }
    */
    
    if ( error_code == OBD_ERROR_CODE__SUCCESS)
    {
      //  printf("%s :: SUCCESS :: error code [%d] / read_cnt [%d]\r\n", __func__, error_code, read_cnt);
        memcpy(obd_buff, ret_buff, 100);
        return OBD_RET_SUCCESS;
    }
    else
    {
       // printf("%s :: FAIL :: error code [%d] / read_cnt [%d]\r\n",__func__,  error_code, read_cnt);
        return OBD_RET_FAIL;
    }
}

int delete_obd_data_2()
{
	unsigned char ret_buff[512] = {0,};
	int error_code = 0;
	int read_cnt = 0;
	
	
	// chk & open
	if ( chk_obd_port() != OBD_RET_SUCCESS)
		return OBD_CMD_UART_INIT_FAIL;
	
	read_cnt = katech_obd_write_cmd_resp("D2", NULL, 0, ret_buff, &error_code);
    
   // printf("%s() :: ret_buff is [%s]\r\n", __func__, ret_buff);
    
    if (( error_code == OBD_ERROR_CODE__SUCCESS ) ||  ( error_code == OBD_ERROR_CODE__NO_DATA_RET ) )
    {
        //printf("%s :: SUCCESS :: error code [%d] / read_cnt [%d]\r\n", __func__, error_code, read_cnt);
        return OBD_RET_SUCCESS;
    }
    else
    {
        printf("%s :: FAIL :: error code [%d] / read_cnt [%d]\r\n",__func__,  error_code, read_cnt);
        return OBD_RET_FAIL;
    }
}
