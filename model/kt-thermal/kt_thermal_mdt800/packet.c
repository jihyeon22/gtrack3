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

#if 0 // btn odo senario remove.
    if ( ev_code == eBUTTON_START_MILEAGE_EVT )
    {
        packet->vehicle_odo = 0;
    }
    else if ( ev_code == eBUTTON_END_MILEAGE_EVT )
    {
        packet->vehicle_odo = ktth_sernaio__keybtn_pkt_odo_calc();
    }
	else
#endif
    {   
        packet->vehicle_odo = ktth_sernaio__normal_pkt_odo_calc(cur_odo);
    }

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

