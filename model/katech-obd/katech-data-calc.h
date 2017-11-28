#ifndef __KATECH_TRIPDATA_H__
#define __KATECH_TRIPDATA_H__

#include "seco_obd_1.h"


#define TRIPDATA_STAT_KEYON         1
#define TRIPDATA_STAT_KEYOFF        2

#define TRIPDATA_RPM_ZERO_CHK_CNT   5

void init_tripdata();
void start_tripdata();
void end_tripdata();
void tmp_save_end_tripdata();



void calc_tripdata();

// --------------------------------------------------------
// trip data.
// --------------------------------------------------------
// 1) device ID : mdm_dev_id
int tripdata__init_dev_id();
int tripdata__get_dev_id();

// 2) vIN : mdm_char_vin
int tripdata__init_car_vin();
int tripdata__set_car_vin(char* car_vin);
int tripdata__get_car_vin(char* car_vin);

// 3) payload :  적재량 : tripdata_payload
int tripdata__init_pay_load();
int tripdata__set_pay_load(int payload);
int tripdata__get_pay_load();

// 4) total time : 개별트립의 주행시간 : tripdata_total_time
int tripdata__init_total_time_sec();
int tripdata__set_total_time_sec(int flag, int sec);
int tripdata__get_total_time_sec();
int tripdata__get_keystat_time_sec(int flag);
int tripdata__calc_total_time_sec();

// 5) driving time : tripdata_driving_time
int tripdata__init_driving_time_sec();
int tripdata__get_driving_time_sec();
int tripdata__calc_driving_time_sec(int speed);

// 6) stop time : tripdata_stop_time
int tripdata__init_stoptime_sec();
int tripdata__get_stoptime_sec();
int tripdata__calc_stoptime_sec(int speed);

// 7) driving distance : tripdata_driving_dist
int tripdata__init_driving_distance_m();
int tripdata__get_driving_distance_m();
int tripdata__set_driving_distance_m(int flag, int distance);
int tripdata__calc_driving_distance_m(int speed);

// 8) number of stop : tripdata_num_of_stop
int tripdata__init_stop_cnt();
int tripdata__get_stop_cnt();
int tripdata__calc_stop_cnt(int speed);

// 9) mean speed w/ stop : tripdata_mean_spd_w_stop
int tripdata__init_total_speed_avg();
int tripdata__get_total_speed_avg();

// 10) mean speed w/o stop : tripdata_mean_spd_wo_stop
int tripdata__init_run_speed_avg();
int tripdata__get_run_speed_avg();

// 11) acc rate : tripdata_acc_rate
int tripdata__init_accelation_rate();
int tripdata__get_accelation_rate();
int tripdata__calc_accelation_rate(float acceleration, int speed);

// 12) dec rate : tripdata_dec_rate
int tripdata__init_deaccelation_rate();
int tripdata__get_deaccelation_rate();
int tripdata__calc_deaccelation_rate(float acceleration, int speed);

// 13) cruise rate : tripdata_cruise_rate
int tripdata__init_cruise_rate();
int tripdata__calc_cruise_rate(float acceleration, int speed);
int tripdata__get_cruise_rate();

// 14) stop rate : tripdata_stop_rate
int tripdata__init_stop_rate();
int tripdata__get_stop_rate();

// 15) PKE : tripdata_pke
int tripdata__init_PKE();
int tripdata__get_PKE();
int tripdata__calc_PKE(int cur_speed);

// 16) PRA : tripdata_rpa
int tripdata__init_RPA();
int tripdata__get_RPA();
int tripdata__calc_RPA(float acceleration, int speed);

// 17) Mean acc : tripdata_mean_acc
int tripdata__init_acc_avg();
int tripdata__get_acc_avg();
int tripdata__calc_acc_avg(float acceleration, int speed);

// 18) cold rate : tripdata_cold_rate
int tripdata__init_cold_rate();
int tripdata__get_cold_rate();
int tripdata__calc_cold_rate(int cot);

// 19) warm : tripdata_warm
int tripdata__init_warm_rate();
int tripdata__get_warm_rate();
int tripdata__calc_warm_rate(int cot);

// 20) hot rate : tripdata_hot
int tripdata__init_hot_rate();
int tripdata__get_hot_rate();
int tripdata__calc_hot_rate(int cot);

// 21) fuel useage : tripdata_fuel_usage
int tripdata__init_fuel_useage();
int tripdata__set_fuel_useage(int flag, int fuel_useage);
int tripdata__get_fuel_useage();

// 22) fuel economy : tripdata_fuel_eco
int tripdata__init_fuel_economy();
int tripdata__get_fuel_economy();

// 23) trip start date : trip_start_date
int tripdata__init_start_date();
int tripdata__get_start_date();

// 24) trip start time : trip_start_time
int tripdata__init_start_time();
int tripdata__get_start_time();

// 25) trip end date : trip_end_date
int tripdata__init_end_date();
int tripdata__get_end_date();

// 26) trip end time : trip_end_time
int tripdata__init_end_time();
int tripdata__get_end_time();



// ----------------------------------------------------------------
#define FUEL_TYPE_DESEL     "DIS"
#define FUEL_TYPE_GASOLINE  "GAS"
#define FUEL_TYPE_LPG       "LPG"
#define FUEL_TYPE_HYB       "HYB"

int timeserise_calc__set_fuel_type(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
int timeserise_calc__set_ef_cal_type(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
// 79 Fuel flow rate;
int timeserise_calc__fuel_fr(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
// 80 Engine brake Torque;
int timeserise_calc__engine_break_torque(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
// 81 Engine brake Power;
int timeserise_calc__eng_break_pwr(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
// 82 Exhaus gas mass flowerate;
int timeserise_calc__exhaus_gas_mass_fr(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
//  83 Accessory power;
int timeserise_calc__accessory_power(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
// 87 Acceleration;
int timeserise_calc__acceleration(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
//  88 Corrected vehicle speed;
int timeserise_calc__corr_v_speed(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);
// 89 road gradient;
int timeserise_calc__garde(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff);




#endif

