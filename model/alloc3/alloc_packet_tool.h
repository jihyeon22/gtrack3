#ifndef __ALLOCATION_PACKET_TOOL_HEADER__
#define __ALLOCATION_PACKET_TOOL_HEADER__

#include <time.h>
#include "alloc_packet.h"
#include <board/board_system.h>

#define GEO_FENCE_MAX_COUNT  100

// jhcho compile
//#define RFID_SAVED_FILE			"/data/alloc_rfid.dat"
//#define GEOFENCE_SAVED_FILE		"/data/alloc_geofence.dat"
#define RFID_SAVED_FILE			CONCAT_STR(USER_DATA_DIR, "/alloc_rfid.dat")
#define GEOFENCE_SAVED_FILE		CONCAT_STR(USER_DATA_DIR, "/alloc_geofence.dat")

typedef enum load_saved_file_status load_saved_file_status_t;
enum load_saved_file_status{
	eSTAT_LOAD_INIT = 0,		// 0
	eSTAT_LOAD_FAIL,	// 1
	eSTAT_LOAD_SUCCESS		// 2
};

typedef enum get_passenger_status get_passenger_status_t;
enum get_passenger_status{
	eGET_PASSENGER_STAT_NULL = 0,		// 0
	eGET_PASSENGER_STAT_DOWNLOADING,	// 1
	eGET_PASSENGER_STAT_COMPLETE		// 2
};

typedef struct {
	int  status;
	unsigned char  cmd_id[10];
	int  max_pkt;
	int  cur_pkt;
	unsigned char  version[8];
	int  rfid_list_len;
	unsigned char  rfid_list[380*300*3];
}__attribute__((packed))allocation_passenger_info_t;

typedef enum get_geofence_status get_geofence_status_t;
enum get_geofence_status{
	eGET_GEOFENCE_STAT_NULL = 0,		// 0
	eGET_GEOFENCE_STAT_DOWNLOADING,	// 1
	eGET_GEOFENCE_STAT_COMPLETE		// 2
};

typedef struct {
	int  status;
	unsigned char  cmd_id[10];
	int  max_pkt;
	int  cur_pkt;
	int  total_geo_fence;
	int  stop_num[GEO_FENCE_MAX_COUNT];
	double latitude[GEO_FENCE_MAX_COUNT];
	double longitude[GEO_FENCE_MAX_COUNT];
	int range[GEO_FENCE_MAX_COUNT];
	int recent_geo_fence;
}__attribute__((packed))allocation_geofence_info_t;


typedef struct {
	int  status;
	unsigned char  cmd_id[10];
	unsigned char  ftp_ip[20];
	unsigned char  ftp_port[20];
	unsigned char  ftp_id[20];
	unsigned char  ftp_pw[20];
	unsigned char  filename[32];
}__attribute__((packed))allocation_ftpserver_info_t;

void init_passenger_info();
int load_file_passenger_info(char* version);
int save_geofence_info(packet_frame_t result);
int get_passenger_info_stat();
int get_passenger_cmd_id(char* cmd_id);
int save_passenger_info(packet_frame_t result);
int find_rfid(char* rfid_pat, int size, char *rfid_date);
void alloc_geo_fence_info_init();
int load_file_geofence_info();
int get_geo_fence_id(int idx, char* geo_fence_id);
char _alloc_get_binary_packet_id(packet_frame_t result);
int get_geo_fence_info_stat();
int get_geo_fence_cmd_id(char* cmd_id);
int set_geofence_data();
void print_geo_fence_info();
void alloc_ftpserver_info_init();
int save_ftpserver_info(packet_frame_t result);
int get_ftpserver_cmd_id(char* cmd_id);
void print_ftpserver_info();

unsigned char convert_angle(float azimuth);
int get_geofence_status(); //jwrho

static allocation_ftpserver_info_t ftp_server_info;

#endif /* __ALLOCATION_PACKET_TOOL_HEADER__ */
