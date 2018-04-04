#ifndef __KATECH_PACKET_H__
#define __KATECH_PACKET_H__

#include <base/gpstool.h>
#include "seco_obd_1.h"

#define DEFAULT_REPORT_DATA_CNT     60

#define KATECH_PKT_RET_SUCCESS          0
#define KATECH_PKT_RET_FAIL             -1
#define KATECH_PKT_RET_FAIL_NO_AUTH     -2
#define KATECH_PKT_RET_FAIL_CANNOT_MALLOC       -2

#define KATECH_PKT_LAST_CHAR    '>'
#define KATECH_PKT_START_CHAR    '<'

#define KATECH_PKT_INTERVAL_SEND    0
#define KATECH_PKT_IMMEDIATELY_SEND 1

#define KATECH_PKT_VER_NUM          2

typedef enum {
    UNSIGNED_1_BYTE,
    SIGNED_1_BYTE,
    UNSIGNED_2_BYTE,
    SIGNED_2_BYTE,
    UNSIGNED_4_BYTE,
    SIGNED_4_BYTE,
}e_PKT_FIELD_CONVERT_TYPE;

typedef enum {
    KATECH_PKT_ID_AUTH,
    KATECH_PKT_ID_FW_CHK,
    KATECH_PKT_ID_REPORT_1,
    KATECH_PKT_ID_REPORT_2,
} e_KATECH_PKT_ID;


typedef enum {
    KATECH_SVR_STAT_NONE,
    KATECH_SVR_STAT_AUTH_RUNNING,
    KATECH_SVR_STAT_AUTH_FAIL,
    KATECH_SVR_STAT_AUTH_SUCCESS,
} e_KATECH_SVR_STAT;


typedef struct {
    int svr_stat;
    char auth_key[16];
    char report_ip[16];
    int report_port;
}KATCH_PKT_STAT;


// ------------------------------------------
// common ::: header 
// ------------------------------------------
typedef struct {
    unsigned char  pkt_start;   
    unsigned char  pkt_num; 
    unsigned char  protocol_ver;
    char  auth_key[16];
    char  dev_num[16];
}__attribute__((packed))KATCH_PKT_HEADER;

// ------------------------------------------
// common ::: tail 
// ------------------------------------------
typedef struct {
    unsigned char  body_chksum; 
    unsigned char  pkt_end; 
}__attribute__((packed))KATCH_PKT_TAIL;


// ------------------------------------------
// body : auth.
// ------------------------------------------
typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    unsigned char  auth_body_content[100];  // TBD
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_AUTH_REQ;

typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    unsigned short resp_code;
    unsigned char  resp_msg_1[30];
    unsigned char  resp_msg_2[30];
    unsigned char  encrypt_key[16];
    unsigned char  server_ip[15];
    unsigned char  server_port[5];
    unsigned char   reserved[2];
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_AUTH_RESP;

// ------------------------------------------
// body : firmware update.
// ------------------------------------------
typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    unsigned char  obd_version;
    unsigned char  fw_body_content[99]; // TBD
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_FW_REQ;

typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    unsigned short resp_code;
    unsigned char  fw_path_1[30];
    unsigned char  fw_path_2[30];
    unsigned char  need_to_update;
    unsigned char  curversion;
    unsigned char  fw_dl_ip[15];
    unsigned char  fw_dl_port[5];
    unsigned char  fw_file_name[16];
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_FW_RESP;

// ------------------------------------------
// body : report data 1 .
// ------------------------------------------

