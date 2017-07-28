
#include <base/devel.h>
#include <base/sender.h>
#include <base/thread.h>
#include <base/watchdog.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/transfer.h>
#include <util/poweroff.h>
#include <logd_rpc.h>


#include "alloc2_pkt.h"
#include "alloc2_senario.h"
#include "alloc2_obd_mgr.h"
#include "alloc2_bcm_mgr.h"

// get_pkt_pipe_type(e_mdm_stat_evt,evt_code)
// ePIPE_1 : lifo
// ePIPE_2 : fifo
int get_pkt_pipe_type(int pkt_code, int evt_code)
{
    int ret = ePIPE_1;
    // ePIPE_2
    switch(pkt_code)
    {
        case e_mdm_stat_evt_fifo:
        case e_mdm_gps_info_fifo:
        {
            ret = ePIPE_2;
            break;
        }
        case e_mdm_stat_evt: // = 0x02,      // 0x02 : 단말 상태 정보 (이벤트)
        case e_mdm_setting_val: // = 0x01,   // 0x01 : 단말 기본 설정 정보
        case e_mdm_gps_info: // = 0x03,       // 0x03 : GPS 정보
        case e_obd_dev_info: // = 0x11,   // 0x11 : OBD 기본 설정 정보
        case e_obd_stat: // = 0x12,      // 0x12 : OBD 상태 정보 (이벤트)
        case e_obd_data: // = 0x13,          // 0x13 : OBD 수집 정보
        case e_obd_chk_code: // = 0x14,      // 0x14 : OBD 차량 진단코드
        case e_dtg_setting_val: // = 0x21,   // 0x21 : DTG 기본 정보
        case e_dtg_data: // = 0x22,          // 0x22 : DTG 수집 정보
        case e_mdm_geofence_setting_val: // = 0x31, // 0x31 : zone 설정 정보
        case e_mdm_geofence_evt: // = 0x32,  // 0x32 : zone 입출 정보
        case e_bcm_stat_evt: // = 0x41,      // 0x41 : All key 상태 정보 (이벤트)
        case e_bcm_statting_val: // = 0x42,  // 0x42 : All key 설정 정보
        case e_bcm_mastercard_regi: // = 0x45, // 0x45 : 마스터카드 등록
        case e_bcm_reserv_val: // = 0x47,    // 0x47 : 예약정보
        case e_bcm_knocksensor_setting_val: // = 0x51, // 0x51 : 노크센서 설정 정보
        case e_firm_info: // = 0x71, // 0x71 : 펌웨어 정보
        case e_firm_update: // = 0x72, // 0x72 : 펌웨어 업데이트
        case e_firm_complete: // = 0x79, // 0x79 : 펌웨어 업데이트 완료
        case e_sms_recv_info: // = 0xf0, // 0xF0 : SMS 수신 정보
        {
            ret = ePIPE_1;
            break;
        }
        default:
            ret = ePIPE_1;
            break;
    }
    return ret;
}
