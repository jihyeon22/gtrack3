#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <at/at_util.h>
#include <base/gpstool.h>
#include <base/thermtool.h>
#include <board/battery.h>
#include <board/power.h>
#include "gps_utill.h"
#include "gpsmng.h"
#include "packet.h"
#include "logd/logd_rpc.h"
#include "debug.h"

#include "color_printf.h"

#include "packetdata_util.h"
#include "kt_flood_unit.h"

int create_report2_divert_buffer(unsigned char **buf, int num)
{
	*buf = malloc( sizeof(lotte_packet2_t) * 2 * num);
	if(*buf == NULL)
		return -1;

	return 0;
}

int is_available_report_divert_buffer(int cnt)
{
	if(cnt >= (LIMIT_TRANSFER_PACKET_COUNT-5)) //max static buffer size check
		return 0;

	return 1;
}

int get_main_power_volt()
{
	int voltage;
	voltage = battery_get_battlevel_car();
	//printf("voltage is [%d]\r\n",voltage);
	
	if(voltage < 0)
		voltage = 0;

	voltage = voltage/100;
	return voltage;
}

unsigned char convert_angle_kt_thermal_mdt(float azimuth)
{
	int bearing;
	unsigned char ret_val = 0;
	bearing = RoundOff(azimuth, 0);

	ret_val = bearing / 10;

	return ret_val;
}


int create_report2_data(int ev_code, lotte_packet2_t *packet, gpsData_t gpsdata, char *record, int rec_len)
{
	char *ver = PRG_VER;
	char phonenum[MAX_DEV_ID_LED];

	static int cur_odo = 0;

	memset(packet, 0x00, sizeof(lotte_packet2_t));

	packet->msg_id = MDT800_PROTOCOL_ID;
	packet->msg_type = MDT800_MESSAGE_TYPE2;

	memset(packet->dev_id, 0x20, MAX_DEV_ID_LED);
	at_get_phonenum(phonenum, MAX_DEV_ID_LED);
	memcpy(packet->dev_id, phonenum, strlen(phonenum));
	
	packet->evcode = ev_code;
	
	packet->date.year = gpsdata.year;
	packet->date.mon  = gpsdata.mon;
	packet->date.day  = gpsdata.day;
	packet->date.hour = gpsdata.hour;
	packet->date.min  = gpsdata.min;
	packet->date.sec  = gpsdata.sec;

	if(gpsdata.active == 1)
		packet->gps_status = eSAT_GSP;
	else
		packet->gps_status = eNOSIGNAL_GPS;

	packet->gps_pos.latitude = gpsdata.lat * 10000000.0;
	packet->gps_pos.longitude = gpsdata.lon * 10000000.0;

	packet->gps_dir = convert_angle_kt_thermal_mdt(gpsdata.angle);
	packet->speed = gpsdata.speed;

	cur_odo = get_server_mileage() + get_gps_mileage();

	packet->report_cycle_time = get_report_interval(); // unit : sec
	packet->gpio_status = 0;

	packet->dev_power = power_get_power_source();
	packet->create_cycle_time = get_collection_interval(); // unit : sec

	sprintf(packet->version, "%s", KT_THERMAL_VER_STRING);

    {
        char test_tmp[32] ={0,};
        strncpy(test_tmp, packet->version, 3);
        printf("version string is [%s]\r\n", test_tmp);
    }

	packet->record_leng = rec_len % 101;
	strncpy(packet->record, record, packet->record_leng);

	//packet->record_leng = sprintf(packet->record, ">BATVOLT:%d<", car_volt % 100);

	print_report2_data(*packet);

	return sizeof(lotte_packet2_t)-100 + packet->record_leng;
}

