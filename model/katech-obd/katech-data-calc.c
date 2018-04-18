
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <at/at_util.h>
#include <base/gpstool.h>

#include <katech-data-calc.h>
#include <time.h>
#include "board/modem-time.h"
#include "logd/logd_rpc.h"
#include "seco_obd_1.h"
#include "seco_obd_mgr.h"
#include "katech-packet.h"
#include <base/devel.h>

#define LOG_TARGET eSVC_COMMON

#define DEBUG_MSG_ACC_RATE
//#define OBD_DEBUG_MSG

#ifdef OBD_DEBUG_MSG
#define OBD_DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define OBD_DEBUG_PRINT(...) do{ } while ( 0 )
#endif


float tripdata__get_acceleration(float speed)
{
    static float speed_data[3] = {0,};
    static float calc_acceleration = 0;
    float tmp_accelation = 0;

    int i = 0;

    //OBD_DEBUG_PRINT(" set accelation calc : cur speed [%d]\r\n", speed);
    for ( i = 0 ; i < (3 -1) ; i ++)
    {
        speed_data[i] = speed_data[i+1];
    }
    
    speed_data[2] = speed;
/*
    for ( i = 0 ; i < 3 ; i ++)
        OBD_DEBUG_PRINT("speed [%d]->[%d]\r\n", i , speed_data[i]);
*/
    tmp_accelation =  (speed_data[2] - speed_data[0] ) / 2;

    //_g_calc_acceleration = tmp_accelation * 10;
    calc_acceleration =  tmp_accelation;
    return calc_acceleration;
}

/*
필요팩터
 * obd-SPD  // 1st cmd 04
 * obd-RPM  // 1st cmd 05
 * obd-EST  // 1st cmd 20
 * obd-COT  // 1st cmd 15

 * mdm-start-time
 * mdm-stop-time
*/

void init_tripdata()
{
    tripdata__init_dev_id();        // 1) device ID
    tripdata__init_car_vin();       // 2) vIN
    tripdata__init_pay_load();      // 3) payload :  적재량
    tripdata__init_total_time_sec();    // 4) total time : 개별트립의 주행시간
    tripdata__init_driving_time_sec();  // 5) driving time 
    tripdata__init_stoptime_sec();      // 6) stop time
    tripdata__init_driving_distance_km();    // 7) driving distance
    tripdata__init_stop_cnt();              // 8) number of stop
    tripdata__init_total_speed_avg();       // 9) mean speed w/ stop
    tripdata__init_run_speed_avg();         // 10) mean speed w/o stop
    tripdata__init_accelation_rate();       // 11) acc rate
    tripdata__init_deaccelation_rate();     // 12) dec rate
    tripdata__init_cruise_rate();   // 13) cruise rate
    tripdata__init_stop_rate();     // 14) stop rate
    tripdata__init_PKE();       // 15) PKE
    tripdata__init_RPA();       // 16) PRA
    tripdata__init_acc_avg();   // 17) Mean acc
    tripdata__init_cold_rate(); // 18) cold rate
    tripdata__init_warm_rate(); // 19) warm
    tripdata__init_hot_rate();  // 20) hot rate
    tripdata__init_fuel_useage();   // 21) fuel useage
    tripdata__init_fuel_economy();  // 22) fuel economy
    tripdata__init_start_date();   // 23) trip start date
    tripdata__init_start_time();   // 24) trip start time
    tripdata__init_end_date();   // 25) trip end date
    tripdata__init_end_time();   // 26) trip end time 
    
}

void dbg_print_trip_info()
{
    char tmp_str[512] = {0,};

    OBD_DEBUG_PRINT("-----------------------------------------------------\r\n");
    OBD_DEBUG_PRINT("< debug info trip end > \r\n");
    OBD_DEBUG_PRINT("  - 1) device ID : [%s]\r\n", "null");
    memset(tmp_str, 0x00, sizeof(tmp_str));
    tripdata__get_car_vin(tmp_str);
    OBD_DEBUG_PRINT("  - 2) vIN : [%s]\r\n", tmp_str);
    OBD_DEBUG_PRINT("  - 3) payload  : [%d]\r\n", tripdata__get_pay_load());
    OBD_DEBUG_PRINT("  - 4) total time : [%d]\r\n", tripdata__get_total_time_sec());
    OBD_DEBUG_PRINT("  - 5) driving time : [%d]\r\n", tripdata__get_driving_time_sec());
    OBD_DEBUG_PRINT("  - 6) stop time : [%d]\r\n", tripdata__get_stoptime_sec());
    OBD_DEBUG_PRINT("  - 7.1) driving distance : [%f]m\r\n", tripdata__get_driving_distance_km());
    OBD_DEBUG_PRINT("  - 7.2) driving distance : [%f]km\r\n", tripdata__get_driving_distance_km()/1000);
    OBD_DEBUG_PRINT("  - 8) number of stop : [%d]\r\n", tripdata__get_stop_cnt());
    OBD_DEBUG_PRINT("  - 9) mean speed w stop 1 [%d]/100 m/h\r\n", tripdata__get_total_speed_avg());
    OBD_DEBUG_PRINT("  - 10) mean speed w stop 2 [%d]/100 m/h\r\n", tripdata__get_run_speed_avg());
    OBD_DEBUG_PRINT("  - 11) acc rate [%d] /h\r\n", tripdata__get_accelation_rate());
    OBD_DEBUG_PRINT("  - 12) de acc rate [%d] /h\r\n", tripdata__get_deaccelation_rate());
    OBD_DEBUG_PRINT("  - 13) cruise rate [%d] /h\r\n", tripdata__get_cruise_rate());
    OBD_DEBUG_PRINT("  - 14) stop rate [%d] /h\r\n", tripdata__get_stop_rate());
    OBD_DEBUG_PRINT("  - 15) PKE [%d] /h\r\n", tripdata__get_PKE());
    OBD_DEBUG_PRINT("  - 16) RPA [%d] /h\r\n", tripdata__get_RPA());
    OBD_DEBUG_PRINT("  - 17) mean acc [%d]\r\n", tripdata__get_acc_avg());
    OBD_DEBUG_PRINT("  - 18) cold rate [%d]\r\n", tripdata__get_cold_rate());
    OBD_DEBUG_PRINT("  - 19) warm rate [%d]\r\n", tripdata__get_warm_rate());
    OBD_DEBUG_PRINT("  - 20) hot rate [%d]\r\n", tripdata__get_hot_rate());
    OBD_DEBUG_PRINT("  - 21) fuel usage [%d]\r\n", tripdata__get_fuel_useage());
    OBD_DEBUG_PRINT("  - 22) fuel econommy [%d]\r\n", tripdata__get_fuel_economy());
    OBD_DEBUG_PRINT("  - 23) trip start date :  [%d]\r\n", tripdata__get_start_date());
    OBD_DEBUG_PRINT("  - 24) trip start time  [%d]\r\n", tripdata__get_start_time());
    OBD_DEBUG_PRINT("  - 25) trip end date  [%d]\r\n", tripdata__get_end_date());
    OBD_DEBUG_PRINT("  - 26) trip end time [%d]\r\n", tripdata__get_end_time());
    OBD_DEBUG_PRINT("-----------------------------------------------------\r\n");
}


