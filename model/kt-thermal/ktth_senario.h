<<<<<<< HEAD
#ifndef __BTN_SENARIO_H__
#define __BTN_SENARIO_H__

#include <board/board_system.h>

#define BTN_STATUS__PUSH      1
#define BTN_STATUS__RELEASE   0

#define BTN_TYPE__START      1
#define BTN_TYPE__END        2

#define KTTH_RESUME_DATA_PATH    CONCAT_STR(USER_DATA_DIR, "/ktth_resum_v1.dat")

int ktth_sernaio__init_btn();
int ktth_sernaio__push_btn(int btn_type, int status);
int ktth_sernaio__normal_pkt_odo_calc(int odo);
int ktth_sernaio__keybtn_pkt_odo_calc();

void ktth_sernaio__load_resume_data();
void ktth_sernaio__save_resume_data();

typedef struct {
	int btn_start__stat;
    int btn_start__mileage;
	int btn_end__stat;
    int btn_end__mileage;
}__attribute__((packed))btn_status_t;

typedef struct {
	btn_status_t btn_data;
}__attribute__((packed))kttn_resum_data_t;


#endif // __BTN_SENARIO_H__

=======
#ifndef __BTN_SENARIO_H__
#define __BTN_SENARIO_H__

#include <board/board_system.h>

#define BTN_STATUS__PUSH      1
#define BTN_STATUS__RELEASE   0

#define BTN_TYPE__START      1
#define BTN_TYPE__END        2

#define KTTH_RESUME_DATA_PATH    CONCAT_STR(USER_DATA_DIR, "/ktth_resum_v1.dat")

int ktth_sernaio__init_btn();
int ktth_sernaio__push_btn(int btn_type, int status);
int ktth_sernaio__normal_pkt_odo_calc(int odo);
int ktth_sernaio__keybtn_pkt_odo_calc();

void ktth_sernaio__load_resume_data();
void ktth_sernaio__save_resume_data();

typedef struct {
	int btn_start__stat;
    int btn_start__mileage;
	int btn_end__stat;
    int btn_end__mileage;
}__attribute__((packed))btn_status_t;

typedef struct {
	btn_status_t btn_data;
}__attribute__((packed))kttn_resum_data_t;


#endif // __BTN_SENARIO_H__

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
