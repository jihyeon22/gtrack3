#ifndef __ALLOC2_PKT_H__
#define __ALLOC2_PKT_H__

#define PKT_VER_V_1_6

// PKT_VER_ALWAYS_RF_OFF
//  - 항상 rf off 하고있다가, 통신시에만 rf on 한다.
// #define PKT_VER_ALWAYS_RF_OFF

// PKT_VER_POWERSAVE_MODE
//  - 설정 전압에 따라서 event 를 보내고 말고를 설정한다.
#define PKT_VER_POWERSAVE_MODE

// 두개의 mode 를 한번에 세팅할수없다.
#if defined (PKT_VER_ALWAYS_RF_OFF) && defined (PKT_VER_POWERSAVE_MODE)
#error " only on power save featre ..."
#endif

// --------------------------------------------------------------------------
// pkt header idx
typedef enum {
    e_mdm_setting_val = 0x01,   // 0x01 : 단말 기본 설정 정보
    e_mdm_stat_evt = 0x02,      // 0x02 : 단말 상태 정보 (이벤트)
    e_mdm_stat_evt_fifo = 0x92,      // 0x02 : 단말 상태 정보 (이벤트)
    e_mdm_gps_info = 0x03,       // 0x03 : GPS 정보
    e_mdm_gps_info_fifo = 0x93,       // 0x03 : GPS 정보
    e_obd_dev_info = 0x11,   // 0x11 : OBD 기본 설정 정보
    e_obd_stat = 0x12,      // 0x12 : OBD 상태 정보 (이벤트)
    e_obd_data = 0x13,          // 0x13 : OBD 수집 정보
    e_obd_chk_code = 0x14,      // 0x14 : OBD 차량 진단코드
    e_dtg_setting_val = 0x21,   // 0x21 : DTG 기본 정보
    e_dtg_data = 0x22,          // 0x22 : DTG 수집 정보
    e_mdm_geofence_setting_val = 0x31, // 0x31 : zone 설정 정보
    e_mdm_geofence_evt = 0x32,  // 0x32 : zone 입출 정보
    e_bcm_stat_evt = 0x41,      // 0x41 : All key 상태 정보 (이벤트)
    e_bcm_statting_val = 0x42,  // 0x42 : All key 설정 정보
    e_bcm_mastercard_regi = 0x45, // 0x45 : 마스터카드 등록
    e_bcm_reserv_val = 0x47,    // 0x47 : 예약정보
    e_bcm_knocksensor_setting_val = 0x51, // 0x51 : 노크센서 설정 정보
    e_firm_info = 0x71, // 0x71 : 펌웨어 정보
    e_firm_update = 0x72, // 0x72 : 펌웨어 업데이트
    e_firm_complete = 0x79, // 0x79 : 펌웨어 업데이트 완료
    e_sms_recv_info = 0xf0, // 0xF0 : SMS 수신 정보
} e_ALLOC2_MSG_TYPE;


typedef struct {
    unsigned int 	pkt_total_len;  // (b-4)  Header를 포함한 전체 Message size
    unsigned int 	mdm_phonenum;   // (b-4) 단말기 sim 번호 (01211112222)
	unsigned int 	time_stamp;		// (b-4) 메시지 생성 시각
	unsigned char	msg_type;	// (b-1) e_ALLOC2_MSG_TYPE /  Msssage type
	unsigned char	reserved;  // (b-1) reserved
}__attribute__((packed))ALLOC_PKT_COMMON_HEADER;

// ===============================================================================================
// e_mdm_setting_val = 0x01
// ===============================================================================================

// 3.	단말 기본 설정 정보 ( e_mdm_setting_val )
// 3.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    char   mdm_sn[16];   // (a-16) 단말 시리얼
    unsigned char   reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__MDM_SETTING_VAL;

// 3.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    char   proxy_server_ip[16];    // (a-16) 중계서버 IP 
    unsigned short  proxy_server_port;      // (b-2) 중계서버 port
    unsigned short  key_on_gps_report_interval;    // (b-2) ig on gps 정보보고 시간설정(초단위) 운행시
    unsigned short  key_off_gps_report_interval;    // (b-2) ig off gps 정보보고 시간설정(초단위) 비운행시
    unsigned char   low_batt_voltage;    // (b-1) 저전압설정 : 0.1v 단위 설정
    unsigned short  mdm_reset_interval; // (b-2) 단말주기적 리셋시간 : 03:20 에 리셋시에는 320
    unsigned char   warnning_horn_cnt;    // (b-1) 경적횟수 : 1~10
    unsigned char   warnning_light_cnt;   // (b-1) 비상등횟수
    unsigned char   over_speed_limit_km;     // (b-1) 과속 기준속도 : km/h // alm2 : 98,99 반복이벤트 횟수 
    unsigned short  over_speed_limit_time;     // (b-2) 과속 기준시간  : 초 // alm2 : 반복이벤트 해제
    unsigned char   use_obd;    // (b-1) obd 사용여부 : 1 : 사용 / 0 : 사용않함
    unsigned char   use_dtg;    // (b-1) dtg 사용여부 : 1 : 사용 / 0 : 사용않함
    unsigned char   use_zone_function;  //(b-1) zone 입출입가능여부 : 1 사용 / 0 사용않함
    unsigned char   use_bcm;   // (b-1) all key 사용여부 : 1 사용 / 0 사용않함
    unsigned char   use_knock_sensor;  // (b-1) 노크센서 사용여부 : 1 사용 / 0 사용않함