static int _g_tripdata_stat = 0;

static int tripdata_stat__set_stat(int stat)
{
    _g_tripdata_stat = stat;
    return _g_tripdata_stat;
}

static int tripdata_stat__get_stat()
{
    return _g_tripdata_stat;
}

// OBD+SRR+TDD? // km
void start_tripdata()
{
    float tmp_total_distance = tripdata__get_driving_distance_km();
    float tmp_fuel_usage = tripdata__get_fuel_useage();
    int tmp_cur_time = get_modem_time_utc_sec();

    LOGI(LOG_TARGET, "\t > START-TRIP : distance_m %f\r\n", tmp_total_distance);
    LOGI(LOG_TARGET, "\t > START-TRIP : fuel_useage %f\r\n", tmp_fuel_usage);
    LOGI(LOG_TARGET, "\t > START-TRIP : time_sec %d\r\n", tmp_cur_time);
    
    devel_webdm_send_log("start trip [%d][%u] => [%f][%f]\r\n",tmp_cur_time, mileage_get_m(), tmp_total_distance, tmp_fuel_usage);

    init_tripdata();

    tripdata_stat__set_stat(TRIPDATA_STAT_KEYON);

    dbg_print_trip_info();
}

void end_tripdata()
{
    float tmp_total_distance = tripdata__get_driving_distance_km();
    float tmp_fuel_usage = tripdata__get_fuel_useage();
    int tmp_cur_time = get_modem_time_utc_sec();
    
    LOGI(LOG_TARGET, "\t > END-TRIP : distance_m %f\r\n", tmp_total_distance);
    LOGI(LOG_TARGET, "\t > END-TRIP : fuel_useage %f\r\n", tmp_fuel_usage);
    LOGI(LOG_TARGET, "\t > END-TRIP : time_sec %d\r\n", tmp_cur_time);

    devel_webdm_send_log("end trip [%d][%u] => [%f][%f]\r\n",tmp_cur_time, mileage_get_m(), tmp_total_distance, tmp_fuel_usage);

    if ( TRIPDATA_STAT_KEYON != tripdata_stat__get_stat() )
        return;
    
    tripdata__set_total_time_sec(TRIPDATA_STAT_KEYOFF, tmp_cur_time);
    
    tripdata_stat__set_stat(TRIPDATA_STAT_KEYOFF);

    dbg_print_trip_info();
    katech_pkt_2_insert_and_send();
}

void tmp_save_end_tripdata()
{
    int tmp_cur_time = get_modem_time_utc_sec();
    tripdata__set_total_time_sec(TRIPDATA_STAT_KEYOFF, tmp_cur_time);
}

// 1초에 1번씩 불려야함.
void calc_tripdata()
{
    float accelation_val = 0;

    float factor_SPD = 0;
    float factor_COT = 0;

    // static int chk_rpm_zero_cnt = 0;
    // static int chk_rpm_zero_cnt2 = 0;

    if ( TRIPDATA_STAT_KEYON != tripdata_stat__get_stat() )
    {
        OBD_DEBUG_PRINT("invalid tripdata calc stat  [%d]\r\n", tripdata_stat__get_stat());
        return;
    }

    SECO_CMD_DATA_SRR_TA1_T ta1_buff_cur;
    SECO_CMD_DATA_SRR_TA2_T ta2_buff_cur;

    memset(&ta1_buff_cur, 0x00, sizeof(ta1_buff_cur));
    memset(&ta2_buff_cur, 0x00, sizeof(ta2_buff_cur));

    if ( katech_obd_mgr__get_ta1_obd_info(&ta1_buff_cur) <= 0)
    {
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
        return;
    }

    if ( katech_obd_mgr__get_ta2_obd_info(&ta2_buff_cur) <= 0)
    {
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
        return;
    }

    factor_SPD = ta1_buff_cur.obd_data[eOBD_CMD_SRR_TA1_SPD].data;
    if ( factor_SPD == -1 )
        factor_SPD = 0;
    
    factor_COT = ta1_buff_cur.obd_data[eOBD_CMD_SRR_TA1_COT].data;
    if ( factor_COT == -1 )
        factor_COT = 0;

    accelation_val = tripdata__get_acceleration(factor_SPD);
    //printf(" >>>>>>> accel [%f]  / spd [%f] \r\n",accelation_val, factor_SPD);

    // 1) device ID : mdm_dev_id
    // 2) vIN : mdm_char_vin
    // 3) payload :  적재량 : tripdata_payload
    tripdata__calc_total_time_sec(); // 4) total time : 개별트립의 주행시간 : tripdata_total_time
    tripdata__calc_driving_time_sec(factor_SPD);    // 5) driving time : tripdata_driving_time
    tripdata__calc_stoptime_sec(factor_SPD);        // 6) stop time : tripdata_stop_time
    tripdata__calc_driving_distance_km(factor_SPD);  // 7) driving distance : tripdata_driving_dist
    tripdata__calc_stop_cnt(factor_SPD);            // 8) number of stop : tripdata_num_of_stop
    // 9) mean speed w/ stop : tripdata_mean_spd_w_stop
    // 10) mean speed w/o stop : tripdata_mean_spd_wo_stop
    tripdata__calc_accelation_rate(accelation_val, factor_SPD);     // 11) acc rate : tripdata_acc_rate
    tripdata__calc_deaccelation_rate(accelation_val, factor_SPD);   // 12) dec rate : tripdata_dec_rate
    tripdata__calc_cruise_rate(accelation_val, factor_SPD);         // 13) cruise rate : tripdata_cruise_rate
    // 14) stop rate : tripdata_stop_rate
    tripdata__calc_PKE(factor_SPD);                     // 15) PKE : tripdata_pke
    tripdata__calc_RPA(accelation_val, factor_SPD);     // 16) PRA : tripdata_rpa
    tripdata__calc_acc_avg(accelation_val, factor_SPD); // 17) Mean acc : tripdata_mean_acc
    tripdata__calc_cold_rate(factor_COT);   // 18) cold rate : tripdata_cold_rate
    tripdata__calc_warm_rate(factor_COT);   // 19) warm : tripdata_warm
    tripdata__calc_hot_rate(factor_COT);    // 20) hot rate : tripdata_hot
    // 21) fuel useage : tripdata_fuel_usage
    tripdata__calc_fuel_usage(&ta1_buff_cur,&ta2_buff_cur);
    // 22) fuel economy : tripdata_fuel_eco
    // 23) trip start date : trip_start_date
    // 24) trip start time : trip_start_time
    // 25) trip end date : trip_end_date
    // 26) trip end time : trip_end_time

    // 시동꺼짐 체크..
    // obd 데이터를 갖고오지 못하고 그냥 꺼지는일이 발생할까봐 가끔저장
    /*
    if ( ta1_buff_cur.obd_data[eOBD_CMD_SRR_TA1_RPM].data == 0 )
    {
        chk_rpm_zero_cnt++;
    }

    if ( chk_rpm_zero_cnt > (TRIPDATA_RPM_ZERO_CHK_CNT*chk_rpm_zero_cnt2) )
    {
        tmp_save_end_tripdata();
        chk_rpm_zero_cnt = 0;
        chk_rpm_zero_cnt2++;
    }*/

}


