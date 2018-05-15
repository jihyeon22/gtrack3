#ifndef __SKYAN_PACKET_H__
#define __SKYAN_PACKET_H__

#define SKY_AUTONET_PKT__PREFIX   'S'
#define SKY_AUTONET_PKT__SUFFIX   '\n'

// PKT MSG TYPE
#define SKY_AUTONET_PKT__MSG_TYPE__MDT   'B' // 기본관제
#define SKY_AUTONET_PKT__MSG_TYPE__DTG   'D' // DTG 관제

#define SKY_AUTONET_PKT__PKT_ENDFLAG__END   '1' // 패킷종료
#define SKY_AUTONET_PKT__PKT_ENDFLAG__ING   '0' // 패킷종료않함

#define SKY_AUTONET_PKT__DEV_STAT_BIT__KEYON_STAT_1  0 // 시동차단       // 1- 시동차단
#define SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_LOW      1 // 배터리방전상태 // 1- 밧데리방전
#define SKY_AUTONET_PKT__DEV_STAT_BIT__BATT_DEVIDE   2 // 배터리탈거상태 // 1- 밧데리탈거
#define SKY_AUTONET_PKT__DEV_STAT_BIT__GPS_CONN      3 // GPS 단선상태    // 1- 단선
#define SKY_AUTONET_PKT__DEV_STAT_BIT__KEYON_STAT_2  4 // 시동차단실행상태 // 1- 시동차단실행
#define SKY_AUTONET_PKT__DEV_STAT_BIT__WORKING       5 // 비업무시간여부 // 1- 비업무시간여부

// ------------------------------------------------------------------
// 이벤트코드
// ------------------------------------------------------------------

// ------- sms 명령보고 --------------------------
#define SKY_AUTONET_EVT__SMS_RESP__STAT           'n'     // 상태요청보고 : 상태요청 SMS에 대한 보고
#define SKY_AUTONET_EVT__SMS_RESP__NOSTART        'c'     // 시동차단설정보고 : 시동차단설정 sms 에 대한 보고
#define SKY_AUTONET_EVT__SMS_RESP__SETTING        't'     // 설정요청보고 : 설정요청 sms에 대한 보고
#define SKY_AUTONET_EVT__SMS_RESP__RESET          'r'     // 리셋요청보고 : 리셋요청 SMS에 대한보고
#define SKY_AUTONET_EVT__SMS_RESP__SERVER_INFO    's'     // 서버설정보고 : 서버설정 sms에 대한 보고
// ------- 주기보고 ------------------------------
#define SKY_AUTONET_EVT__KEYON                    'N' // 키 on 보고 // 키 on 주행 시작보고
#define SKY_AUTONET_EVT__KEYON_REPORT             '1' // 키 on 주기보고 // 키 on 주기보고
#define SKY_AUTONET_EVT__KEYOFF                   'F' // 키 on 보고 // 키 on 주행 시작보고
#define SKY_AUTONET_EVT__KEYOFF_REPORT            '0' // 키 on 주기보고 // 키 on 주기보고
#define SKY_AUTONET_EVT__LOW_BATT                 'L' // 밧데리 방전이벤트 // 차량전압이 방전전압 미만일경우
#define SKY_AUTONET_EVT__REMOVE_BATT              'B' // 밧데리 탈거 이벤트 // 차량전압이 최소전압 이하일 경우 
#define SKY_AUTONET_EVT__GPS_ANT_DISCONN          'G' // GPS 단선이벤트 // GPS 이상 시그널 감지
#define SKY_AUTONET_EVT__NOSTART                  'C' // 시동차단실행 이벤트 // 시동차단 설정후 키 입력시
#define SKY_AUTONET_EVT__PWR_RESET                'P' // 전원리셋보고이벤트 // 전원리셋 보고
// #define SKY_AUTONET_EVT__ 급감속
// #define SKY_AUTONET_EVT__ 급가속
// #define SKY_AUTONET_EVT__ 급출발
// #define SKY_AUTONET_EVT__ 급정지
// #define SKY_AUTONET_EVT__ 공회전
// #define SKY_AUTONET_EVT__ 과속
// ------- 주기보고 ------------------------------
#define SKY_AUTONET_EVT__GEOFENCE__ENTRY          'i' // 경유지 진입 // 경유지 진입 이벤트
#define SKY_AUTONET_EVT__GEOFENCE__EXIT           'o' // 경유지 이탈 // 경유지 이탈 이벤트
#define SKY_AUTONET_EVT__GEOFENCE__GET_FROM_SVR   'v' // 경유지 목록수신 // 경유지 목록 수신


