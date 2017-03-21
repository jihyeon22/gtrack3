#ifndef _TACOM_DSIC_PROTOCOL_H_
#define _TACOM_DSIC_PROTOCOL_H_

#pragma pack(push, 1)
struct mdt_packet {
	unsigned char protocol_id;
	unsigned char message_id;
	char terminal_id[15];
	unsigned char event_code;
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char position_type;
	signed long gps_x;
	signed long gps_y;
	unsigned short azimuth;
	unsigned char speed;
	unsigned long cumul_dist;
	signed short temperature_a;
	signed short temperature_b;
	signed short temperature_c;
	unsigned short report_period;
	unsigned char gpio_input;
	unsigned char power_type;
	unsigned short create_period;
	unsigned short crc16;
	unsigned char terminate_char;
}__attribute__((packed));
typedef struct mdt_packet mdt_packet_t;

struct user_hdr {
	unsigned short length;
	unsigned long packet_id;			//set on tacoc
	char req_cmd_id[4];					//set on tacoc
	unsigned char device_type;
	unsigned char product_type;
}__attribute__((packed));
typedef struct user_hdr user_hdr_t;

struct dtg_hdr {
	char vehicle_model[20];
	char vehicle_id_num[17];
	char vehicle_type[2];
	char registration_num[12];
	char business_license_num[10];
	char driver_code[18];
}__attribute__((packed));
typedef struct dtg_hdr dtg_hdr_t;

struct dtg_data {
	unsigned long day_run_dist;
	unsigned long cumul_run_dist;
	unsigned short year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char min;
	unsigned char sec;
	unsigned char c_sec;
	unsigned char speed;
	unsigned short rpm;
	unsigned char bs;
	signed long gps_x;
	signed long gps_y;
	unsigned short azimuth;
	signed short accelation_x;
	signed short accelation_y;
	unsigned char status;
	/* extended */
	unsigned long day_oil_usage;
	unsigned long cumulative_oil_usage;
}__attribute__((packed));
typedef struct dtg_data dtg_data_t;

#pragma pack(pop)

#endif 