// --------------------------------------------------
// 1) device ID
// --------------------------------------------------
int tripdata__init_dev_id()
{
    return 0;
}

int tripdata__get_dev_id()
{
    char phonenum[AT_LEN_PHONENUM_BUFF] = {0,};
	at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);
	
    //strcpy(dev_id, "01096267299");
    //strcpy(dev_id, phonenum);
	// 010-9626-7299
	
	return 0;
}

// --------------------------------------------------
// 2) vIN
// --------------------------------------------------
static char _g_tripdata__carvin[64]= {0,};
int tripdata__init_car_vin()
{
    //memset(_g_tripdata__carvin, 0x00, sizeof(_g_tripdata__carvin));
    return 0;
}

int tripdata__set_car_vin(char* car_vin)
{
    if ( car_vin == NULL ) 
        return -1;

    //tripdata__init_car_vin();
    memset(_g_tripdata__carvin, 0x00, sizeof(_g_tripdata__carvin));
    strcpy(_g_tripdata__carvin, car_vin);
    
    return 0;
}

int tripdata__get_car_vin(char* car_vin)
{
    if ( car_vin == NULL ) 
        return -1;

    strcpy(car_vin, _g_tripdata__carvin);

    return 0;
}

// --------------------------------------------------
// 3) payload :  적재량
// --------------------------------------------------
#define TRIPDATA__DEFAULT_PAYLOAD   65
static int _g_tripdata__pay_load = TRIPDATA__DEFAULT_PAYLOAD;
int tripdata__init_pay_load()
{
    _g_tripdata__pay_load = TRIPDATA__DEFAULT_PAYLOAD;
    return _g_tripdata__pay_load;
}

int tripdata__set_pay_load(int payload)
{
    _g_tripdata__pay_load = payload;
    return _g_tripdata__pay_load;

}

int tripdata__get_pay_load()
{
    return _g_tripdata__pay_load;    
}

// --------------------------------------------------
// 4) total time : 개별트립의 주행시간
// --------------------------------------------------
static int _g_triptotal_start_sec = 0;
static int _g_triptotal_end_sec = 0;
static int _g_tripdatal_total_time_calc_sec = 0;
int tripdata__init_total_time_sec()
{
    _g_triptotal_start_sec = 0;
    _g_triptotal_end_sec = 0;

    _g_tripdatal_total_time_calc_sec = 0;
    return 0;
}

int tripdata__set_total_time_sec(int flag, int sec)
{
    if ( sec <= 0 )
        return 0;

    if ( flag == TRIPDATA_STAT_KEYON ) 
        _g_triptotal_start_sec = sec;
    else
        _g_triptotal_end_sec = sec;

    return 0;
}

int tripdata__get_total_time_sec()
{
    // OBD_DEBUG_PRINT("----- get total time sec : end [%d] / start [%d] \r\n",  _g_triptotal_end_sec, _g_triptotal_start_sec);
    if ( _g_triptotal_end_sec <= _g_triptotal_start_sec )
        return 0;

    //return _g_triptotal_end_sec - _g_triptotal_start_sec;
    return _g_tripdatal_total_time_calc_sec;
}

int tripdata__get_keystat_time_sec(int flag)
{
    int ret_val = 0;

    if ( flag == TRIPDATA_STAT_KEYON ) 
        ret_val = _g_triptotal_start_sec;
    else
        ret_val = _g_triptotal_end_sec;
    
    return ret_val;
}

int tripdata__calc_total_time_sec()
{
    _g_tripdatal_total_time_calc_sec += 1;
    return _g_tripdatal_total_time_calc_sec;
}

// --------------------------------------------------
// 5) driving time 
// --------------------------------------------------
static int _g_drivingtime_sec = 0;
int tripdata__init_driving_time_sec()
{
    _g_drivingtime_sec = 0;
    return _g_drivingtime_sec;
}

int tripdata__get_driving_time_sec()
{
    return _g_drivingtime_sec;
}

int tripdata__calc_driving_time_sec(float speed)
{
    if (speed > 0)
        _g_drivingtime_sec++;

    return _g_drivingtime_sec;

}

// --------------------------------------------------
// 6) stop time
// --------------------------------------------------
static int _g_stoptime_sec = 0;
int tripdata__init_stoptime_sec()
{
    _g_stoptime_sec = 0;
    return _g_stoptime_sec;
}

int tripdata__get_stoptime_sec()
{
    return _g_stoptime_sec;
}

int tripdata__calc_stoptime_sec(float speed)
{
    if (speed == 0)
        _g_stoptime_sec ++;

    return _g_stoptime_sec;
}


// --------------------------------------------------
// 7) driving distance
// --------------------------------------------------
static float _g_total_drive_distance = 0;
float tripdata__init_driving_distance_km()
{
    _g_total_drive_distance = 0;
    return _g_total_drive_distance;
}

float tripdata__get_driving_distance_km()
{
    return _g_total_drive_distance;
}

float tripdata__calc_driving_distance_km(float cur_speed)
{
    static float speed_prev = 0;
    float speed_diff_calc = 0;

    float calc_factor_1 = cur_speed + speed_prev;
    float calc_factor_2 = 7200;

    speed_diff_calc = calc_factor_1 / calc_factor_2;
    
    _g_total_drive_distance = _g_total_drive_distance + speed_diff_calc;

    //printf("total distance : [%f][%f], [%f],[%f]\r\n", calc_factor_1, calc_factor_2, speed_diff_calc, _g_total_drive_distance);
    speed_prev = cur_speed;
    return _g_total_drive_distance;
}



// --------------------------------------------------
// 8) number of stop
// --------------------------------------------------
static int _g_tripdata__stop_cnt = 0;
int tripdata__init_stop_cnt()
{
    _g_tripdata__stop_cnt = 0;
    return _g_tripdata__stop_cnt;
}

int tripdata__get_stop_cnt()
{
    return _g_tripdata__stop_cnt;
}

int tripdata__calc_stop_cnt(float speed)
{
    static float speed_data[3] = {0,};

    int i = 0;

//    float tmp_accelation = 0;

    //OBD_DEBUG_PRINT(" set accelation calc : cur speed [%d]\r\n", speed);
    for ( i = 0 ; i < (3 -1) ; i ++)
    {
        speed_data[i] = speed_data[i+1];
    }
    
    speed_data[2] = speed;

    if ( ( speed_data[0] == 0 ) && ( speed_data[1] == 0 ) && ( speed_data[2] > 0 ) )
    {
        OBD_DEBUG_PRINT("increase stop count [%d]\r\n", _g_tripdata__stop_cnt);
        _g_tripdata__stop_cnt ++;
    }

    return _g_tripdata__stop_cnt;
}

