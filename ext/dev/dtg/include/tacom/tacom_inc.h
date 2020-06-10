<<<<<<< HEAD
#ifndef __TACOM_COMMON_H__
#define __TACOM_COMMON_H__

// type...
#define TACOM_VEHICLE_MODEL				0x0001
#define TACOM_VEHICLE_ID_NUM			0x0002
#define TACOM_VEHICLE_TYPE				0x0003
#define TACOM_REGISTRAION_NUM			0x0004
#define TACOM_BUSINESS_LICENSE_NUM		0x0005
#define TACOM_DRIVER_CODE				0x0006
#define TACOM_SPEED_COMPENSATION		0x0007
#define TACOM_RPM_COMPENSATION			0x0008
#define TACOM_ACCUMULATED_DISTANCE		0x0009
#define TACOM_OIL_SUPPLY_COMPENSATION	0x000a
#define TACOM_OIL_TIME_COMPENSATION		0x000b
/* spaecial_case*/
#define TACOM_DATA_BUILD_PERIOD			0x0100
#define TACOM_DATA_TYPE					0x0101

struct tacom_info {
    int code;
    char *data;
};
typedef struct tacom_info tacom_info_t;

// get stream buff form struct tm.
char *tacom_get_stream();

int tacom_read_summary();
int tacom_ack_summary();
int tacom_read_current();
int tacom_unreaded_records_num();
int tacom_read_records (int r_num);
int tacom_ack_records (int r_num);
int tacom_read_last_records (void);

char *tacom_get_info (int type);
int tacom_get_info_hdr(void *infohdr);
int tacom_set_info (int type, char *infodata);

#endif // __TACOM_H__

=======
#ifndef __TACOM_COMMON_H__
#define __TACOM_COMMON_H__

// type...
#define TACOM_VEHICLE_MODEL				0x0001
#define TACOM_VEHICLE_ID_NUM			0x0002
#define TACOM_VEHICLE_TYPE				0x0003
#define TACOM_REGISTRAION_NUM			0x0004
#define TACOM_BUSINESS_LICENSE_NUM		0x0005
#define TACOM_DRIVER_CODE				0x0006
#define TACOM_SPEED_COMPENSATION		0x0007
#define TACOM_RPM_COMPENSATION			0x0008
#define TACOM_ACCUMULATED_DISTANCE		0x0009
#define TACOM_OIL_SUPPLY_COMPENSATION	0x000a
#define TACOM_OIL_TIME_COMPENSATION		0x000b
/* spaecial_case*/
#define TACOM_DATA_BUILD_PERIOD			0x0100
#define TACOM_DATA_TYPE					0x0101

struct tacom_info {
    int code;
    char *data;
};
typedef struct tacom_info tacom_info_t;

// get stream buff form struct tm.
char *tacom_get_stream();

int tacom_read_summary();
int tacom_ack_summary();
int tacom_read_current();
int tacom_unreaded_records_num();
int tacom_read_records (int r_num);
int tacom_ack_records (int r_num);
int tacom_read_last_records (void);

char *tacom_get_info (int type);
int tacom_get_info_hdr(void *infohdr);
int tacom_set_info (int type, char *infodata);

#endif // __TACOM_H__

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