typedef struct {
    unsigned int    mdm_date;                   // 7    // 4        // v
    unsigned int    mdm_time;                   // 8    // 4        // v
    unsigned char   obd_trip_num[3];            // 9    // 3        // v
    unsigned char   obd_trip_elapsed[3];        // 10   // 3        // v
    unsigned char   mdm_gps_num_of_sat;         // 11   // 1        // v
    unsigned int    mdm_gps_time;               // 12   // 4        // v
    unsigned int    mdm_gps_long;               // 13   // 4        // v
    unsigned int    mdm_gps_lat;                // 14   // 4        // v
    unsigned short  mdm_gps_attitude;           // 15   // 2        // v
    unsigned short  mdm_gps_heading;            // 16   // 2        // v
    unsigned short  mdm_gps_speed;              // 17   // 2        // v
    unsigned char   obd_basic_spare_1;          // 18   // 1        // v
    unsigned char   obd_basic_spare_2;          // 19   // 1        // v
    unsigned char   obd_basic_spare_3;          // 20   // 1        // v
    unsigned char   obd_basic_spare_4;          // 21   // 1        // v
    unsigned char   obd_basic_spare_5;          // 22   // 1        // v
    unsigned short  obd_basic_spare_6;          // 23   // 2        // v
    unsigned short  obd_basic_spare_7;          // 24   // 2        // v
    unsigned short  obd_basic_spare_8;          // 25   // 2        // v
    unsigned short  obd_basic_spare_9;          // 26   // 2        // v
    unsigned short  obd_basic_spare_10;         // 27   // 2        // v
    unsigned short  obd_basic_spare_11;         // 28   // 2        // v
    unsigned short  obd_basic_spare_12;         // 29   // 2        // v
    unsigned short  obd_basic_spare_13;         // 30   // 2        // v
    unsigned short  obd_basic_spare_14;         // 31   // 2        // v
    unsigned short  obd_basic_spare_15;         // 32   // 2        // v
    unsigned short  obd_basic_spare_16;         // 33   // 2        // v
    unsigned int    obd_basic_spare_17;         // 34   // 4        // v
    unsigned int    obd_basic_spare_18;         // 35   // 4        // v
    unsigned int    obd_basic_spare_19;         // 36   // 4        // v
    unsigned int    obd_basic_spare_20;         // 37   // 4        // v
    unsigned char   obd_sd_card_capacity;       // 38   // 1        // v
    unsigned char   obd_err_code;               // 39   // 1        // v
    unsigned short  obd_spare;                  // 40   // 2        // v
    unsigned char   obd_calc_load_val;          // 41   // 1        // v
    unsigned char   obd_intake_map;             // 42   // 1        // v
    unsigned short  obd_rpm;                    // 43   // 2        // v
    unsigned char   obd_speed;                  // 44   // 1        // v
    unsigned short  obd_air_flow_rate;          // 45   // 2        // v
    unsigned char   obd_abs_throttle_posi;      // 46   // 1        // v
    unsigned short  obd_lambda;                 // 47   // 2        // v
    unsigned short  obd_ctrl_module_vol;        // 48   // 2        // v
    unsigned char   obd_acc_pedal_posi;         // 49   // 1        // v
    unsigned short  obd_engine_fule_rate;       // 50   // 2        // v
    unsigned char   obd_actual_engine;          // 51   // 1        // v
    unsigned char   obd_command_egr;            // 52   // 1        // v
    unsigned char   obd_actual_egr_duty;        // 53   // 1        // v
    unsigned char   obd_engine_friction;        // 54   // 1        // v
    unsigned char   obd_engine_coolant_temp;    // 55   // 1        // v
    unsigned char   obd_intake_air_temp;        // 56   // 1        // v
    unsigned short  obd_catalyst_temp_1;        // 57   // 2        // v
    unsigned short  obd_time_engine_start;      // 58   // 2        // v
    unsigned char   obd_barometic_press;        // 59   // 1        // v
    unsigned char   obd_ambient_air_temp;       // 60   // 1        // v
    unsigned short  obd_ref_torque;             // 61   // 2        // v
    unsigned int    obd_mon_st_since_dtc_clr;   // 62   // 4        // v
    unsigned short  obd_dist_travled_mil;       // 63   // 2        // v
    unsigned short  obd_dist_travled_dtc;       // 64   // 2        // v
    unsigned char   obd_spare_1_egr_cmd2;        // 65   // 1        // v
    unsigned char   obd_spare_2_egr_err;        // 66   // 1        // v
    unsigned char   obd_spare_3;                // 67   // 1        // v
    unsigned char   obd_spare_4;                // 68   // 1        // v
    unsigned char   obd_spare_5;                // 69   // 1        // v
    unsigned short  obd_spare_6;                // 70   // 2        // v
    unsigned short  obd_spare_7;                // 71   // 2        // v
    unsigned short  obd_spare_8;                // 72   // 2        // v
    unsigned short  obd_spare_9;                // 73   // 2        // v
    unsigned short  obd_spare_10;               // 74   // 2        // v
    unsigned short  obd_spare_11;               // 75   // 2        // v
    unsigned short  obd_spare_12;               // 76   // 2        // v
    unsigned int    obd_spare_13;               // 77   // 4        // v
    unsigned int    obd_spare_14;               // 78   // 4        // v
    unsigned short  obd_fuel_flow_rate;         // 79   // 2        // v
    unsigned short  obd_engine_brake_torq;      // 80   // 2        // v
    unsigned short  obd_engine_brake_pwr;       // 81   // 2        // v
    unsigned short  obd_exhaust_gas_flowrate;   // 82   // 2        // v
    unsigned short  obd_acc_power;              // 83   // 2        // v
    unsigned short  obd_engine_spare_1;         // 84   // 2        // v
    unsigned short  obd_engine_spare_2;         // 85   // 2        // v
    unsigned short  obd_engine_spare_3;         // 86   // 2        // v
    unsigned char   obd_acceleration;           // 87   // 1        // v
    unsigned short  obd_cor_speed;              // 88   // 2        // v
    unsigned char   obd_road_gradient;          // 89   // 1        // v
    unsigned short  obd_driving_spare_1;        // 90   // 2        // v
    unsigned short  obd_driving_spare_2;        // 91   // 2        // v
    unsigned short  obd_driving_spare_3;        // 92   // 2        // v
    unsigned short  obd_after_spare_1;          // 93   // 2        // v
    unsigned short  obd_after_spare_2;          // 94   // 2        // v
    unsigned short  obd_after_spare_3;          // 95   // 2        // v
    unsigned short  obd_after_spare_4;          // 96   // 2        // v
    unsigned short  obd_after_spare_5;          // 97   // 2        // v
    unsigned int    obd_after_spare_6;          // 98   // 4        // v
    unsigned int    obd_after_spare_7;          // 99   // 4        // v
    unsigned int    obd_after_spare_8;          // 100  // 4        // v
    unsigned int    obd_after_spare_9;          // 101  // 4        // v
    unsigned int    obd_after_spare_10;         // 102  // 4        // v
}__attribute__((packed))REPORT_DATA_1_BODY_DATA; // total 200 byte