// --------------------------------------------------
// 9) mean speed w/ stop
// --------------------------------------------------
int tripdata__init_total_speed_avg()
{
    // do nothing
    return 0;
}

// TODO : 형변환관련 추가
float tripdata__get_total_speed_avg()
{
    float total_distance = tripdata__get_driving_distance_km();
    int total_time_sec = tripdata__get_total_time_sec();

    //float speed_avg = total_distance / total_time_sec;
    float speed_avg = 0;

    float calc_factor_1 = total_distance;
    float calc_factor_2 = total_time_sec;
    float calc_factor_3 = 3600;

    if ( ( calc_factor_1 > 0 ) && (calc_factor_2 > 0) )
    {
        speed_avg = calc_factor_1 / calc_factor_2 / calc_factor_3;
    }
    else
    {
        speed_avg = 0;
        //devel_webdm_send_log("%s : %d => err [%d] [%d]\n", __func__, __LINE__, total_distance, total_time_sec);
    }
   // speed_avg = speed_avg * 100; // CHK: 180418 ok!!

    return speed_avg;
}

// --------------------------------------------------
// 10) mean speed w/o stop
// --------------------------------------------------
int tripdata__init_run_speed_avg()
{
    return 0;
}

float tripdata__get_run_speed_avg() // CHK: 180418 ok!!
{
    float total_distance = tripdata__get_driving_distance_km();
    int total_time_sec = tripdata__get_driving_time_sec();

    float speed_avg = 0;

    float calc_factor_1 = total_distance;
    float calc_factor_2 = total_time_sec; 
    float calc_factor_3 = 3600; // CHK: 180418 ok!!

    if ( calc_factor_2 > 0 )
    {
        speed_avg = calc_factor_1 / calc_factor_2 / calc_factor_3; // CHK: 180418 ok!!
    }
    else
    {
        speed_avg = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }
    
    // speed_avg = speed_avg * 100;

    return speed_avg;
}

// --------------------------------------------------
// 11) acc rate
// --------------------------------------------------
static int _g_acceleration_rate;
int tripdata__init_accelation_rate()
{
    _g_acceleration_rate = 0;
    return  _g_acceleration_rate;
}


int tripdata__get_accelation_rate()
{
    int total_time_sec = tripdata__get_total_time_sec();

    float total_rate = 0;
    float calc_factor_1 = _g_acceleration_rate;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        total_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        total_rate = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    OBD_DEBUG_PRINT("              --> tripdata__get_accelation_rate : [%d] / [%d] = [%f]\r\n", _g_acceleration_rate, total_time_sec, total_rate);
    total_rate = total_rate * 100;

    return total_rate;
}

int tripdata__calc_accelation_rate(float acceleration, float speed)
{
    if ( ( acceleration >= 0.139 ) && (speed > 0) ) // NOTE: 180418 fix
    {
        #ifdef DEBUG_MSG_ACC_RATE
        OBD_DEBUG_PRINT("increase acc rate [%d]\r\n", _g_acceleration_rate);
        #endif
        _g_acceleration_rate ++;
    }

    return  _g_acceleration_rate;
}

// --------------------------------------------------
// 12) dec rate
// --------------------------------------------------
static int _g_deacceleration_rate;

int tripdata__init_deaccelation_rate()
{
    _g_deacceleration_rate = 0;
    return _g_deacceleration_rate;
}

int tripdata__get_deaccelation_rate()
{
    int total_time_sec = tripdata__get_total_time_sec();

    float total_rate = 0;
    float calc_factor_1 = _g_deacceleration_rate;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        total_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        total_rate = 0;
        // devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    OBD_DEBUG_PRINT("              --> tripdata__get_deaccelation_rate : [%d] / [%d] = [%f]\r\n", _g_deacceleration_rate, total_time_sec, total_rate);
    total_rate = total_rate * 100;

    return total_rate;
}

int tripdata__calc_deaccelation_rate(float acceleration, float speed)
{
    if ( ( acceleration <= -0.139 ) && (speed > 0) ) // NOTE: 180418 0.5 -> 0.139
    {
        #ifdef DEBUG_MSG_ACC_RATE
        OBD_DEBUG_PRINT("increase acc de rate [%d]\r\n", _g_deacceleration_rate);
        #endif
        _g_deacceleration_rate ++;
    }

    return _g_deacceleration_rate;
}

// --------------------------------------------------
// 13) cruise rate
// --------------------------------------------------
static int _g_cruise_rate;
int tripdata__init_cruise_rate()
{
    _g_cruise_rate = 0;
    return _g_cruise_rate;
}

int tripdata__calc_cruise_rate(float acceleration, float speed)
{
    
    if ( ( acceleration > -0.139 ) && ( acceleration < 0.139 ) && (speed > 0) ) // NOTE: 180418 0.5 -> 0.139
    {
        #ifdef DEBUG_MSG_ACC_RATE
        OBD_DEBUG_PRINT("increase cruise rate [%d]\r\n", _g_cruise_rate);
        #endif
        _g_cruise_rate ++;
    }
    return _g_cruise_rate;
}

int tripdata__get_cruise_rate()
{
    int total_time_sec = tripdata__get_total_time_sec();

    float total_rate = 0;
    float calc_factor_1 = _g_cruise_rate;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        total_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        total_rate = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    OBD_DEBUG_PRINT("              --> tripdata__get_cruise_rate : [%d] / [%d] = [%f]\r\n", _g_cruise_rate, total_time_sec, total_rate);
    total_rate = total_rate * 100;

    return total_rate;
}
// --------------------------------------------------
// 14) stop rate
// --------------------------------------------------
int tripdata__init_stop_rate()
{
    return 0;
}

int tripdata__get_stop_rate()
{
    int total_time_sec = tripdata__get_total_time_sec();
    int stop_time_sec = tripdata__get_stoptime_sec();

    float stop_rate = 0;
    float calc_factor_1 = stop_time_sec;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        stop_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        stop_rate = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    OBD_DEBUG_PRINT("              --> tripdata__get_stop_rate : [%d] / [%d] = [%f]\r\n", stop_time_sec, total_time_sec, stop_rate);
    stop_rate = stop_rate * 100;

    return stop_rate;
}


// --------------------------------------------------
// 15) PKE
// --------------------------------------------------
static float _g_pke_value;
float tripdata__init_PKE()
{
    _g_pke_value = 0;
    return _g_pke_value;
}


