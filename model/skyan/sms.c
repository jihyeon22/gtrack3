#include <stdlib.h>
#include <string.h>

#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <board/power.h>
#include <config.h>
#include <logd_rpc.h>
#include "sms.h"

int parse_model_sms(const char *time, const char *phonenum, const char *sms)
{
    #if 0
    for(j = 0; j < MAX_SMS_CMD; j++)
		{
			if  (!( strncasecmp ( model_argv[i], sms_cmd_func[j].cmd, strlen(sms_cmd_func[j].cmd) ) ))
			{
				// found command
				sms_pkt_result_code = get_sms_pkt_cmd_code(atoi(sms_cmd_func[i].cmd+1));
				printf(" >> sms cmd found target [%s] / code [%d]\r\n",model_argv[i],sms_pkt_result_code);
				
				if ( sms_cmd_func[j].proc_func != NULL )
				{
					// argc는 항상 2개다.
					printf("-----------------------------\r\n");
					int proc_ret = sms_cmd_func[j].proc_func(2, &model_argv[i], phonenum);

					if ( proc_ret == SMS_CMD_ALLOC2_RET_FAIL )
					{
						printf(" >> sms cmd proc result fail [%s] \r\n",model_argv[i]);
						sms_proc_result = SMS_CMD_ALLOC2_RET_FAIL;
					}
					else if ( proc_ret == SMS_CMD_ALLOC2_RET_REQ_RESET ) 
					{
						need_to_reset_flag = 1;
					}
					else if ( proc_ret == SMS_CMD_ALLOC2_RET_REQ_INIT )
					{
						need_to_init_flag = 1;
					}
					else
					{
						printf(" >> sms cmd proc result success [%s] \r\n",model_argv[i]);
					}
					// 그리고 argc2가 두개씩이니...
				}
				i++;
			}
		}
#endif
	return 0;
}


#if 0
static SMS_CMD_FUNC_T sms_cmd_func[] =
{
    {eSMS_CMD_GET__STAT, "Gn", _sms_cmd_proc_get__stat} , // Gn // 상태요청 // data '1'
    {eSMS_CMD_SET__NOSTART_MODE, "Gc", _sms_cmd_proc_set__nostart_mode} , // Gc // 시동차단요청 // 시동차단 '1', 차단해제 '0'
    {eSMS_CMD_SET__DEV_SETTING, "Gt", _sms_cmd_proc_set__dev_setting} , // Gt // 설정요청 // 추가정보
    {eSMS_CMD_SET__DEV_RESET, "Gr", _sms_cmd_proc_set__dev_reset} , // Gr // 리셋요청 // 단말리셋 - 'T', GPS리셋 - 'G' , 모뎀리셋 - 'M'
    {eSMS_CMD_SET__SERVER_SETTING, "Gs", _sms_cmd_proc_set__server_setting} , // Gs // 서버설정 // 추가정보..
}

#endif