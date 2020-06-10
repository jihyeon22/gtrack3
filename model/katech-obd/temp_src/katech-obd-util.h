#ifndef __KATECH_OBD_UTIL_H__
#define __KATECH_OBD_UTIL_H__

#include "katech-obd.h"

#define OBD_DEV_PATH	"/dev/ttyMSM"


// uart 관련 함수
int katech_obd_init();
void katech_obd_deinit();

unsigned short int katech_obd_chk_checksum(const char buf[], const unsigned int len);

// 실제 데이터를 주고받는 함수 : 내부적으로 length 계산및 checksum 계산을 하고 uart 로 쏜다.
// obd_init() 이 먼저 불려야한다.
//   katech_obd_cmd : 커맨드만 입력
//   katech_obd_data : data 만 입력
//   ret_buff : 데이터 결과받을 버퍼
//   error_code : 에러값에 대한 결과
// 리턴값
//   data 의 결과값 길이

int katech_obd_write_cmd_resp(unsigned char* katech_obd_cmd, unsigned char* katech_obd_data, int obd_data_len, unsigned char* ret_buff, int* error_code);
int make_obd_cmd(const char cmd[], const int cmd_len, unsigned char outbuff[]);

#endif