void print_report2_data(lotte_packet2_t packet)
{
	int i;
	printf("created report2 data ====>\n");
	printf("\tdata = %04d-%02d-%02d %02d:%02d:%02d\n", 
												packet.date.year,
												packet.date.mon,
												packet.date.day,
												packet.date.hour,
												packet.date.min,
												packet.date.sec
												);
	printf("\tmsg_id = [0x%02x]\n", packet.msg_id);
	printf("\tmsg_type = [0x%02x]\n", packet.msg_type);

	printf("\tdev_id : ");
	for(i = 0; i < MAX_DEV_ID_LED; i++) {
		printf("[%02x]", packet.dev_id[i]);
	}
	printf("\n");
	printf("\tevcode = [%d]\n", packet.evcode);
	printf("\tgps_status = [%d]\n", packet.gps_status);

	printf("\tlatitude = [%d]\n", packet.gps_pos.latitude);
	printf("\tlongitude = [%d]\n", packet.gps_pos.longitude);

	printf("\tgps_dir = [%d]\n", packet.gps_dir);
	printf("\tspeed = [%d]\n", packet.speed);

	printf("\tvehicle_odo = [%u]\n", packet.vehicle_odo);

	printf("\treport_cycle_time = [%d]\n", packet.report_cycle_time);
	printf("\tgpio_status = [%d]\n", packet.gpio_status);
	printf("\tdev_power = [%d]\n", packet.dev_power);
	printf("\tcreate_cycle_time = [%d]\n", packet.create_cycle_time);

	printf("\tversion = [%.3s]\n", packet.version);
	printf("\trecord_leng = [%d]\n", packet.record_leng);
	printf("\trecord = [%.*s]\n", packet.record_leng, packet.record);
}

int create_flood_divert_buffer(unsigned char **buf, int num)
{
	*buf = malloc( sizeof(flood_packet_t) * 2 * num);
	if(*buf == NULL)
		return -1;

	return 0;
}
char  DecimalToBCD (int Decimal)
{
    return (((Decimal/10) << 4) | (Decimal % 10));
}
int create_flood_report_data(flood_packet_t *packet)
{
	int num = 0, i = 0;
	int sensor_idx = 0;
	int packetlen = sizeof(flood_packet_t);
	char year[2];
	//char phonenum[MAX_DEV_ID_LED];

	memset(packet, 0x00, sizeof(flood_packet_t));

	packet->start_idx = FLOOD_PACKET_START_FLAG;
	packet->msg_id = FLOOD_PROTOCOL_ID;
	packet->msg_enc_type = FLOOD_PROTOCOL_ENC_TYPE;
	packet->msg_len = htonl(packetlen);

	packet->unique_type = FLOOD_UNIQUE_TYPE;

	get_phonenum_binary(packet->dev_id);

	struct tm time = {0,};
	get_modem_time_tm(&time);

	packet->date.year[0] = DecimalToBCD((time.tm_year + 1900) / 100);
	packet->date.year[1] = DecimalToBCD((time.tm_year + 1900) % 100);
	//packet->date.year = year;
	//packet->date.year = time.tm_year + 1900;
	packet->date.mon = DecimalToBCD(time.tm_mon + 1);
	packet->date.day = DecimalToBCD(time.tm_mday);
	packet->date.hour = DecimalToBCD(time.tm_hour);
	packet->date.min = DecimalToBCD(time.tm_min);
	packet->date.sec = DecimalToBCD(time.tm_sec);

	packet->sensor_date_len = 6;

	for(i = 0; i < MAX_SENSOR_CNT; i++)
	{
		packet->sensor_data[i].sensor_num = i + 1;
		packet->sensor_data[i].sensor_state = g_sensor_state[i];
	}
	
	packet->end_idx = FLOOD_PACKET_END_FLAG;

	print_flood_report_data(*packet);

	return packetlen;
}

void print_flood_report_data(flood_packet_t packet)
{
	int i;

	printf("created flood_report_data data ====>\n");
	printf("\tdata = %02x%02x-%02x-%02x %02x:%02x:%02x\n", 
												packet.date.year[0],
												packet.date.year[1],
												packet.date.mon,
												packet.date.day,
												packet.date.hour,
												packet.date.min,
												packet.date.sec
												);
	printf("\tmsg_id = [0x%02x]\n", packet.msg_id);
	printf("\tmsg_enc_type = [0x%02x]\n", packet.msg_enc_type);
	//printf("\tmsg_len = [%0d]\n", htonl(packet.msg_len));
	printf("\tunique_type = [0x%02x]\n", packet.unique_type);

	printf("\tdev_id : ");

	for(i = 0; i < MAX_DEV_ID_LEN; i++) {
		printf("[%02x]", packet.dev_id[i]);
	}
	printf("\n");


	for(i = 0; i < MAX_SENSOR_CNT; i++)
	{
		printf("\tsensor_num = [%d]\n", packet.sensor_data[i].sensor_num);
		printf("\tsensor_data = [0x%02x]\n", packet.sensor_data[i].sensor_state);
	}


}