#ifdef PKT_VER_V_1_6   
    unsigned short  door_lock_time; // (b-2) Door 자동잠금시간 : 0 : 사용안함 / 1이상 초단위
#endif
    unsigned char   reserved;   // (b-1) reserved
#ifdef PKT_VER_POWERSAVE_MODE
    unsigned char   powersave_mode_start;   //  (b-1) Power Save Mode진입 기준값 // 0.1 volt단위로 설정 (예: 108 = 10.8 volt)
    unsigned char   powersave_mode_end;     //  (b-1) Power Save Mode WakeUp 기준값 // 0.1 volt단위로 설정 (예: 120 = 12.0 volt)
#endif
}__attribute__((packed))ALLOC_PKT_RECV__MDM_SETTING_VAL;


// ===============================================================================================
//  e_mdm_stat_evt = 0x02
// ===============================================================================================
typedef enum {
    e_evt_code_normal = 0, // 0 : 정상
    e_evt_code_poweroff = 1, // 1 : 전원 해제
    e_evt_code_poweron = 2, // 2 : 전원 인가
    e_evt_code_car_low_batt = 3, //3 : 차량배터리 저전압
    e_evt_code_internal_low_batt = 4, // 4 : 자체 배터리 저전압
    e_evt_code_igi_on = 5, // 5 : IG1 On
    e_evt_code_igi_off = 6, // 6 : IG1 Off
    e_evt_code_gps_reset = 7, // 7 : GPS Reset(장기간 fix 안된경우)
    e_evt_code_mdm_reset = 8, // 8 : 단말리셋
    e_evt_code_door_open = 9, // 9 : Door open
    e_evt_code_door_close = 10, // 10 : Door close
    e_evt_code_trunk_open = 11, // 11 : trunk open
    e_evt_code_trunk_close = 12, // 12 : trunk close
    e_evt_code_door_lock = 13, //  13 : Door lock
    e_evt_code_door_unlock = 14, // 14 : Door unlock
    e_evt_code_dev_bcm_err = 20,// 20 : BCM 이상
    e_evt_code_dev_obd_err = 21, // 21 : OBD 이상
#ifdef PKT_VER_POWERSAVE_MODE
    e_evt_code_powersave_mode_start = 91, // 91 : powersave mode start
#endif
    e_evt_code_sensor_1_on = 98, // 98 : 근접센서 장착
    e_evt_code_sensor_1_off = 99 // 99 : 근접센서 미장착
} e_ALLOC2_MDM_EVT_CODE;

