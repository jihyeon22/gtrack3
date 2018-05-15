#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

int parse_model_sms(const char *time, const char *phonenum, const char *sms);


typedef enum
{
    eSMS_CMD_GET__STAT, // Gn // 상태요청 // data '1'
    eSMS_CMD_SET__NOSTART_MODE, // Gc // 시동차단요청 // 시동차단 '1', 차단해제 '0'
    eSMS_CMD_SET__DEV_SETTING, // Gt // 설정요청 // 추가정보
    eSMS_CMD_SET__DEV_RESET, // Gr // 리셋요청 // 단말리셋 - 'T', GPS리셋 - 'G' , 모뎀리셋 - 'M'
    eSMS_CMD_SET__SERVER_SETTING, // Gs // 서버설정 // 추가정보..
	MAX_SMS_CMD,
}SMS_CMD_INDEX;


#endif

