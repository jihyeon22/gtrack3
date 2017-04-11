#pragma once

#pragma pack(push, 1)

struct vehicle_vrn
{
	char vrn[12];
}__attribute__((packed));
typedef struct vehicle_vrn vehicle_vrn_t;

struct vehicle_gps
{
	unsigned int gps_x;
	unsigned int gps_y;
	unsigned short azimuth;
	char speed;
	int distantall;
}__attribute__((packed));
typedef struct vehicle_gps vehicle_gps_t;

#pragma pack(pop)

#define VRN_INFO_FILE_PATH	"/data/mds/data/vrn.dat"
#define VGPS_INFO_FILE_PATH	"/data/mds/data/vgps.dat"

void load_vrn_info();
void set_vrn_info(char *vrn);
int get_vrn_info(char *vrn, int vrn_len);

void load_vgps_info();
void save_vgps_info(vehicle_gps_t *data);
void set_vgps_info_memory(vehicle_gps_t data);
void get_vgps_info_memory(vehicle_gps_t *data);

