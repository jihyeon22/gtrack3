
#ifndef __ALLKEY_BCM_1_H__
#define __ALLKEY_BCM_1_H__

#define ALLKEY_BCM_RET_FAIL     -1
#define ALLKEY_BCM_RET_SUCCESS  0

#define ALLKEY_BCM_MAX_CHK_DEV_CNT  3


#define ALLKEY_BCM_UART_DEVNAME         "/dev/ttyHSL2"
#define ALLKEY_BCM_UART_BAUDRATE        4800
#define ALLKEY_BCM_UART_READ_TIMEOUT    4
#define ALLKEY_BCM_UART_READ_RETRY_CNT  2

#define ALLKEY_BCM_READ_THREAD_TIMEOUT  1


typedef struct allkey_bcm_1_info_t
{
    int init_stat;
    unsigned char horn_cnt;
    unsigned char light_cnt;
    unsigned char bcm_swver;
}ALLKEY_BCM_1_INFO_T;


int allkey_bcm_1_init(int (*p_mdm_evt_proc)(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list));
int allkey_bcm_1_chk_stat(const unsigned int evt, const unsigned char stat);

int allkey_bcm_ctr__get_info(ALLKEY_BCM_1_INFO_T* allkey_bcm_info);

int allkey_bcm_ctr__light_on(int stat);
int allkey_bcm_ctr__theft_on(int stat);
int allkey_bcm_ctr__horn_on(int stat); // 1 : on ,0 : off
int allkey_bcm_ctr__door_evt_on(int stat);
int allkey_bcm_ctr__door_lock(int stat); // 1 :lock ,0 : nulock
int allkey_bcm_cmd__get_stat();

//int (*p_mdm_evt_proc)(const int evt_code, const unsigned char stat_1, const unsigned char stat_2, const unsigned char err_list);

typedef enum
{
    e_bcm_evt_driver_call, // 차주호출
    e_bcm_evt_monitor_off_door_open, // 경계 해제시 도어 잠김
    e_bcm_evt_monitor_off_door_close, // 경계 해제시 도어 잠김
    e_bcm_evt_small_shock,  // 약한 충격감지
    e_bcm_evt_big_shock,  // 강한 충격감지
    e_bcm_evt_monitor_on_door_stat, // 침입감지 (도어)
    e_bcm_evt_monitor_on_trunk_stat, // 침입감지 (트렁크)
    e_bcm_evt_monitor_on_hood_stat, // 침입감지 (후드)
    e_bcm_evt_monitor_none,
}e_allkey_bcm_1_evt_code;


typedef enum
{
    // stat code 1
    e_bcm_stat1_door_open = 0x01,
    e_bcm_stat1_monitor_on = 0x02,
    e_bcm_stat1_engine_on = 0x04,
    e_bcm_stat1_wireless_engine_on = 0x08,
    e_bcm_stat1_hood_open = 0x10,
    e_bcm_stat1_turbo_on = 0x20,
    e_bcm_stat1_timer_on = 0x40,
    e_bcm_stat1_mute_on = 0x80,
    // stat code 2
    e_bcm_stat2_shock_off = 0x01,
    e_bcm_stat2_acc_on = 0x02,
    e_bcm_stat2_trunk_open = 0x04,
    e_bcm_stat2_always_evt_off = 0x08,
    e_bcm_stat2_ig1_on = 0x10,
    e_bcm_stat2_theft_off = 0x20,
    e_bcm_stat2_valet_on = 0x40,
    e_bcm_stat2_alam_on = 0x80,
}e_allkey_bcm_1_stat_code;





#endif // __ALLKEY_BCM_1_H__