float tripdata__get_PKE()  // CHK: 180418 ok!!
{
    float driving_distance = tripdata__get_driving_distance_km();

    float tmp_pke_val = 0;
    float calc_factor_1 = _g_pke_value;
    float calc_factor_2 = driving_distance;
    float calc_factor_3 = 1000;

    calc_factor_2 = calc_factor_2 * calc_factor_3;

    if ( calc_factor_2 > 0 )
    {
        tmp_pke_val = calc_factor_1 / calc_factor_2 ;
    }
    else
    {
        tmp_pke_val = 0;
        //devel_webdm_send_log("%s : %d => err [%f]\n", __func__, __LINE__, calc_factor_2);
    }

    OBD_DEBUG_PRINT("              --> tripdata__get_PKE is [%f]/[%f] -> [%f]\r\n", calc_factor_1, calc_factor_2, tmp_pke_val);

    // tmp_pke_val = tmp_pke_val * 1000;
    // tmp_pke_val = tmp_pke_val * 100;   // FIX: 180418 remove

    return tmp_pke_val;
}


float tripdata__calc_PKE(int cur_speed) // CHK: 180418 ok!!
{
    //static float speed_last = 0;
    //float cur_speed = cur_speed_i;
    static int speed_last = 0;

    if ( cur_speed > speed_last )
    {
        OBD_DEBUG_PRINT("1 >> cur_speed [%f] / speed_last [%f] / _g_pke_value [%f]\r\n",cur_speed, speed_last, _g_pke_value);
        _g_pke_value = _g_pke_value + ( cur_speed ^ 2 ) - (speed_last ^ 2);
    }
    else
    {
        OBD_DEBUG_PRINT("2 >> cur_speed [%f] / speed_last [%f] / _g_pke_value [%f]\r\n",cur_speed, speed_last, _g_pke_value);
    }

    speed_last = cur_speed;

    return _g_pke_value;
}

// --------------------------------------------------
// 16) PRA
// --------------------------------------------------
static float _g_rpa_value;
float tripdata__init_RPA()
{
    _g_rpa_value = 0;
    return _g_rpa_value;
}