// --------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------
// HEADER : COMMON HEADER
// -------------------------------------------------------
typedef struct {
    char pkt_stx[1+1];                                // 1 // service id 'S'
    unsigned int phonenum;                      // 4 // phone num : 012-1234-5678 => 1212345678
    // unsigned short pkt_len;                     // 2 // 메시지구분에서 ext 까지의 바이트
    char pkt_msg[1+1];                               // 1 //메시지구분 // 'B' 기본관제 , 'D' DTG 관제
    // char pkt_end_flag;                          // 1 // 패킷종료플랙 // '1' 패킷종료, '0' 패킷종료않함
    char pkt_evt_code[1+1];                          // 1 // 응답코드 : 응답하고자하는 단말패킷메시지의 메시지코드
}__attribute__((packed))SKY_AUTONET_PKT__HEAD_T; // 10 BYTE


// -------------------------------------------------------
// SETTING VAL : COMMON VAL
// -------------------------------------------------------
typedef struct {
    unsigned short set_info__keyon_interval;            // 2 // key on 보고주기 // sec
    unsigned short set_info__keyoff_interval;           // 2 // key off 보고주기 // sec
    unsigned char  set_info__gps_on;                    // 1 // gps on // 1 - 항상 on, 0 - 키연동
    unsigned short set_info__low_batt;                  // 2 // 방전전압레벨 // 11.5 -> 115
    unsigned char  set_info__over_speed;                // 1 // 과속기준 // km/h
    unsigned short set_info__idle;                      // 2 // 공회전기준 // sec
    unsigned short set_info__over_rpm;                  // 2 // 고 rpm 기준 // rpm
    unsigned char  set_info__gps_act;                   // 1 // 위경도 활성화 // '1' - 활성화, '0' - 비활성화
    //unsigned char  set_info__reserved[3];               // 3 // reserved
}__attribute__((packed))SKY_AUTONET_PKT__SETTING_INFO_T; // 16 BYTE


// -------------------------------------------------------
// SEND PKT BODY : BODY
// -------------------------------------------------------
typedef struct {
    // 단말상태 --------------------------------------------------------
    unsigned int    report_time;             // 4 // 보고시간 : utc sec 
    unsigned int    dev_stat__keyon_time;    // 4 // 최종 키 on 시간 (초단위)
    unsigned int    dev_stat__keyoff_time;   // 4 // 최종 키 off 시간  (초단위)
    unsigned char   dev_stat__keystat;       // 1 // 키상태 // 0 off , 1 on
    unsigned char   dev_stat__stat;          // 1 // 단말상태 필드
    unsigned short  dev_stat__main_volt;     // 2 // 차량전압 // 135 -> 13.5
    unsigned char   dev_stat__firm_ver[10+1];  // 10 // 펌웨어버젼 // 길이 5 LBS, 길이 10 OBD
    // GPS  --------------------------------------------------------
    unsigned char  gps_dev__stat;           // 1 // 현재 GPS상태 // 0 단선, 1 무효, 2 유효(2D), 3 유효(3D)
    unsigned int   gps_dev__last_fix_time;  // 4 // 최종 유효 GPS시간 // UTCS
    unsigned int   gps_dev__lat;            // 4 // gps 위도 : 37.123456 => 37123456 // 최종유효좌표
    unsigned int   gps_dev__lon;            // 4 // gps 경도 : 127.123456 => 127123456 // 최종유효좌표
    unsigned short gps_dev__dir;            // 2 // gps 방향 // 0~360
    unsigned short gps_dev__speed;          // 2 // gps 속도 : 단위 km/h
    unsigned int   gps_dev__total_dist;     // 4 // 장착누적거리 : 단위 m
    unsigned int   gps_dev__geofence_id;    // 4 // default : 0
    // 설정정보  -----------------------------------------------------------
    SKY_AUTONET_PKT__SETTING_INFO_T set_info; // 16 byte
}__attribute__((packed))SKY_AUTONET_PKT__GPS_BODY_T;  // 67byte

