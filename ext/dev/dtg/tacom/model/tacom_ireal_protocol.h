#ifndef __DTG_IREAL_DATA_FORMAT_DEFINE_HEADER__
#define __DTG_IREAL_DATA_FORMAT_DEFINE_HEADER__

#include <stdint.h>

// 표준시간정보 
typedef struct {
	uint8_t	Year;
	uint8_t Mon;
	uint8_t Day;
	uint8_t Hour;
	uint8_t Min;
	uint8_t Sec;
	uint8_t mSec;
}__attribute__((packed))IREAL_DATE;

// 헤더데이타
typedef struct {
	IREAL_DATE Date;				// 시간정보
	uint8_t	DTGModel[20];			// 운행기록계 모델명
	uint8_t	VIN[17];				// 차대번호
	uint8_t	VechicleType[2];		// 자동차 유형
	uint8_t	VRN[12];				// 자동차 등록번호
	uint8_t	BRN[10];				// 운송사업자 등록번호
	uint8_t	DCode[18];				// 운전자 코드
} __attribute__((packed))tacom_ireal_hdr_t;

// 운행기록데이타(TC데이타)
typedef struct {
	IREAL_DATE	 		Date;				// 시간정보
// 장치상태 Bit Map
#define STATUS_BRAKE					0x0001
#define WARN_SPEED_SENSOR				0x0002
#define WARN_RPM_SENSOR					0x0004
#define WARN_ACCELATION_SENSOR			0x0008
#define WARN_GPS						0x0010
#define WARN_POWER						0x0020
#define WARN_BREAK_DETECT_SENSOR		0x0100
#define WARN_SENSOR_INPUT				0x0200
#define STATUS_KEY						0x0400
	uint16_t			Status;				// 장치상태
	uint8_t		 		Speed;				// 차량속도
	uint16_t			DistanceAday;		// 일일주행거리
	uint32_t			DistanceAll;		// 누적주행거리
	uint32_t			GPS_Y;				// 경도	(GPS Y) 
	uint32_t			GPS_X;				// 위도	(GPS X) 환산 value / 1000000 = 127.123456
	uint16_t			Azimuth;			// GPS방위각
	uint16_t			RPM;				// RPM
	int8_t				Accelation_X;		// 가속도(x) 범위 +127 ~ -128
	int8_t				Accelation_Y;		// 가속도(y) 환산식 : 가속도 = value * (16.0/256.0) * 0.98066
} __attribute__((packed))tacom_ireal_data_t;

// 실시간데이타(1초데이타)
typedef struct {
	tacom_ireal_data_t	 	DTGBody;			// 운행기록데이타
	uint8_t		 		DrivingTimeAday;	// 일일운행 시간(초)
}__attribute__((packed))IREAL_DTGREAL;

/* value of expression 'MAX_IREAL_DATA * MAX_IREAL_DATA_PACK' 
 * have to set over ireal_setup.max_records_per_once's value.
 */
#define MAX_IREAL_DATA		10
#define MAX_IREAL_DATA_PACK	600

#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

typedef struct {
	unsigned int status;
	unsigned int count;
	tacom_ireal_data_t buf[MAX_IREAL_DATA];
}ireal_data_pack_t;

extern const struct tm_ops ireal_ops;
extern struct tacom_setup ireal_setup;

#endif	//__DTG_IREAL_DATA_FORMAT_DEFINE_HEADER__
