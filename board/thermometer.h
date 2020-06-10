#ifndef __LINUX_THERM_H
#define __LINUX_THERM_H

typedef struct
{
	//int sign;
	short data; //jwrho
	int status;
	int alarm;
}TEMP_LOCAL_DATA;

typedef struct
{
	int channel;
	TEMP_LOCAL_DATA temper[4];
}THERMORMETER_DATA;


#pragma pack(push, 1)

typedef struct
{
	char sign;
	char data[3];
}__attribute__((packed))COMMON_UT1;

typedef struct
{
	char stx;
	COMMON_UT1 temper[2];
	char etx;
}__attribute__((packed))UT1_PFnA_10;

typedef struct
{
	char stx;
	COMMON_UT1 temper[2];
	char alarm[2];
	char etx;
}__attribute__((packed))UT1_PFA_12;

typedef struct
{
	char stx;
	COMMON_UT1 temper[4];
	char etx;
}__attribute__((packed))UT1_PFnA_18;

typedef struct
{
	char stx;
	char signiture;
	char channel;
	char status;
	char year[2];
	char mon[2];
	char day[2];
	char hour[2];
	char min[2];
	char latitude[6];
	char longitude[7];
	COMMON_UT1 temper[2];
	char optime[6];
	char etx;
}__attribute__((packed))UT3_PFA_42;

typedef struct
{
	char stx;
	char signiture;
	char channel;
	char status;
	char year[2];
	char mon[2];
	char day[2];
	char hour[2];
	char min[2];
	char speed[3];
	char distance[10];
	COMMON_UT1 temper[2];
	char optime[6];
	char etx;
}__attribute__((packed))UT1_PF3_42;

#pragma pack(pop)


enum TEMP_STATUS
{
	eOK=0,
	eOPEN,
	eSHORT,
	eUNUSED,
	eNOK
};

enum TEMP_CHANNEL
{
	INVALID=0,
	CHANNEL2=2,
	CHANNEL4=4,
};

enum TEMP_ALARM
{
	NORMAL=0,
	HIGHOVER,
	LOWUNDER
};

typedef enum DEVICE_TYPE DEVICE_TYPE_t;
enum DEVICE_TYPE
{
	eUnknown=0,
	eUT1,
	eUT2,
	eUT3
};
#define BAUDRATE_UT1 19200
#define BAUDRATE_UT3 9600

//#define DEV_THERM_PORT			"/dev/ttyMSM"
#define DEV_THERM_PORT			"/dev/ttyHSL2"
#define MAX_THERM_BUF			128
#define GET_THERM_TRY			3
#define USE_TYPE_UT1	1

void dump(char *buf,int len);
int parse_temper(char *buf,int len,THERMORMETER_DATA *therm);
int init_therm();
int get_therm(THERMORMETER_DATA *therm);
int check_type();
int get_tempature(THERMORMETER_DATA	*thermometer);
int set_therm_device(char *dev, int len_dev);

typedef struct
{
	char device[64];
	DEVICE_TYPE_t msg_type;
	int baudrate;
}therm_conf_t;
//configuration *conf = get_config();
//
//	if(conf->therm.msg_type == UT1)
//conf->therm.baudrate

#endif