// -------------------------------------------------------
// RECV PKT BODY : BODY
// -------------------------------------------------------
typedef struct {
    unsigned int    report_time;             // 4 // 보고시간 : utc sec 
    unsigned int    dev_stat__keyon_time;    // 4 // 최종 키 on 시간 (초단위)
    unsigned int    dev_stat__keyoff_time;   // 4 // 최종 키 off 시간  (초단위)
    unsigned char   dev_stat__keystat;       // 1 // 키상태 // 0 off , 1 on
    unsigned char   dev_stat__stat;          // 1 // 단말상태 필드
    unsigned short  dev_stat__main_volt;     // 2 // 차량전압 // 135 -> 13.5
    unsigned char   dev_stat__firm_ver[10+1];  // 10 // 펌웨어버젼 // 길이 5 LBS, 길이 10 OBD
    SKY_AUTONET_PKT__SETTING_INFO_T set_info; // 16 byte
}__attribute__((packed))SKY_AUTONET_PKT__RESP_BODY_T;  // 67byte


// -------------------------------------------------------
// GEOFENCE PKT BODY : BODY
// -------------------------------------------------------
typedef struct {
    int   fence_point_id; // fence id
	int   fence_path_id; // fence id
    float gps_lat;
    float gps_lon;
	int   visit_order;
    int   radius_m;    // distance m
    int   set_info;    // 1 - set / 0 - clear
}__attribute__((packed))SKY_AUTONET__GEOFENCE_INFO_T; 

// --------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------
// RECV PKT BODY : STRUCTURE
// -------------------------------------------------------
typedef struct {
    SKY_AUTONET_PKT__HEAD_T      header;
    SKY_AUTONET_PKT__GPS_BODY_T  body;
}__attribute__((packed))SKY_AUTONET_PKT__GPS_PKT_T;  // 67byte


// -------------------------------------------------------
// SEND PKT BODY : STRUCTURE
// -------------------------------------------------------
#define MAX_GEOFENCE_ONE_PARSE_CNT      100
typedef struct {
    int status;
    SKY_AUTONET_PKT__HEAD_T      header;
    SKY_AUTONET_PKT__RESP_BODY_T body; // 16 byte
    unsigned int    fence_cnt;
    SKY_AUTONET__GEOFENCE_INFO_T fence[MAX_GEOFENCE_ONE_PARSE_CNT];
}__attribute__((packed))SKY_AUTONET_PKT__RESP_PKT_T;  // 67byte


// --------------------------------------------------------------------------------------------------------------

typedef struct {
    int   fence_id; // fence id
    int   evt_code; // event code
}__attribute__((packed))SKY_AUTONET__PKT_ARG_T; 

// --------------------------------------------------------------------------------------------------------------

int create_sky_autonet_report_pkt(SKY_AUTONET__PKT_ARG_T* pkt_arg, unsigned char **pbuf, unsigned short *packet_len);
int send_sky_autonet_report_pkt();
int send_sky_autonet_evt_pkt(int evt);


#endif // __SKYAN_PACKET_H__

