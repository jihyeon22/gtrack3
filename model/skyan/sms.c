#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <board/power.h>
#include <config.h>
#include <logd_rpc.h>

#include "skyan_tools.h"
#include "skyan_senario.h"
#include "packet.h"

#include "sms.h"

static int _sms_cmd_proc_get__stat(char* arg, int size, const char* phonenum)
{
    int chk_size = 1;
    int data_offset = 0;

    char get_val__stat = 0;

    if ( chk_size > size )
    {
        printf("%s() err => [%d] / [%d]\r\n", __func__, chk_size, size);
        return -1;
    }

    memcpy(&get_val__stat, arg + data_offset, sizeof(get_val__stat));
    data_offset += sizeof(get_val__stat);
    printf("%s() => get_val__stat is [%c]\r\n", __func__, get_val__stat);

    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__SMS_RESP__STAT);

    return 0;

}

static int _sms_cmd_proc_set__nostart_mode(char* arg, int size, const char* phonenum)
{
    int chk_size = 1;
    int data_offset = 0;

    char get_val__stat = 0;

    if ( chk_size > size )
    {
        printf("%s() err => [%d] / [%d]\r\n", __func__, chk_size, size);
        return -1;
    }

    memcpy(&get_val__stat, arg + data_offset, sizeof(get_val__stat));
    data_offset += sizeof(get_val__stat);
    printf("%s() => get_val__stat is [%c]\r\n", __func__, get_val__stat);

    if ( get_val__stat == '1' )
    {
        skyan_tools__set_nostart_flag(SKYAN_TOOLS__NOSTART_MODE);
    }
    else if ( get_val__stat == '0')
    {
        skyan_tools__set_nostart_flag(SKYAN_TOOLS__NORMAL_MODE);
    }

    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__SMS_RESP__NOSTART);

    return 0;

}


static int _sms_cmd_proc_set__dev_setting(char* arg, int size, const char* phonenum)
{
    int data_offset = 0;
    int chk_size = 0;

    unsigned short get_val__keyon_interval = 0;
    unsigned short get_val__keyoff_interval = 0;
    unsigned char  get_val__dev_mode = 0;
    unsigned short get_val__low_batt = 0;

    chk_size += sizeof(get_val__keyon_interval);
    chk_size += sizeof(get_val__keyoff_interval);
    chk_size += sizeof(get_val__dev_mode);
    chk_size += sizeof(get_val__low_batt);

    if ( chk_size > size )
    {
        printf("%s() err => [%d] / [%d]\r\n", __func__, chk_size, size);
        return -1;
    }

    memcpy(&get_val__keyon_interval, arg + data_offset, sizeof(get_val__keyon_interval));
    data_offset += sizeof(get_val__keyon_interval);
    printf("%s() => get_val__keyon_interval is [%d]\r\n", __func__, get_val__keyon_interval);

    memcpy(&get_val__keyoff_interval, arg + data_offset, sizeof(get_val__keyoff_interval));
    data_offset += sizeof(get_val__keyon_interval);
    printf("%s() => get_val__keyoff_interval is [%d]\r\n", __func__, get_val__keyoff_interval);

    memcpy(&get_val__dev_mode, arg + data_offset, sizeof(get_val__dev_mode));
    data_offset += sizeof(get_val__dev_mode);
    printf("%s() => get_val__dev_mode is [%d]\r\n", __func__, get_val__dev_mode);

    memcpy(&get_val__low_batt, arg + data_offset, sizeof(get_val__low_batt));
    data_offset += sizeof(get_val__low_batt);
    printf("%s() => get_val__low_batt is [%d]\r\n", __func__, get_val__low_batt);


    skyan_tools__set_low_batt_level(get_val__low_batt);
    set_user_cfg_keyon_interval(get_val__keyon_interval);
    set_user_cfg_keyoff_interval(get_val__keyoff_interval);

    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__SMS_RESP__SETTING);

    return 0;

}

static int _sms_cmd_proc_set__dev_reset(char* arg, int size, const char* phonenum)
{
    int chk_size = 1;
    int data_offset = 0;

    char get_val__stat = 0;

    if ( chk_size > size )
    {
        printf("%s() err => [%d] / [%d]\r\n", __func__, chk_size, size);
        return -1;
    }

    memcpy(&get_val__stat, arg + data_offset, sizeof(get_val__stat));
    data_offset += sizeof(get_val__stat);
    printf("%s() => get_val__stat is [%c]\r\n", __func__, get_val__stat);

    


    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__SMS_RESP__RESET);

    // TODO: device reset?
    skyan_senario__poweroff();

    return 0;
}