// 4.	단말 상태 정보(이벤트) 
// 4.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   car_stat;   // (b-1) 차량상태구분 : 0x00 IG1 on , 0x01 IG1 off, 0x02 이동중
    unsigned char   mdm_rssi;   // (b-1) 모뎀 수신레벨 : rssi
    unsigned short  key_on_gps_report_interval;    // (b-2) ig on gps 정보보고 시간설정(초단위) 운행시
    unsigned short  key_off_gps_report_interval;    // (b-2) ig off gps 정보보고 시간설정(초단위) 비운행시
    unsigned short  low_batt_voltage;    // (b-2) 저전압설정 : 0.1v 단위 설정
    unsigned char   warnning_horn_cnt;    // (b-1) 경적횟수 : 1~10
    unsigned char   warnning_light_cnt;   // (b-1) 비상등횟수
    unsigned char   over_speed_limit_km;     // (b-1) 과속 기준속도 : km/h
    unsigned short  over_speed_limit_time;     // (b-2) 과속 과속기준시간 secs
    unsigned char   main_pwr; // (b-1) 주전원 유형 : 0 상시전원 , 1 내장배터리
    unsigned char   always_pwr_stat; // (b-1) 상시전원상태 : 0 : 정상 1 : 저전압 2 : 탈거
    unsigned short  mdm_reset_interval; // (b-2) 단말주기적 리셋시간 : 03:20 에 리셋시에는 320
    unsigned char   mdm_internal_batt_stat; // (b-1) 자체배터리상태 : 0 : 정상 1 : 저전압 2 : 탈거
    unsigned char   mdm_gps_stat; // (b-1) gps 상태 : 0 : 정상 1 : 무효(연결 위성수가 기준치 이하) 2 : 탈거
    unsigned short  car_batt_volt; // (b-2) 차량배터리전압 : 0.1 volt 단위
    unsigned short  mdm_internal_batt_volt; // (b-2) 자체배터리전압 : 0.1 volt 단위
    unsigned char   car_start_stop; // (b-1) 시동차단여부 : 0: 정상 1: 시동차단중
    unsigned short  event_code; // (b-2) 발생 이벤트 code : 
    /*
    0 : 정상
    1 : 전원 해제
    2 : 전원 인가
    3 : 차량배터리 저전압
    4 : 자체 배터리 저전압
    5 : IG1 On
    6 : IG1 Off
    7 : GPS Reset(장기간 fix 안된경우)
    8 : 단말리셋
    9 : Door open
    10 : Door close
    11 : trunk open
    12 : trunk close
    13 : Door lock
    14 : Door unlock
    */
    unsigned short  over_speed_cnt; // (b-2) 과속횟수 
}__attribute__((packed))ALLOC_PKT_SEND__MDM_STAT_EVT;

// 4.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved : 0: 정상
}__attribute__((packed))ALLOC_PKT_RECV__MDM_STAT_EVT;


// ===============================================================================================
// e_mdm_gps_info = 0x03,
// ===============================================================================================

// 5.	GPS 정보
// 5.1.	단말 -> 중계서버
#ifdef SERVER_ABBR_ALM1
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   gps_time; // (b-4) gps 생성시간 : unix timestamp
    unsigned int   gps_lat; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
    unsigned int   gps_lon; // (b-4) gps 경도 : WGS84좌표체계 예)127123456
    unsigned short gps_dir; // (b-2) gps 방향 : 360도표현
    unsigned short gps_speed; // (b-2) gps 속도 : 속도단위에 의존적임
    unsigned int   total_distance; // (b-4) 누적거리 meter
    unsigned int   day_distance; // (b-4) 일일 운행거리 meter
    unsigned int   section_distance; // (b-4) 구간운행거리 : ig1 on ~ ig1 off 누적거리
    unsigned int   gps_vector; // (b-4) 이동거리 : 이전좌표와 현재 좌표와의 거리
    // unsigned char  reserved[2]; // reserved // spec 1.8 에서 변경 : 차량배터리로
    unsigned short car_batt; // spec 1.8 에서 변경 : 차량배터리로
}__attribute__((packed))ALLOC_PKT_SEND__MDM_GPS_INFO;
#endif

#ifdef SERVER_ABBR_ALM2
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   gps_time; // (b-4) gps 생성시간 : unix timestamp
    unsigned int   gps_lat; // (b-4) gps 위도 : WGS84좌표체계 예)127123456
    unsigned int   gps_lon; // (b-4) gps 경도 : WGS84좌표체계 예)127123456
    unsigned short gps_dir; // (b-2) gps 방향 : 360도표현
    unsigned short gps_speed; // (b-2) gps 속도 : 속도단위에 의존적임
    unsigned int   total_distance; // (b-4) 누적거리 meter
    unsigned int   day_distance; // (b-4) 일일 운행거리 meter
    unsigned int   section_distance; // (b-4) 구간운행거리 : ig1 on ~ ig1 off 누적거리
    unsigned int   gps_vector; // (b-4) 이동거리 : 이전좌표와 현재 좌표와의 거리
    // unsigned char  reserved[2]; // reserved // spec 1.8 에서 변경 : 차량배터리로
    unsigned short car_batt; // spec 1.8 에서 변경 : 차량배터리로
    unsigned char  gps_invalid; // 0 : valid , 1: invalid  // FIX: 180308 add
    unsigned char  reserved; // FIX: 180308 add
}__attribute__((packed))ALLOC_PKT_SEND__MDM_GPS_INFO;
#endif



// 5.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved : 0: 정상
}__attribute__((packed))ALLOC_PKT_RECV__MDM_GPS_INFO;


 // ===============================================================================================
 // e_obd_dev_info = 0x11
// ===============================================================================================

// 6.	OBD 기본 설정 정보
// 6.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   obd_sn; // (b-4) obd sn
    unsigned char  reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__OBD_DEV_INFO;