float tripdata__get_RPA()
{
    float tmp_rpa = 0 ;
    float driving_distance = tripdata__get_driving_distance_km();

    float calc_factor_1 = _g_rpa_value;
    float calc_factor_2 = driving_distance;
    float calc_factor_3 = 1000;

    calc_factor_2 = calc_factor_2 * calc_factor_3;

    if ( calc_factor_2 > 0 )
    {
        tmp_rpa = calc_factor_1 / calc_factor_2;
    }
    else
    {
        tmp_rpa = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    OBD_DEBUG_PRINT("              --> tripdata__get_RPA is [%f]/[%f] -> [%f]\r\n", calc_factor_1, calc_factor_2, tmp_rpa);
    //tmp_rpa = tmp_rpa * 1000;
    // tmp_rpa = tmp_rpa * 100; // FIX : 180418 remove

    return tmp_rpa;
}

float tripdata__calc_RPA(float acceleration, float speed) // CHK: 180418 ok!!
{
    float tmp_speed = speed ;

    float tmp_rpa = 0;
    float calc_factor_2 = 3.6;

    if ( acceleration > 0 )
    {
        tmp_rpa = (acceleration * tmp_speed);
        tmp_rpa = tmp_rpa  / calc_factor_2;
    }

    _g_rpa_value += tmp_rpa;
    OBD_DEBUG_PRINT(" _g_rpa_value ::: [%f]\r\n", _g_rpa_value);
    return _g_rpa_value;
}



// --------------------------------------------------
// 17) Mean acc
// --------------------------------------------------
static int _g_mean_acc_over_cnt;
static int _g_mean_acc_total_cnt;
static float _g_mean_acc_total;

int tripdata__init_acc_avg()
{
    _g_mean_acc_over_cnt = 0;
    _g_mean_acc_total_cnt = 0;
    return 0;
}

int tripdata__get_acc_avg()
{
    float tmp_acc_avg = 0 ;

    float calc_factor_1 = _g_mean_acc_total;
    float calc_factor_2 = _g_mean_acc_total_cnt;

    if ( calc_factor_2 > 0 )
    {
        tmp_acc_avg = calc_factor_1 / calc_factor_2;
    }
    else
    {
        tmp_acc_avg = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    //tmp_acc_avg = tmp_acc_avg * 100; // FIX : 180418 remove

    // printf("tripdata__get_acc_avg :: [%f][%f] \r\n",tmp_acc_avg, tmp_acc_avg);
    return tmp_acc_avg;
}

int tripdata__calc_acc_avg(float acceleration, float speed)
{
    if ( ( acceleration > 0 ) && ( speed > 0 ) )
    {
        _g_mean_acc_total_cnt = _g_mean_acc_total_cnt + 1;
        _g_mean_acc_total += acceleration;
    }

    // printf("tripdata__get_acc_avg :: [%f][%f] ==> [%d]/[%f]\r\n" , acceleration, speed, _g_mean_acc_total_cnt, _g_mean_acc_total);

    return 0;
}



// --------------------------------------------------
// 18) cold rate
// --------------------------------------------------
static int _g_cot_rate;
int tripdata__init_cold_rate()
{
    _g_cot_rate = 0;
    return _g_cot_rate;
}

int tripdata__get_cold_rate()
{
    float tmp_cot_rate = 0;
    int total_time_sec = tripdata__get_total_time_sec();

    float calc_factor_1 = _g_cot_rate;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        tmp_cot_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        tmp_cot_rate = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    tmp_cot_rate = tmp_cot_rate*100;

    return tmp_cot_rate;
}

int tripdata__calc_cold_rate(float cot)
{
    if (cot <= 20)
        _g_cot_rate ++;

    return _g_cot_rate;
}


// --------------------------------------------------
// 19) warm
// --------------------------------------------------
static int _g_warm_rate;
int tripdata__init_warm_rate()
{
    _g_warm_rate = 0;
    return _g_warm_rate;
}


int tripdata__get_warm_rate()
{
    float tmp_warm_rate = 0;
    int total_time_sec = tripdata__get_total_time_sec();

    float calc_factor_1 = _g_warm_rate;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        tmp_warm_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        tmp_warm_rate = 0;
        // devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }

    tmp_warm_rate = tmp_warm_rate * 100;

    return tmp_warm_rate;
}


int tripdata__calc_warm_rate(float cot)
{
    if ((cot > 20) && (cot <= 60))
        _g_warm_rate ++;
    return _g_warm_rate;
}


// --------------------------------------------------
// 20) hot rate
// --------------------------------------------------
static int _g_hot_rate;
int tripdata__init_hot_rate()
{
    _g_hot_rate = 0;
    return _g_hot_rate;
}

int tripdata__get_hot_rate()
{
    float tmp_hot_rate = 0;
    int total_time_sec = tripdata__get_total_time_sec();

    float calc_factor_1 = _g_hot_rate;
    float calc_factor_2 = total_time_sec;

    if ( calc_factor_2 > 0 )
    {
        tmp_hot_rate = calc_factor_1 / calc_factor_2;
    }
    else
    {
        tmp_hot_rate = 0;
        //devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);
    }
    tmp_hot_rate = tmp_hot_rate * 100;

    return tmp_hot_rate;
}

int tripdata__calc_hot_rate(float cot)
{
    if (cot > 60)
        _g_hot_rate ++;
    return _g_hot_rate;
}

// --------------------------------------------------
// 21) fuel useage
// --------------------------------------------------
static float _g_trip_fuel_useage = 0;

// Trip.fuel_usage=Trip.fuel_usage+Post.Post.fuel_fr/3600;
int tripdata__init_fuel_useage()
{
    _g_trip_fuel_useage = 0;
    return 0;
}

/*
int tripdata__set_fuel_useage(int flag, int fuel_useage)
{
    if ( fuel_useage <= 0 )
        return 0;

    if ( flag == TRIPDATA_STAT_KEYON)
        _g_start_fuel_useage = fuel_useage;
    else
        _g_end_fuel_useage = fuel_useage;

    return 0;
}
*/
float tripdata__calc_fuel_usage(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float calc_factor_1 = timeserise_calc__fuel_fr(ta1_buff, ta2_buff);
    float calc_factor_2 = 3600;

    _g_trip_fuel_useage = _g_trip_fuel_useage + (calc_factor_1 / calc_factor_2);
    //printf("fuel useage [%f][%f][%f]\r\n", _g_trip_fuel_useage, calc_factor_1, calc_factor_2);
    return _g_trip_fuel_useage;
}

float tripdata__get_fuel_useage()
{
    return _g_trip_fuel_useage;
}




// --------------------------------------------------
// 22) fuel economy
// --------------------------------------------------
int tripdata__init_fuel_economy()
{
    return 0;
}

float tripdata__get_fuel_economy()
{
    float tmp_fuel_economy = 0;

    float distance = tripdata__get_driving_distance_km();
    float fuel_usage = tripdata__get_fuel_useage();

    float calc_factor_1 = distance;
    float calc_factor_2 = fuel_usage;


    if ( calc_factor_2 > 0 )
        tmp_fuel_economy = calc_factor_1 / calc_factor_2;
  
    //tmp_fuel_economy = tmp_fuel_economy * 100;

    return tmp_fuel_economy;
}

// --------------------------------------------------
// x) TBD : spare 1~18
// --------------------------------------------------


// --------------------------------------------------
// 23) trip start date
// --------------------------------------------------
int tripdata__init_start_date()
{
    return 0;
}

int tripdata__get_start_date()
{
    time_t time_sec = tripdata__get_keystat_time_sec(TRIPDATA_STAT_KEYON);
    int trip_date = 0;

    struct tm *lt;

    if((lt = localtime(&time_sec)) == NULL) {
        perror("get_modem_time_tm() call error");
        return 0;
    }

    trip_date = (lt->tm_year + 1900)*10000 + (lt->tm_mon+1)*100 + lt->tm_mday;

    return trip_date;
}

// --------------------------------------------------
// 24) trip start time
// --------------------------------------------------
int tripdata__init_start_time()
{
    return 0;
}

int tripdata__get_start_time()
{
    time_t time_sec = tripdata__get_keystat_time_sec(TRIPDATA_STAT_KEYON);
    int trip_time = 0;

    struct tm *lt;

    if((lt = localtime(&time_sec)) == NULL) {
        perror("get_modem_time_tm() call error");
        return 0;
    }
    //get_modem_time_tm();
    trip_time = ( lt->tm_hour*10000 + lt->tm_min*100 + lt->tm_sec ) * 100;

/*
	{
		char   time_str[26];	
		sprintf(time_str, "%d-%d-%d %d:%d:%d", lt->tm_year+1900,
						lt->tm_mon+1,
						lt->tm_mday,
						lt->tm_hour,
						lt->tm_min,
						lt->tm_sec);

		OBD_DEBUG_PRINT("%s\r\n",time_str);
    }
    */

    return trip_time;
}


// --------------------------------------------------
// 25) trip end date
// --------------------------------------------------
int tripdata__init_end_date()
{
    return 0;
}

int tripdata__get_end_date()
{
    time_t time_sec = tripdata__get_keystat_time_sec(TRIPDATA_STAT_KEYOFF);
    int trip_date = 0;

    struct tm *lt;

    if((lt = localtime(&time_sec)) == NULL) {
        perror("get_modem_time_tm() call error");
        return 0;
    }

    trip_date = (lt->tm_year + 1900)*10000 + (lt->tm_mon+1)*100 + lt->tm_mday;

    return trip_date;
}

// --------------------------------------------------
// 26) trip end time
// --------------------------------------------------
int tripdata__init_end_time()
{
    return 0;
}

int tripdata__get_end_time()
{
    time_t time_sec = tripdata__get_keystat_time_sec(TRIPDATA_STAT_KEYOFF);
    int trip_time = 0;

    struct tm *lt;

    if((lt = localtime(&time_sec)) == NULL) {
        perror("get_modem_time_tm() call error");
        return 0;
    }
    //get_modem_time_tm();
    trip_time = ( lt->tm_hour*10000 + lt->tm_min*100 + lt->tm_sec ) * 100;

/*
	{
		char   time_str[26];	
		sprintf(time_str, "%d-%d-%d %d:%d:%d", lt->tm_year+1900,
						lt->tm_mon+1,
						lt->tm_mday,
						lt->tm_hour,
						lt->tm_min,
						lt->tm_sec);

		OBD_DEBUG_PRINT("%s\r\n",time_str);
    }
    */

    return trip_time;
}




// ==========================================================
// 시계열후처리

static float _g_den_fuel = 0;
static float _g_lambda_st = 0;
int timeserise_calc__set_fuel_type(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    int max_retry = 3;
    int i = 0;
    char fuel_type[512] = {0,};

    float obd_data_MAF = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_MAF].data;
    for ( i = 0 ; i < max_retry ; i++ )
    {
        memset(fuel_type, 0x00, 512);
        if ( get_seco_obd_1_fueltype(fuel_type) == OBD_RET_SUCCESS )
        {
            OBD_DEBUG_PRINT("[ALLOC2 SENARIO] get obd fueltype : [%s]\r\n", fuel_type);
//            strcpy(g_seco_obd_info.fuel_type, fuel_type);
            break;
        }
        OBD_DEBUG_PRINT("[ALLOC2 SENARIO] get obd fueltype fail [%d]\r\n",i);
        sleep(1);
    }

    if ( strcmp(fuel_type, FUEL_TYPE_DESEL) == 0 )
    {
        _g_den_fuel=0.840;
        _g_lambda_st=14.5;
    }
    else if ( strcmp(fuel_type, FUEL_TYPE_GASOLINE) == 0 )
    {
        _g_den_fuel=0.739;
        _g_lambda_st=14.6;
    }
    else if ( strcmp(fuel_type, FUEL_TYPE_LPG) == 0 )
    {
        _g_den_fuel=0.584;
        _g_lambda_st=15.4;
    }
    else
    {
        _g_den_fuel=0.739;                /*연비 계산 방법 Case 설정용 변수, 연료 밀도 */
        _g_lambda_st=14.6;                /*연비 계산 방법 Case 설정용 변수, 연료 이론 공연비 */
    }

    devel_webdm_send_log("ts calc init :: _g_den_fuel [%f] / _g_lambda_st [%f] / MAF [%f] ", _g_den_fuel, _g_lambda_st, obd_data_MAF);
    return 0;
}


/*
차량에서 취득 가능한 데이터 중류에 따라 연비 데이터 계산 방법이 달라지며, 이에 따라 4가지 Case로 분류하며, 하기의 순서대로 CASE를 분류 (CASE1을 만족하고, CASE2를 만족할 경우 CASE1으로 선정)
CASE 1 : EFR 지원 차량 
CASE 2 : BS1, MAF 지원차량
CASE 3 : MAF, CLV 지원되는 가솔린 차량
CASE 4 : 그외 차량, 연료량 산출 안함
각 CASE에 대해 '_g_car_ef_cal_type'을 변수로 사용
1회만 설정하면 이후에는 차량이 변경되기 전에는 진행하지 않아도 됨 
*/
static int _g_car_ef_cal_type = 4;
int timeserise_calc__set_ef_cal_type(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float obd_data_EFR = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_EFR].data;
    float obd_data_BS1 = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_BS1].data;
    float obd_data_MAF = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_MAF].data;
    float obd_data_CLV = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_CLV].data;

    if ( obd_data_EFR >= 0 )
        _g_car_ef_cal_type = 1;
    else if ( ( obd_data_BS1 >= 0 ) && ( obd_data_MAF >= 0 ))
        _g_car_ef_cal_type = 2;
    else if ( ( obd_data_MAF >= 0 ) && ( obd_data_CLV >= 0 ))
        _g_car_ef_cal_type = 3;
    else   
        _g_car_ef_cal_type = 4;

    return _g_car_ef_cal_type;
}