static int _sms_cmd_proc_set__server_setting(char* arg, int size, const char* phonenum)
{
    int data_offset = 0;
    int chk_size = 0;

    unsigned char get_val__ip1 = 0;
    unsigned char get_val__ip2 = 0;
    unsigned char get_val__ip3 = 0;
    unsigned char get_val__ip4 = 0;

    unsigned short get_val__port = 0;

    char ip_addr[128] = {0,};

    chk_size += sizeof(get_val__ip1);
    chk_size += sizeof(get_val__ip2);
    chk_size += sizeof(get_val__ip3);
    chk_size += sizeof(get_val__ip4);
    chk_size += sizeof(get_val__port);

    if ( chk_size > size )
    {
        printf("%s() err => [%d] / [%d]\r\n", __func__, chk_size, size);
        return -1;
    }


    memcpy(&get_val__ip1, arg + data_offset, sizeof(get_val__ip1));
    data_offset += sizeof(get_val__ip1);
    printf("%s() => get_val__ip1 is [%d]\r\n", __func__, get_val__ip1);

    memcpy(&get_val__ip2, arg + data_offset, sizeof(get_val__ip2));
    data_offset += sizeof(get_val__ip2);
    printf("%s() => get_val__ip2 is [%d]\r\n", __func__, get_val__ip2);

    memcpy(&get_val__ip3, arg + data_offset, sizeof(get_val__ip3));
    data_offset += sizeof(get_val__ip3);
    printf("%s() => get_val__ip2 is [%d]\r\n", __func__, get_val__ip3);

    memcpy(&get_val__ip4, arg + data_offset, sizeof(get_val__ip4));
    data_offset += sizeof(get_val__ip4);
    printf("%s() => get_val__ip4 is [%d]\r\n", __func__, get_val__ip4);

    memcpy(&get_val__port, arg + data_offset, sizeof(get_val__port));
    data_offset += sizeof(get_val__port);
    printf("%s() => get_val__port is [%d]\r\n", __func__, get_val__port);

    sprintf(ip_addr, "%d.%d.%d.%d", get_val__ip1, get_val__ip2, get_val__ip3, get_val__ip4);

    printf("%s() => ipaddr [%s] / port [%d]\r\n", __func__, ip_addr, get_val__port);

    set_user_cfg_report_ip(ip_addr);
    set_user_cfg_report_port(get_val__port);

    send_sky_autonet_evt_pkt(SKY_AUTONET_EVT__SMS_RESP__SERVER_INFO);


    

    return 0;
}



static SMS_CMD_FUNC_T sms_cmd_func[] =
{
    {eSMS_CMD_GET__STAT, "Gn", _sms_cmd_proc_get__stat} , // Gn // 상태요청 // data '1'
    {eSMS_CMD_SET__NOSTART_MODE, "Gc", _sms_cmd_proc_set__nostart_mode} , // Gc // 시동차단요청 // 시동차단 '1', 차단해제 '0'
    {eSMS_CMD_SET__DEV_SETTING, "Gt", _sms_cmd_proc_set__dev_setting} , // Gt // 설정요청 // 추가정보
    {eSMS_CMD_SET__DEV_RESET, "Gr", _sms_cmd_proc_set__dev_reset} , // Gr // 리셋요청 // 단말리셋 - 'T', GPS리셋 - 'G' , 모뎀리셋 - 'M'
    {eSMS_CMD_SET__SERVER_SETTING, "Gs", _sms_cmd_proc_set__server_setting} , // Gs // 서버설정 // 추가정보..
};



int parse_model_sms(const char *time, const char *phonenum, const char *sms)
{
    int i,j=0;
    unsigned char decode_b64_str[512] = {0,};
    int decode_size = 0;

    printf("base64 [%s] => [%s]\r\n", sms, decode_b64_str);
    if ( ( decode_size = mds_api_b64_decode(sms, strlen(sms), decode_b64_str, sizeof(decode_b64_str))) <= 0 )
    {
        printf("decode fail...\r\n");
        return -1;
    }
    
    /*
    printf("base64 [%s] => [%s]\r\n", sms, decode_b64_str);
    */
    printf("--- decode success [%d] ------------------------------\r\n", decode_size);
    mds_api_debug_hexdump_buff(decode_b64_str,decode_size);
    printf("------------------------------------------------------\r\n");
    
    for(j = 0; j < MAX_SMS_CMD; j++)
    {
        if  (!( strncasecmp ( decode_b64_str, sms_cmd_func[j].cmd, strlen(sms_cmd_func[j].cmd) ) ))
        {
            if ( sms_cmd_func[j].proc_func != NULL )
            {
                // argc는 항상 2개다.
                printf("-----------------------------\r\n");
                int proc_ret = sms_cmd_func[j].proc_func(decode_b64_str + strlen(sms_cmd_func[j].cmd), decode_size -  strlen(sms_cmd_func[j].cmd), phonenum);

                // 그리고 argc2가 두개씩이니...
            }
        }
    }

	return 0;
}
