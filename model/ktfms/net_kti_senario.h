#ifndef __NET_KTI_SENARIO_H__
#define __NET_KTI_SENARIO_H__

#define MAX_SET_OBD_INFO_RETRY_CNT	4
#define CHK_KEY_STAT_HOLD_SEC2		60*10//(60*20) // req power off 날리는 시간

int check_init_stat_1();
int check_init_stat_2();

void pre_init();
void check_init();

void poweroff_proc_1();

void hw_check_proc();

#endif