// 79 Fuel flow rate
/*
항목 : Post.fuel_fr
내용 : 일반data 중 시계열 후처리 항목인 79 Fuel flow rate, 1초 간격으로 후처리 된 연료 유량을 의미함
Unit : L/hr
*/
float timeserise_calc__fuel_fr(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float fuel_fr = 0;
    
    float obd_data_EFR = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_EFR].data;
    float obd_data_MAF = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_MAF].data;
    float obd_data_BS1 = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_BS1].data;
    float obd_data_CLV = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_CLV].data;

    switch (_g_car_ef_cal_type) {
        case 1:
        {
            fuel_fr = obd_data_EFR;
            break;
        }
        case 2:
        {
            if (( obd_data_BS1 > 0 ) && (_g_lambda_st > 0) && (_g_den_fuel > 0))
            {
                fuel_fr = ((obd_data_MAF/obd_data_BS1*_g_lambda_st)*3.6)/_g_den_fuel;
            }
            break;
        }
        case 3:
        {
            if (obd_data_CLV < 90) 
            {
                if ( ( _g_lambda_st > 0 ) && ( _g_den_fuel > 0 ) )
                    fuel_fr = ((obd_data_MAF/_g_lambda_st)*3.6)/_g_den_fuel;
            }
            else
            {
                if ( ( _g_lambda_st > 0 ) && ( _g_den_fuel > 0 ))
                    fuel_fr = ((obd_data_MAF/(_g_lambda_st*0.85))*3.6)/_g_den_fuel;
            }
            break;
        }
        case 4:
        {
            fuel_fr=0;
            break;
        }
        
    }
    //printf("%s() -> _g_lambda_st [%f], _g_den_fuel [%f]\r\n", __func__,  _g_lambda_st, _g_den_fuel);
    //printf("%s() -> fuel type : [%d] / obd_data_EFR [%f], obd_data_MAF [%f], obd_data_BS1 [%f], obd_data_CLV [%f]\r\n", __func__, _g_car_ef_cal_type, obd_data_EFR, obd_data_MAF, obd_data_BS1, obd_data_CLV);
    //printf("%s() -> ret : [%f]\r\n", __func__, fuel_fr);

    // 변환...
    //fuel_fr = fuel_fr*100;
    return fuel_fr;
}



// 80 Engine brake Torque
int timeserise_calc__engine_break_torque(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float eng_trq = 0;

    float obd_data_EAT = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_EAT].data;
    float obd_data_ERT = ta2_buff->obd_data[eOBD_CMD_SRR_TA2_ERT].data ;
    float calc_factor_3 = 8;
    float calc_factor_4 = 0.01;

    if ( ( obd_data_EAT >= 0 ) && ( obd_data_ERT >= 0 ))
    {
        eng_trq = obd_data_ERT * (obd_data_EAT - calc_factor_3) * calc_factor_4;
    }
    else 
    {
        eng_trq = 0;
    }
    /*
if (ERT(i)<>'X' and EAT(i)<>'X'){
    Post.eng_trq(i)=ERT(i)*(EAT(i)-8)*0.01;
    else{
    Post.eng_trq(i)=0	
    }
    }*/
    OBD_DEBUG_PRINT("%s() -> fuel type : [%d] / obd_data_EAT [%d], obd_data_ERT [%d]\r\n", __func__, _g_car_ef_cal_type, obd_data_EAT, obd_data_ERT);
    OBD_DEBUG_PRINT("%s() -> ret : [%f], [%d]\r\n", __func__, eng_trq,  (int)eng_trq);
    // 변환...
    eng_trq = eng_trq * 100.0;
    return (int)eng_trq;
}


// 81 Engine brake Power
int timeserise_calc__eng_break_pwr(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float eng_pwr = 0;
    float eng_trq = timeserise_calc__engine_break_torque(ta1_buff, ta2_buff);;
    float obd_data_RPM = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_RPM].data;
    
    
    eng_pwr = 2.0 * 3.141592 * obd_data_RPM * eng_trq / 60000.0;

    OBD_DEBUG_PRINT("%s() -> [%f], [%d]\r\n", __func__, eng_pwr, (int)eng_pwr);

    eng_pwr = eng_pwr * 100.0;
    return (int)eng_pwr;
}


// 82 Exhaus gas mass flowerate
int timeserise_calc__exhaus_gas_mass_fr(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float exh_fr = 0 ;

    float obd_data_EFR = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_EFR].data;
    float obd_data_MAF = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_MAF].data;

    switch (_g_car_ef_cal_type) {
        case 1:
        {
            exh_fr = obd_data_MAF * 3.6 + obd_data_EFR * _g_den_fuel;
            break;
        }
        case 2:
        {
            exh_fr = obd_data_MAF * 3.6 + obd_data_EFR * _g_den_fuel;
            break;
        }
        case 3:
        {
            exh_fr = obd_data_MAF * 3.6 + obd_data_EFR * _g_den_fuel;
            break;
        }
        case 4:
        {
            exh_fr=0;
            break;
        }
        default:
            exh_fr=0;
    }

    OBD_DEBUG_PRINT("%s() -> [%f], [%d]\r\n", __func__, exh_fr, (int)exh_fr);

    // exh_fr = exh_fr * 10.0; // NOTE: 180418 remove
    return (int)exh_fr;
}


