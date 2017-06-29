#ifndef __ALLOC2_OBD_MGR_H__
#define __ALLOC2_OBD_MGR_H__

#define MAX_FAIL_CNT_CHK    60

typedef struct {
    int obd_stat;
    char fuel_type[64];
    char obd_sn[64];
    char obd_swver[64];
}SECO_OBD_INFO_T;

typedef struct {
    unsigned int mdm_time_stamp;
    unsigned int obd_data_fuel_remain;
    unsigned int obd_data_rpm;    // 현재 rpm
    unsigned int obd_data_cot;    // 냉각수온도
    unsigned int obd_data_car_volt; // 차량 밧데리전압
    unsigned int obd_data_car_speed; // 차량 속도
    unsigned int obd_data_total_distance; // 총 누적거리
    unsigned int obd_data_break_signal; // 브레이크상태
    unsigned int obd_data_panel_distance; // 계기판누적거리
}SECO_OBD_DATA_T;

// 브로드 캐스트 메시지처리부분
int alloc2_obd_mgr__obd_broadcast_proc(const int argc, const char* argv[]);
int alloc2_obd_mgr__obd_broadcast_start();
int alloc2_obd_mgr__obd_broadcast_stop();


int alloc2_obd_mgr__get_obd_dev_info(SECO_OBD_INFO_T* obd_info);
int alloc2_obd_mgr__get_cur_obd_data(SECO_OBD_DATA_T* cur_obd_info);

int alloc2_obd_mgr__init();




#endif // __ALLOC2_OBD_MGR_H__