// 6.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned short  obd_recv_keyon_interval; // (b-2) OBD정보 주기 (IG1 On) // OBD정보 보고 시간설정 (초단위) // 운행시
    unsigned short  obd_recv_keyoff_interval; // (b-2) OBD정보 주기 (IG1 off) // OBD정보 보고 시간설정 (초단위) // 비운행시
    unsigned char   reserved[2];   // (b-2) reserved : 0: 정상
}__attribute__((packed))ALLOC_PKT_RECV__OBD_DEV_INFO;


// ===============================================================================================
// e_obd_stat = 0x12, 
// ===============================================================================================
typedef struct {
    unsigned char   obd_stat_flag; // (b-1) obd 연결상태 0:ok 1:fail
    unsigned char   obd_stat; // (b-1) obd 상태
    unsigned char   obd_remain_fuel_stat; // (b-1) 연료잔량 측정상태
    unsigned char   obd_evt_code; // (b-1) 이벤트코드  0 : 정상, 1 : 주유, 2 : 인출
    unsigned char   obd_fuel_type; // (b-1) 연료타입 0 : OBD 미지정, 1 : 가솔린, 2 : 디젤, 3 : LPG, 4 : 전기차, 5 : 수소차
    unsigned short  obd_remain_fuel; // (b-2) 주요/인출/충전량 : 연료단위에 의존적임
}__attribute__((packed))ALLOC_PKT_SEND__OBD_STAT_ARG;


// 7.	OBD 상태 정보(이벤트)
// 7.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   obd_stat_flag; // (b-1) obd 연결상태 0:ok 1:fail
    unsigned char   obd_stat; // (b-1) obd 상태
    /*
    0x00 OK
    0x01 On Calibration
    0x02 On Download ready
    (MBP download or FW update ready)
    0x03 On Diagnostics(equal ok)
    0x10 On Firmware Upgrade
    0x5F Cannot Communicate with Car (Ignition Off)
    0x6F Cannot Communicate with Car
    0x7F Vehicle Information not Exist
    */
    unsigned char   obd_remain_fuel_stat; // (b-1) 연료잔량 측정상태
    /*
    0  Not Supported
    -1  Not Corrected State
    -7  RemainedFuelFull State
    -8  Sensor Line Open
    -5  FuelInputRecognization State
    -6  FuelInputCalcuating State
    +1  Normal State
    -11,-12,-13,-14 Cali State
    */
    unsigned char  obd_evt_code; // (b-1) 이벤트코드  0 : 정상, 1 : 주유, 2 : 인출
    unsigned char  obd_fuel_type; // (b-1) 연료타입 0 : OBD 미지정, 1 : 가솔린, 2 : 디젤, 3 : LPG, 4 : 전기차, 5 : 수소차
    unsigned short obd_remain_fuel; // (b-2) 주요/인출/충전량 : 연료단위에 의존적임
    unsigned char  reserved; // (b-1) reserved
}__attribute__((packed))ALLOC_PKT_SEND__OBD_STAT;

// 7.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved : 0: 정상
}__attribute__((packed))ALLOC_PKT_RECV__OBD_STAT;


// ===============================================================================================
// e_obd_data = 0x13
// ===============================================================================================

// 8.	 OBD 수집 정보
// 8.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   daily_driving_time; // (b-4) 일일 운행시간 : 단위 초 ==> 취득불가
    unsigned int   daily_distance; // (b-4) 일을 누적거리 단위 m ==> 취득불가.
    unsigned int   daily_engine_idle; // (b-4) 일일 공회전시간 =-
    unsigned char  fuel_type; // (b-1) 연료타입 : 0 : OBD 미지정, 1 : 가솔린, 2 : 디젤, 3 : LPG, 4 : 전기차, 5 : 수소차
    unsigned short fuel_remain; // (b-2) 현재연료잔량 : 연료단위에 의존적임 (단위0.1L)
    unsigned short rpm; // (b-2) 현재 rpm
    unsigned short cooling_temp; // (b-2) 냉각수온도,
    unsigned short car_batt_volt; // (b-2) 현재 차량배터리 : 단위 0.1v
    unsigned short speed; // (b-2) 현재 속도
    unsigned int   fuel_supply; // (b-4) 누적주유, 충전량 : 연료단위에 의존적임 (단위 0.1L)
    unsigned int   total_fuel_usage; // (b-4) 누적사용량 : 연료 단위에 의존적임 (단위 0.1L)
    unsigned int   total_distance; // (b-4) 총 누적거리 : 단위 m  --> TDD
    unsigned char  brake_stat;  // (b-1) 현재 브레이크상태 : 0 : OFF, 1 : ON, 2 : 미지원
    unsigned int   over_accel_cnt; // (b-4) 시동후 급가속 카운트 
    unsigned int   over_deaccel_cnt; // (b-4) 시동후 급감속 카운트 
    unsigned short car_inclination; // (b-2) 차량기울기 : value/100
    unsigned char  engine_start_cnt; // (b-1) 시동카운트  : 0~255증가 (하루기준)
    unsigned int   section_time; // (b-4) 시동후 누적운행시간 : 단위 초
    unsigned int   section_distance; // (b-4) 시동후 누적거리 : 단위 ?
    unsigned int   section_engine_idle_time; // (b-4) 시동후 공회전시간 : 단위 초
    unsigned int   section_fuel_usage; // (b-4) 시동후 누적연료소모량 ( 단위 0.1L)
    unsigned int   pannel_total_distance; // (b-4) 계기판 누적거리 ( meter)
    unsigned int   pannel_section_distance; // (b-4) 시동후 계기판 누적거리 ( meter)
    unsigned int   pannel_driving_availabe; // (b-4) 계기판 주행가능거리 ( meter)
    unsigned char  reserved; // (b-1) reserved
}__attribute__((packed))ALLOC_PKT_SEND__OBD_DATA;

