<<<<<<< HEAD
#ifndef __SECO_OBD_MGR_H__
#define __SECO_OBD_MGR_H__

#include "seco_obd_1.h"

#define KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC     1
#define KATECH_OBD_TA2_INTERVAL_DEFAULT_SEC     5

int katech_obd_mgr__get_ta1_obd_info(SECO_CMD_DATA_SRR_TA1_T* ta1_buff);
int katech_obd_mgr__get_ta2_obd_info(SECO_CMD_DATA_SRR_TA2_T* ta2_buff);

int katech_obd_mgr__set_ta1_interval_sec(int sec);
int katech_obd_mgr__set_ta2_interval_sec(int sec);

void thread_katech_obd(void);
void exit_thread_katech_obd(void);

int katech_obd_mgr__timeserise_calc_init();


#endif

=======
#ifndef __SECO_OBD_MGR_H__
#define __SECO_OBD_MGR_H__

#include "seco_obd_1.h"

#define KATECH_OBD_TA1_INTERVAL_DEFAULT_SEC     1
#define KATECH_OBD_TA2_INTERVAL_DEFAULT_SEC     5

int katech_obd_mgr__get_ta1_obd_info(SECO_CMD_DATA_SRR_TA1_T* ta1_buff);
int katech_obd_mgr__get_ta2_obd_info(SECO_CMD_DATA_SRR_TA2_T* ta2_buff);

int katech_obd_mgr__set_ta1_interval_sec(int sec);
int katech_obd_mgr__set_ta2_interval_sec(int sec);

void thread_katech_obd(void);
void exit_thread_katech_obd(void);

int katech_obd_mgr__timeserise_calc_init();


#endif

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