/*
typedef struct {
    unsigned char  obd_raw_data[200]; // REPORT_DATA_1_BODY_DATA
}__attribute__((packed))REPORT_DATA_1_BODY;
*/
/*
// 패킷의 크기가 가변이다.
// body count 의 크기에 따라 전체 패킷의 크기가 달라지기 때문에 static 하게 정의하지 않는다.
typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    //REPORT_DATA_1_BODY body; // body 갯수 변화가능
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_REPORT_DATA_1_REQ;
*/

typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    unsigned short resp_code;
    unsigned char  resp_msg_1[30];
    unsigned char  resp_msg_2[30];
    unsigned char  spare_1[4];
    unsigned char  spare_2[4];
    unsigned char  spare_3[10];
    unsigned char  spare_4[10];
    unsigned char  spare_5[10];
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_REPORT_DATA_1_RESP;

// ------------------------------------------
// body : trip data 2 .
// ------------------------------------------

typedef struct {
    unsigned short  mdm_dev_id;                 // 7      // 2
    unsigned char   mdm_char_vin[17];           // 8      // 17
    unsigned char   tripdata_payload;           // 9      // 1
    unsigned short  tripdata_total_time;         // 10     // 2
    unsigned short  tripdata_driving_time;       // 11     // 2
    unsigned short  tripdata_stop_time;          // 12     // 2
    unsigned short  tripdata_driving_dist;       // 13     // 2
    unsigned short  tripdata_num_of_stop;        // 14     // 2
    unsigned short  tripdata_mean_spd_w_stop;    // 15     // 2
    unsigned short  tripdata_mean_spd_wo_stop;   // 16     // 2
    unsigned char   tripdata_acc_rate;           // 17     // 1
    unsigned char   tripdata_dec_rate;           // 18     // 1
    unsigned char   tripdata_cruise_rate;        // 19     // 1
    unsigned char   tripdata_stop_rate;          // 20     // 1
    unsigned char   tripdata_pke;                // 21     // 1
    unsigned char   tripdata_rpa;                // 22     // 1
    unsigned char   tripdata_mean_acc;           // 23     // 1
    unsigned char   tripdata_cold_rate;          // 24     // 1
    unsigned char   tripdata_warm;               // 25     // 1
    unsigned char   tripdata_hot;                // 26     // 1
    unsigned short  tripdata_fuel_usage;         // 27     // 2
    unsigned short  tripdata_fuel_eco;           // 28     // 2
    unsigned char   tripdata_trip_spare_1;       // 29     // 1
    unsigned char   tripdata_trip_spare_2;       // 30     // 1
    unsigned char   tripdata_trip_spare_3;       // 31     // 1
    unsigned char   tripdata_trip_spare_4;       // 32     // 1
    unsigned char   tripdata_trip_spare_5;       // 33     // 1
    unsigned char   tripdata_trip_spare_6;       // 34     // 1
    unsigned char   tripdata_trip_spare_7;       // 35     // 1
    unsigned char   tripdata_trip_spare_8;       // 36     // 1
    unsigned short  tripdata_trip_spare_9;       // 37     // 2
    unsigned short  tripdata_trip_spare_10;      // 38     // 2
    unsigned short  tripdata_trip_spare_11;      // 39     // 2
    unsigned short  tripdata_trip_spare_12;      // 40     // 2
    unsigned short  tripdata_trip_spare_13;      // 41     // 2
    unsigned short  tripdata_trip_spare_14;      // 42     // 2
    unsigned short  tripdata_trip_spare_15;      // 43     // 2
    unsigned short  tripdata_trip_spare_16;      // 44     // 2
    unsigned short  tripdata_trip_spare_17;      // 45     // 2
    unsigned short  tripdata_trip_spare_18;      // 46     // 2
    unsigned int    trip_start_date;             // 47     // 4
    unsigned int    trip_start_time;             // 48     // 4
    unsigned int    trip_end_date;               // 49     // 4
    unsigned int    trip_end_time;               // 50     // 4
    unsigned int    trip_spare_23;               // 51     // 4
    unsigned int    trip_spare_24;               // 52     // 4
}__attribute__((packed))REPORT_DATA_2_BODY_DATA;