// 8.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved : 0: 정상
}__attribute__((packed))ALLOC_PKT_RECV__OBD_DATA;


// ===============================================================================================
// ===============================================================================================

// 9.	OBD 차량 진단코드
// 9.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char  diag_code[100]; // (a-100) 차량진단코드
}__attribute__((packed))ALLOC_PKT_SEND__OBD_DIAG_CODE;

// 9.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved : 0: 정상
}__attribute__((packed))ALLOC_PKT_RECV__OBD_DIAG_CODE;


// ===============================================================================================
// ===============================================================================================

// 10.	 DTG 기본 정보
// 10.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char  dtg_model_no[20]; // (a-20) dtg 모델명
    unsigned char  dtg_vin[17]; // (a-17) 차대번호
    unsigned char  car_type[2]; // (a-2) 자동차 유형 : 자동차유형코드
    unsigned char  car_reg_no[12]; // (a-12) 자동차 등록번호
    unsigned char  company_no[10]; // (a-10) 운송사업자 번호
    unsigned char  driver_no[18]; // (a-18) 운전자코드
    unsigned char  reserved; // (b-1) reserved
}__attribute__((packed))ALLOC_PKT_SEND__DTG_INFO;

// 10.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved : 1: 정상
}__attribute__((packed))ALLOC_PKT_RECV__DTG_INFO;


// ===============================================================================================
// ===============================================================================================

// 11.	DTG 수집 정보
// 11.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char    dtg_drive_end; // (b-1) 운행종료여부 : 0:운행중, 1:운행종료(Key Off상태)
    unsigned char    dtg_reserved; // (b-1) reserved
    unsigned short   dtg_data_cnt; // (b-2) dtg data count
    unsigned short   dtg_daily_distance; // (b-2) 일일주행거리 : 0.1km 단위
    unsigned short   dtg_total_distance; // (b-2) 누적주행거리 : 0.1km 단위
    unsigned int     dtg_time; // (b-4) 정보발생일시
    unsigned short   dtg_car_speed; // (b-2) 차량속도
    unsigned short   dtg_car_rpm; // (b-2) 분당엔진회전수
    unsigned short   dtg_car_break_signal; // (b-2) 브레이크신호 : 0(Off), 1(On)
    unsigned int     dtg_gps_lat; // (b-4) 차량위치 경도 예)127123456
    unsigned int     dtg_gps_lon; // (b-4) 차량위치 위도 예)37123456
    unsigned int     dtg_gps_dir; // (b-4) gps 방위각 (0 ~ 359)
    unsigned short   dtg_accelation_speed_x; // (b-2) 가속도 : -1000 ~ +1000, (실제값은 /10 하여 사용), 예) -150 -> -15.0
    unsigned short   dtg_accelation_speed_y; // (b-2) 가속도 : -1000 ~ +1000, (실제값은 /10 하여 사용), 예) -150 -> -15.0
    unsigned short   dtg_diag_code; // (b-2) 기기 및 통신상태 코드
}__attribute__((packed))ALLOC_PKT_SEND__DTG_DATA;

// 11.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved 
}__attribute__((packed))ALLOC_PKT_RECV__DTG_DATA;


// ===============================================================================================
// ===============================================================================================

//  12.	zone 설정 정보
// 12.1.	단말 -> 중계서버

typedef struct {
    unsigned int  zone_req_id; // (b-4) zone id
    unsigned int  zone_req_lat; // (b-4) 위도
    unsigned int  zone_req_lon; // (b-4) 경도
    unsigned short zone_req_distance; // (b-2) 반경
}__attribute__((packed))ALLOC_PKT_SEND__ZONE_REQ_DATA;

typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char  reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__ZONE_REQ;
 
// 11.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2]; // (b-2) reserved
    unsigned short  zone_req_data_cnt; // (b-2) 데이터갯수 // max 50
    ALLOC_PKT_SEND__ZONE_REQ_DATA zone_req_data[50]; // 가변..
}__attribute__((packed))ALLOC_PKT_RECV__ZONE_REQ;


// ===============================================================================================
// ===============================================================================================

// 13.	zone 입출 정보
// 13.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   zone_id; // (b-4) zone id
    unsigned char  zone_type; // (b-1) 진입/진출유형 : 1: 진입 ,  2: 진출
    unsigned int   zone_evt_time; // (b-4) 이벤트발생시간
    unsigned char  reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__ZONE_DATA;

// 13.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved 
}__attribute__((packed))ALLOC_PKT_RECV__ZONE_DATA;


// ===============================================================================================
// ===============================================================================================
// e_bcm_statting_val = 0x42,  // 0x42 : All key 설정 정보
// 14.	All key 기본 설정 정보
// 14.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   bcm_allkey_sn; // (b-4) all key sn
    unsigned char  reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__BCM_ALLKEY_DATA;

// 14.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   bcm_allkey_gps_use; // (b-1) gps 사용여부 0 : 사용안함, 1 : 사용
    unsigned char   bcm_allkey_bt_use; // (b-1) bt 사용여부 0 : 사용안함, 1 : 사용
    unsigned char   bcm_allkey_rfid_use; // (b-1) rfid 사용여부 0 : 사용안함, 1 : 사용
    unsigned char   bcm_allkey_smoke_sensor_use; // (b-1) 흡연센서 사용여부  0 : 사용안함, 1 : 사용
    unsigned char   bcm_allkey_oil_card_use; // (b-1) 주유카드사용여부  0 : 사용안함, 1 : 사용
    unsigned char   bcm_allkey_beacon_tx_power; // (b-1) beacon tx power 1~5
    unsigned short  bcm_allkey_smoke_co2; // (b-2) 흡연기준 co2
    unsigned short  bcm_allkey_smoke_led; // (b-2) 흡연시 led
    unsigned short  bcm_allkey_smoke_beep; // (b-2) 흡연시 비프음
    unsigned short  bcm_allkey_rfid_tag_success_led; // (b-2) rfid 태깅 성공시 led
    unsigned short  bcm_allkey_rfid_tag_success_beep; // (b-2) rfid 태깅 성공시 beep
    unsigned short  bcm_allkey_rfid_tag_fail_led; // (b-2) rfid 태깅 실패시 led
    unsigned short  bcm_allkey_rfid_tag_fail_beep; // (b-2) rfid 태깅 실패시 beep
    unsigned char   bcm_allkey_oil_card_id[16]; // (a-16) 주유카드 id
    unsigned char   bcm_allkey_oil_card_out_noti; // (b-1) 주유카드 out 지속시 알람기준시간 분단위
    unsigned short  bcm_allkey_oil_card_in_led; // (b-2) 주유카드 in led
    unsigned short  bcm_allkey_oil_card_in_beep; // (b-2) 주유카드 in 비프음
    unsigned short  bcm_allkey_oil_card_out_led; // (b-2) 주유카드 in led
    unsigned short  bcm_allkey_oil_card_out_beep; // (b-2) 주유카드 in 비프음
    unsigned char   reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_RECV__BCM_ALLKEY_DATA;





// ===============================================================================================
// ===============================================================================================

// 15.	All key 상태 정보(이벤트)
// 15.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   bcm_allkey_stat_code; // (b-1) all key 상태 :  0x00 OK, 0x01 On Calibration, 0x02 Cannot Communicate with CSD
    unsigned char   bcm_allkey_evt_code; // (b-1) 이벤트코드 : 0 : 정상, 1 : 주유카드 In, 2 : 주유카드 Out, 3 : 흡연감지, 4 : RFID 태깅, 5 : BLE connect
    unsigned char   bcm_allkey_rfid[16]; // (a-16) rfid 
    unsigned char   reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__BCM_ALLKEY_STAT_INFO_EVT;

// 15.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header;
    unsigned char   reserved[2];   // (b-2) reserved 
}__attribute__((packed))ALLOC_PKT_RECV__BCM_ALLKEY_STAT_INFO_EVT;


// ===============================================================================================
// ===============================================================================================

// 16.	마스터 카드 등록
// 16.1.	단말 -> 중계서버
#define MAX_SUPPORT_MASTERCARD_RFID_CNT 100
typedef struct {
    unsigned char   mastercard_rfid_data[16]; // (a-16) rfid : 0 개면 0x00 으로 보냄 // 반복
}__attribute__((packed))ALLOC_PKT_SEND__MASTER_CARD_REGI_DATA;

typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__MASTER_CARD_REGI;

typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned short  mastercard_cnt; // (b-2) 마스터카드 갯수
    ALLOC_PKT_SEND__MASTER_CARD_REGI_DATA  mastercard_rfid[MAX_SUPPORT_MASTERCARD_RFID_CNT]; // 최대 100개가지만 지원하자 // 가변데이터
}__attribute__((packed))ALLOC_PKT_RECV__MASTER_CARD_REGI;


// ===============================================================================================
// ===============================================================================================

// 17.	예약정보
// 17.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__RESERVED_INFO_REQ;

// 17.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char    reservation_cnt; // (b-1) 예약정보갯수 // 0~20개
    unsigned char    reservation_stat; // (b-1) 예약상태 // 0 : 예약중 , 1 : 운행중, 2 : 지연중
    unsigned char    reservation_rfid[16]; // (a-16) rfid // 0개면 0x00보냄
    unsigned int     reservation_no; // (b-4) 예약번호 // unix timestamp
    unsigned int     reservation_time_start; // (b-4) 예약시작시간 // unix timestamp
    unsigned int     reservation_time_end; // (b-4) 예약종료시간 // unix timestamp
}__attribute__((packed))ALLOC_PKT_RECV__RESERVED_INFO_REQ;


// ===============================================================================================
// ===============================================================================================
// e_bcm_knocksensor_setting_val = 0x51
// 18.	노크센서 설정 정보
// 18.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   reserved[2]; // (b-2) reserved
}__attribute__((packed))ALLOC_PKT_SEND__KNOCK_SENSOR_INFO_REQ;

// 18.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned short  id; // (B-2) id
    unsigned short  master_number; // (B-2) master number
    unsigned char   reserved[1]; // (b-1) reserved
}__attribute__((packed))ALLOC_PKT_RECV__KNOCK_SENSOR_INFO_REQ;


// ===============================================================================================
// ===============================================================================================

// 19.	펌웨어 정보
// 19.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   firm_mdm_ver[10]; // (a-10) 단말 f/w 버젼
    unsigned char   firm_obd_ver[10]; // (a-10) obd fw 버젼
    unsigned char   firm_car_code[10]; // (a-10) car code
    unsigned short  firm_car_code_version; // (b-2) car code version
    unsigned char   firm_allkey_ver[10]; // (a-10) allkey fw 버젼
    unsigned char   firm_bcm_ver[10]; // (b-10) bcm fw 버젼
    unsigned char   firm_dtg_ver[10]; // (a-10) dtg fw 버젼
    unsigned char   reserved[2]; // (b-2) reserved 
}__attribute__((packed))ALLOC_PKT_SEND__FIRMWARE_INFO;

// 19.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   reserved[2]; // (b-2) reserved 0 : 정상
}__attribute__((packed))ALLOC_PKT_RECV__FIRMWARE_INFO;


// ===============================================================================================
// ===============================================================================================

// 20.	펌웨어 업데이트
// 20.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   firm_req_info; // (b-1) 요청펌웨어 구분 : 0x00 : 단말 펌웨어  , 0x01 : OBD 펌웨어, 0x02 : OBD 초기화 , 0x03 : AllKey 펌웨어, 0x04 : BCM 펌웨어, 0x05 : DTG 펌웨어, 0x06 : 노크센서 펌웨어
    unsigned char   firm_cur_ver[10]; // (a-10) 현재 fw 버젼
    unsigned char   reserved[2];
}__attribute__((packed))ALLOC_PKT_SEND__FIRMWARE_UPDATE_REQ;

// 20.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   firm_ip_port; // (a-22) ip,port // 예)111.112.113.114:1234
    unsigned char   firm_download_url[80]; // (a-80) 다운로드 url 정보
    unsigned char   firm_lastest_ver[10]; // (a-10) 최신 fw 버젼 // 최신이면 null, 아니면 단말은 fota 로 fw 업그레이드 처리
    unsigned char   reserved[2]; // (b-2) reserved // 0 정상
}__attribute__((packed))ALLOC_PKT_RECV__FIRMWARE_UPDATE_REQ;


// ===============================================================================================
// ===============================================================================================

// 21.	펌웨어 업데이트 완료
// 21.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   firm_complete_code; // (b-1) 완료구분 //  0x00 : 단말 펌웨어 완료 , 0x01 : OBD 초기화 완료 , 0x02 : OBD 펌웨어 완료, 0x03 : DTG 펌웨어 완료, 0x04 : AllKey 펌웨어 완료, 0x05 : 노크센서 펌웨어 완료
    unsigned char   firm_complete_ver[10]; // (a-10) 버젼명
    unsigned char   reserved[1];
}__attribute__((packed))ALLOC_PKT_SEND__FIRMWARE_UPDATE_COMPLETE;