//  83 Accessory power
int timeserise_calc__accessory_power(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    //Post.Aux_pwr(i)=0;

    //Post.eng_spare1(i)=0;
    //Post.eng_spare2(i)=0;
    //Post.eng_spare3(i)=0;
    float acc_power = 0;
    OBD_DEBUG_PRINT("%s() -> [%f], [%d]\r\n", __func__, acc_power, (int)acc_power);

    acc_power = acc_power * 100.0;
    return (int)acc_power;
}

// 87 Acceleration // CHK: 180418 ok!!
int timeserise_calc__acceleration(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    //Post.acc(i)=(Post.VS_corr(i+1)-Post.VS_corr(i-1))/7.2;
    static float speed_data[3] = {0,};

    float factor_SPD = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_SPD].data;
    float tmp_accelation = 0;

    int i = 0;

    //OBD_DEBUG_PRINT(" set accelation calc : cur speed [%d]\r\n", speed);
    for ( i = 0 ; i < (3 -1) ; i ++)
    {
        speed_data[i] = speed_data[i+1];
    }
    
    speed_data[2] = factor_SPD;


    tmp_accelation =  (speed_data[2] - speed_data[0] ) / 7.2;

    OBD_DEBUG_PRINT("%s() -> [%f], [%d]\r\n", __func__, tmp_accelation, (int)tmp_accelation);

    // tmp_accelation = tmp_accelation * 500.0;  // NOTE: 180418
    return (int)tmp_accelation;
}

//  88 Corrected vehicle speed // CHK: 180418 ok!!
int timeserise_calc__corr_v_speed(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    float veh_speed = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_SPD].data;

    OBD_DEBUG_PRINT("%s() -> [%f], [%d]\r\n", __func__, veh_speed, (int)veh_speed);

    //veh_speed = veh_speed * 100.0; // NOTE: 180418 remove 
    return (int)veh_speed;
}



// 89 road gradient
int timeserise_calc__garde(SECO_CMD_DATA_SRR_TA1_T* ta1_buff, SECO_CMD_DATA_SRR_TA2_T* ta2_buff)
{
    gpsData_t cur_gpsdata = {0,};

    static float speed_data[3] = {0,};
    static float gps_altitude[3]  = {0,};
    
    float factor_SPD = 0;
    float facotor_gps_altitude = 0;

    float tmp_accelation = 0;
//    float tmp_altitude = 0;

    float tmp_garde = 0;

    int i = 0;

    gps_get_curr_data(&cur_gpsdata);

    factor_SPD = ta1_buff->obd_data[eOBD_CMD_SRR_TA1_SPD].data;
    facotor_gps_altitude = cur_gpsdata.altitude;

    //OBD_DEBUG_PRINT(" set accelation calc : cur speed [%d]\r\n", speed);
    for ( i = 0 ; i < (3 -1) ; i ++)
    {
        speed_data[i] = speed_data[i+1];
        gps_altitude[i] = gps_altitude[i+1];
    }
    
    speed_data[2] = factor_SPD;
    gps_altitude[2] = facotor_gps_altitude;

    tmp_accelation =  (speed_data[2] - speed_data[0] ) / 7.2;
    

    if ( (speed_data[1] + speed_data[0] + 2*speed_data[2]) > 0)
        tmp_garde = (gps_altitude[1] - gps_altitude[0]) / ((speed_data[1] + speed_data[0] + 2*speed_data[2])/7.2)*100;
    //else
    //    devel_webdm_send_log("%s : %d => err \n", __func__, __LINE__);

    //Post.grad(i)=(gps_altitude(i+1)-gps_altitude(i-1)) / ((Post.VS_corr(i+1)+Post.VS_corr(i-1)+2*Post.VS_corr(i))/7.2)*100;

    OBD_DEBUG_PRINT("%s() -> [%f], [%d]\r\n", __func__, tmp_garde, (int)tmp_garde);

    return (int)tmp_garde;
}


void timeserise_calc__init()
{
    SECO_CMD_DATA_SRR_TA1_T ta1_buff_cur;
    SECO_CMD_DATA_SRR_TA2_T ta2_buff_cur;

    int ret_ta1 = 0;
    int ret_ta2 = 0;

    int max_wait_init = 30;

    memset(&ta1_buff_cur, 0x00, sizeof(ta1_buff_cur));
    memset(&ta2_buff_cur, 0x00, sizeof(ta2_buff_cur));

    while(max_wait_init--)
    {
        ret_ta1 = katech_obd_mgr__get_ta1_obd_info(&ta1_buff_cur);
        ret_ta2 = katech_obd_mgr__get_ta2_obd_info(&ta2_buff_cur);

        LOGE(LOG_TARGET, "\t > timeserise_calc__init => wait for init... [%d], [%d], [%d]\r\n",max_wait_init,ret_ta1,ret_ta2);

        if ( ( ret_ta1 > 0 ) && (ret_ta2 > 0) )
            break;
        
        
        sleep(1);
    }
    

    timeserise_calc__set_fuel_type(&ta1_buff_cur, &ta2_buff_cur);
    timeserise_calc__set_ef_cal_type(&ta1_buff_cur, &ta2_buff_cur);
}

int get_running_time_sec()
{
#if 0
    static int boot_time = 0;
    int tmp_cur_time = get_modem_time_utc_sec();
    int tmp_ret_val = 0;

    if ( boot_time == 0 )
        boot_time = tmp_cur_time;

    tmp_ret_val = tmp_cur_time - boot_time;
    printf("get_running_time_sec() => [%d]\r\n", tmp_ret_val);
    return tmp_ret_val;
#endif
    static int running_time = 0;
    return running_time++;
}

static int _dev_boot_time = 0;


static int _get_dev_boot_time()
{
    char read_buff[128] = {0,};
    int read_cnt = 0;

    int ret_val = 0;

    read_cnt = mds_api_read_data(TRIPDATA_DEV_BOOT_CNT_PATH, (void*)read_buff, sizeof(read_buff));
    if ( read_cnt > 0 )
        ret_val = atoi(read_buff);

    LOGT(eSVC_COMMON,"get dev boot cnt cnt [%d]\r\n", ret_val);

    return ret_val;
}

static int _set_dev_boot_time(int cnt)
{
    char write_buff[128] = {0,};
    int write_cnt = 0;

    write_cnt = sprintf(write_buff,"%d", cnt);

    mds_api_write_data(TRIPDATA_DEV_BOOT_CNT_PATH, (void*)write_buff, write_cnt, 0);

    LOGT(eSVC_COMMON,"set dev boot cnt cnt  [%d]\r\n", cnt);

    return cnt;
}

int init_dev_boot_time()
{
    int boot_cnt = _get_dev_boot_time();
    boot_cnt = boot_cnt + 1;
    _set_dev_boot_time(boot_cnt);
    _dev_boot_time = boot_cnt;
    return _dev_boot_time;
}

int get_dev_boot_time()
{
    return _dev_boot_time;
}


