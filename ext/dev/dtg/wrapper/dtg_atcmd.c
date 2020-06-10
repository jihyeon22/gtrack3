#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <at/at_util.h>

char g_dtg_atcmd_phonenum[32] = {0,};
char g_dtg_atcmd_imei[64] = {0,};


char* atcmd_get_phonenum()
{
    char tmp_dial[32] = {0,};

    if ( strlen(g_dtg_atcmd_phonenum) > 0 )
        return g_dtg_atcmd_phonenum;
    
    if ( at_get_phonenum(tmp_dial, 32) != AT_RET_SUCCESS )
        return NULL;

    strcpy(g_dtg_atcmd_phonenum, tmp_dial);

    return g_dtg_atcmd_phonenum;
}

char* atcmd_get_imei()
{
    char tmp_imei[64] = {0,};

    if ( strlen(g_dtg_atcmd_imei) > 0 )
        return g_dtg_atcmd_imei;
    
    if ( at_get_imei(tmp_imei, 64) != AT_RET_SUCCESS )
        return NULL;
    
    strcpy(g_dtg_atcmd_imei, tmp_imei);

    return g_dtg_atcmd_imei;

}

int atcmd_get_rssi()
{
    int tmp_rssi = 0;
    if ( at_get_rssi(&tmp_rssi) != AT_RET_SUCCESS )
        tmp_rssi = -1;

    return tmp_rssi;
}

int atcmd_send_sms(const char *message, const char *sender, const char *receiver)
{
	printf("message : %s, tel : %s\n", message, receiver);
    return (at_send_sms(receiver, message));
}
