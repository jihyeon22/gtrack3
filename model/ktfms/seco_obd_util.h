#ifndef __SECO_OBD_UTIL_H__
#define __SECO_OBD_UTIL_H__

#include "seco_obd.h"

#define OBD_DEV_PATH	"/dev/ttyHSL1"

typedef enum {
    eSECO_OBD_STX_IDX = 0,
    eSECO_OBD_ITEM_IDX = 1,
    eSECO_OBD_COMMAND_IDX = 2,
    eSECO_OBD_LENGTH1_IDX = 3,
	eSECO_OBD_LENGTH2_IDX = 4,
	eSECO_OBD_DATA1_IDX = eSECO_OBD_LENGTH2_IDX,
	eSECO_OBD_DATA2_IDX = 5,
} SECO_OBD_IDX;



// uart 관련 함수
int seco_obd_init();
void seco_obd_deinit();

// 실제 데이터를 주고받는 함수 : 내부적으로 length 계산및 checksum 계산을 하고 uart 로 쏜다.
// obd_init() 이 먼저 불려야한다.
//   sec_obd_cmd : 커맨드만 입력
//   sec_obd_data : data 만 입력
//   ret_buff : 데이터 결과받을 버퍼
//   error_code : 에러값에 대한 결과
// 리턴값
//   data 의 결과값 길이
int seco_obd_write_cmd_resp(char* sec_obd_cmd, unsigned char* sec_obd_data, int obd_data_len, unsigned char* ret_buff, int* error_code);


#endif