// 21.2.	중계서버 -> 단말
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   reserved[2]; // (b-2) reserved 
}__attribute__((packed))ALLOC_PKT_RECV__FIRMWARE_UPDATE_COMPLETE;


// ===============================================================================================
// ===============================================================================================
typedef struct {
    unsigned int   sms_cmd_seq; // (b-4) 명령sms seq :   수신한 sms command seq.
    unsigned char  sms_cmd_code; // (b-1) 명령구분, //  10:단말기 설정 명령, 30:시동 차단 명령, 40:단말기 상태요청 명령, 50:단말기 초기화 명령, 60:단말기 reboot명령, 70:OBD 설정 명령, 80:제어명령
    unsigned char  sms_cmd_success; // (b-1) sms 명령 성공여부 // 0: 성공, 1: 실패
    char  sms_cmd_contents[80]; // (a-80) 수신sms 전문
}__attribute__((packed))ALLOC_PKT_SEND__SMS_PKT_ARG;

// 22.	SMS 수신 정보
// 22.1.	단말 -> 중계서버
typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned int   sms_cmd_seq; // (b-4) 명령sms seq :   수신한 sms command seq.
    unsigned char  sms_cmd_code; // (b-1) 명령구분, //  10:단말기 설정 명령, 30:시동 차단 명령, 40:단말기 상태요청 명령, 50:단말기 초기화 명령, 60:단말기 reboot명령, 70:OBD 설정 명령, 80:제어명령
    unsigned char  sms_cmd_success; // (b-1) sms 명령 성공여부 // 0: 성공, 1: 실패
    char  sms_cmd_contents[80]; // (a-80) 수신sms 전문
}__attribute__((packed))ALLOC_PKT_SEND__SMS_RECV_INFO;

typedef struct {
    ALLOC_PKT_COMMON_HEADER header; // msesage header 공통
    unsigned char   reserved[2]; // (b-2) reserved // 0 정상
}__attribute__((packed))ALLOC_PKT_RECV__SMS_RECV_INFO;


//-------------------------------------------------------------------------------------------
// e_mdm_setting_val = 0x01
int make_pkt__mdm_setting_val(unsigned char **pbuf, unsigned short *packet_len);
int parse_pkt__mdm_setting_val(ALLOC_PKT_RECV__MDM_SETTING_VAL* recv_buff, char* target_ip, int target_port);

// e_mdm_stat_evt = 0x02,
int make_pkt__mdm_stat_evt(unsigned char **pbuf, unsigned short *packet_len, int evt_code);
int parse_pkt__mdm_stat_evt(ALLOC_PKT_RECV__MDM_STAT_EVT* recv_buff);

// e_mdm_gps_info = 0x03,  
int make_pkt__mdm_gps_info(unsigned char **pbuf, unsigned short *packet_len);
int parse_pkt__mdm_gps_info(ALLOC_PKT_RECV__MDM_GPS_INFO* recv_buff);

// e_obd_dev_info = 0x11
int make_pkt__obd_dev_info(unsigned char **pbuf, unsigned short *packet_len);
int parse_pkt__obd_dev_info(ALLOC_PKT_RECV__OBD_DEV_INFO* recv_buff);

// e_obd_stat = 0x12
int make_pkt__obd_stat(unsigned char **pbuf, unsigned short *packet_len, ALLOC_PKT_SEND__OBD_STAT_ARG obd_stat_arg);
int parse_pkt__obd_stat(ALLOC_PKT_RECV__OBD_STAT* recv_buff);

// e_obd_data = 0x13
int make_pkt__obd_data(unsigned char **pbuf, unsigned short *packet_len);
int parse_pkt__obd_data(ALLOC_PKT_RECV__OBD_DATA* recv_buff);

// e_sms_recv_info = 0xf0, // 0xF0 : SMS 수신 정보
int make_pkt__sms_recv_info(unsigned char **pbuf, unsigned short *packet_len, ALLOC_PKT_SEND__SMS_PKT_ARG sms_pkt_arg);
int parse_pkt__sms_recv_info(ALLOC_PKT_RECV__SMS_RECV_INFO* recv_buff);

// e_firm_info = 0x71, // 0x71 : 펌웨어 정보
int make_pkt__firmware_info(unsigned char **pbuf, unsigned short *packet_len);
int parse_pkt__firm_info(ALLOC_PKT_SEND__FIRMWARE_INFO* recv_buff);

#endif
