<<<<<<< HEAD

#ifndef __ALLKEY_BCM_1_INTERNAL_H__
#define __ALLKEY_BCM_1_INTERNAL_H__

#define ALLKEY_BCM_INVAILD_FD 0xAABBCC

#define USE_MUTEX_LOCK      1
#define NOT_USE_MUTEX_LOCK  0
typedef enum
{
    e_cmd_door_ctr = 'D',
    e_cmd_evt_recv = 'E',
    e_cmd_unknown_T = 'T',
    e_cmd_unknown_U = 'U',
    e_cmd_unknown_V = 'V',
    e_cmd_req_stat = 'S',
    e_cmd_unknown_W = 'W',
    e_cmd_door_evt_setting = 'M',
    e_cmd_unknown_Z = 'Z',
    e_cmd_horn_ctr = 'P',
    e_cmd_twingkle_light_ctr = 'A',
    e_cmd_theft_sensor = 'L',
    //e_cmd_unknown_R = 'R',
    e_cmd_knocksensor = 'R',
    e_cmd_unknown_H = 'H',
    e_cmd_unknown_I = 'I',
    e_cmd_unknown_C = 'C',
    e_cmd_unknown_B = 'B',
}e_CMD_LIST;


typedef enum
{
    e_knock_cmd_passwd    = 0x50, //'P',
    e_knock_cmd_id        = 0x73, //'s',
    e_knock_cmd_timedate  = 0x54, //'T',
}e_KNOCKSENSOR_CMD_LIST;
=======

#ifndef __ALLKEY_BCM_1_INTERNAL_H__
#define __ALLKEY_BCM_1_INTERNAL_H__

#define ALLKEY_BCM_INVAILD_FD 0xAABBCC

#define USE_MUTEX_LOCK      1
#define NOT_USE_MUTEX_LOCK  0
typedef enum
{
    e_cmd_door_ctr = 'D',
    e_cmd_evt_recv = 'E',
    e_cmd_unknown_T = 'T',
    e_cmd_unknown_U = 'U',
    e_cmd_unknown_V = 'V',
    e_cmd_req_stat = 'S',
    e_cmd_unknown_W = 'W',
    e_cmd_door_evt_setting = 'M',
    e_cmd_unknown_Z = 'Z',
    e_cmd_horn_ctr = 'P',
    e_cmd_twingkle_light_ctr = 'A',
    e_cmd_theft_sensor = 'L',
    //e_cmd_unknown_R = 'R',
    e_cmd_knocksensor = 'R',
    e_cmd_unknown_H = 'H',
    e_cmd_unknown_I = 'I',
    e_cmd_unknown_C = 'C',
    e_cmd_unknown_B = 'B',
}e_CMD_LIST;


typedef enum
{
    e_knock_cmd_passwd    = 0x50, //'P',
    e_knock_cmd_id        = 0x73, //'s',
    e_knock_cmd_timedate  = 0x54, //'T',
}e_KNOCKSENSOR_CMD_LIST;
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
#endif