typedef struct {
    KATCH_PKT_HEADER header;
    unsigned short body_cnt;
    unsigned short resp_code;
    unsigned char  resp_msg_1[30];
    unsigned char  resp_msg_2[30];
    unsigned int   spare_1;
    unsigned char  spare_2[4];
    unsigned char  spare_3[10];
    unsigned char  spare_4[10];
    unsigned char  spare_5[10];
    KATCH_PKT_TAIL tail;
}__attribute__((packed))KATCH_PKT_REPORT_DATA_2_RESP;


// ----------------------
// function
// ----------------------


int katech_pkt_auth_make(unsigned short *size, unsigned char **pbuf);
int katech_pkt_auth_parse(int res, KATCH_PKT_AUTH_RESP* packet);

int katech_pkt_fw_make(unsigned short *size, unsigned char **pbuf);
int katech_pkt_fw_parse(int res, KATCH_PKT_FW_RESP* packet);

int katech_pkt_1_insert_and_send(gpsData_t* p_gpsdata, int force_send);
int katech_pkt_report_data_1_make(unsigned short *size, unsigned char **pbuf, int pkt_att);
int katech_pkt_report_data_1_resp(int res, KATCH_PKT_REPORT_DATA_1_RESP* packet);

int katech_pkt_2_insert_and_send();
int katech_pkt_report_data_2_make(unsigned short *size, unsigned char **pbuf, int pkt_att);
int katech_pkt_report_data_2_resp(int res, KATCH_PKT_REPORT_DATA_2_RESP* packet);


#endif

