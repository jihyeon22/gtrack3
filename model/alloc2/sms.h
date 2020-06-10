#ifndef __MODEL_SMS_H__
#define __MODEL_SMS_H__

#define SMS_CMD_ALLOC2_PWD "flemgkwkallo"

typedef enum
{
	eSMS_CMD_SET__SERVER_INFO, // &11 // 중계서버의 IP:port	111.112.113.114:1234
	eSMS_CMD_SET__MDM_GPS_REPORT_KEYON_INTERVAL, // &12 // 위치정보 보고 주기 설정(IG1 On)	1 ~ 7200 secs
    eSMS_CMD_SET__MDM_GPS_REPORT_KEYOFF_INTERVAL, // &13 // 위치정보 보고 주기 설정 (IG1 Off)	1 ~ 7200 secs
    eSMS_CMD_SET__MDM_GPS_TOTAL_DISTANCE, // &17 // GPS기반 누적거리 설정	meter단위
    eSMS_CMD_SET__CAR_LOW_BATT_VOLT, // &18 // 저전압 경고 설정	0.1단위 예)12.0 -> 120
    eSMS_CMD_SET__MDM_RESET_TIME, // &19 // 주기적 리셋 타임 설정	분단위 예)03:20 -> 320
    eSMS_CMD_SET__BCM_HORN_CNT, // &20 // 경적횟수 설정	1 ~ 10
    eSMS_CMD_SET__BCM_LIGHT_CNT, // &21 // 비상등 횟수 설정	1 ~ 10
    eSMS_CMD_SET__OBD_OVERSPEED_KMH, // &22 // 과속기준 속도 설정	Km/h 단위
    eSMS_CMD_SET__OBD_OVERSPEED_TIME, // &23 // 과속기준 시간 설정	secs
    eSMS_CMD_SET__CAR_ENGINE_OFF, // &30 // 시동 차단 설정 명령	0 : 시동차단 설정 1 : 시동차단 해제
    eSMS_CMD_SET__MDM_STAT_REQ, // &40 // 단말기 상태 요청	0
    eSMS_CMD_SET__BCM_KNOCK_PASS_REQ, // &45 // 마스터 번호 등록 4자리
    eSMS_CMD_SET__BCM_KNOCK_ID_REQ, // &46 // ID 등록 4자리	0
    eSMS_CMD_SET__BCM_RESERVATION_INFO_REQ, // &47 // 예약정보 요청	0
    eSMS_CMD_SET__MDM_REINIT, // &50 // 단말기 초기화 명령	0
    eSMS_CMD_SET__MDM_RESET, // &60 // 단말기 재부팅 명령	0
    eSMS_CMD_SET__OBD_REPORT_KEY_ON_INTERVAL, // &71 // OBD정보 보고 주기 설정 (IG1 On)	1 ~ 7200 secs
    eSMS_CMD_SET__OBD_REPORT_KEY_OFF_INTERVAL, // &72 // OBD정보 보고 주기 설정 (IG1 Off)	1 ~ 7200 secs
    eSMS_CMD_SET__OBD_TOTAL_DISTANCE_SET, // &73 // OBD기반 누적거리 설정	meter단위
    eSMS_CMD_SET__BCM_DOOR_LOCK, // &81 // Door Lock	0
    eSMS_CMD_SET__BCM_DOOR_UNLOCK, // &82 // Door Unlock	0
    eSMS_CMD_SET__BCM_HORN, // &83 // Horn	0
    eSMS_CMD_SET__BCM_LIGHT, // &84 // Emergency Light	0
    eSMS_CMD_SET__FIRM_UPDATE_MDM, // &91 // 단말 펌웨어	버전 
    eSMS_CMD_SET__FIRM_UPDATE_OBD, // &92 // OBD 펌웨어	버전
    eSMS_CMD_SET__OBD_DEV_INIT, // &93 // OBD 초기화	Car code
    eSMS_CMD_SET__FIRM_UPDATE_ALLKEY_BCM, // &94 // All key 펌웨어	버전
    eSMS_CMD_SET__FIRM_UPDATE_BCM, // &95 // BCM 펌웨어	버전
    eSMS_CMD_SET__FIRM_UPDATE_DTG, // &96 // DTG 펌웨어	버전
    eSMS_CMD_SET__FIRM_UPDATE_KNOCK, // &97 // 노크센서 펌웨어	버전
	MAX_SMS_CMD,
}SMS_CMD_INDEX;

typedef struct
{
	int index;
    const char * cmd;
    int (*proc_func)(int argc, char* argv[], const char* phonenum);
}SMS_CMD_FUNC_T;



int parse_model_sms(const char *time, const char *phonenum, const char *sms);

#endif

