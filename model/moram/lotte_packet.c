#ifdef TEST_CODE_ENABLE
	#include "depend.h"
#else
	#include <time.h>
	#include <at/at_util.h>
	#include <base/gpstool.h>
	#include <base/thermtool.h>
	#include <board/battery.h>
	#include <board/power.h>
	#include "lotte_gps_utill.h"
	#include "lotte_gpsmng.h"
	#include "logd/logd_rpc.h"
	#include "debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lotte_packet.h"


int create_report_divert_buffer(unsigned char **buf, int num)
{
	*buf = malloc(sizeof(lotte_packet_t) * 2 * num);
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

unsigned char convert_angle_moram_mdt(float azimuth)
{
	int bearing;

	bearing = RoundOff(azimuth, 0);

	if(bearing == 0) {
		return 0;
	} else if((bearing > 0) && (bearing < 90)) {
		return 1;
	} else if(bearing == 90) {
		return 2;
	} else if((bearing > 90) && (bearing < 180)) {
		return 3;
	} else if(bearing == 180) {
		return 4;
	} else if((bearing > 180) && (bearing < 270)) {
		return 5;
	} else if(bearing == 270) {
		return 6;
	} else if((bearing > 270) && (bearing <= 360)) {
		return 7;
	}

	return 0xff;
}

void create_report_data(int ev_code, lotte_packet_t *packet, gpsData_t gpsdata)
{
	THERMORMETER_DATA tmp_therm;
	char phonenum[MAX_DEV_ID_LED];
	memset(packet, 0x00, sizeof(lotte_packet_t));

	packet->msg_id = LOTTE_PROTOCOL_ID;
	packet->msg_type = LOTTE_MESSAGE_TYPE;

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
		packet->gps_status = eWCDMA_GSP;

	packet->gps_pos.latitude = gpsdata.lat * 10000000.0;
	packet->gps_pos.longitude = gpsdata.lon * 10000000.0;

	packet->gps_dir = convert_angle_moram_mdt(gpsdata.angle);
	packet->speed = gpsdata.speed;

	if(get_server_mileage() == MILEAGE_NOT_INIT)
		packet->vehicle_odo = 0;
	else
		packet->vehicle_odo = get_server_mileage() + get_gps_mileage();

	packet->temp1 = -5555;
	packet->temp2 = -5555;
	packet->temp3 = -5555;

	if(therm_get_curr_data(&tmp_therm) == 0)
	{
		int i = 0;
		short temper = 0;

		for(i=0 ; i < tmp_therm.channel; i++)
		{
			switch(tmp_therm.temper[i].status)
			{
				case eOK:
					temper =  tmp_therm.temper[i].data;
					break;
					
				case eOPEN:
					temper = -3333;
					break;
					
				case eSHORT:
					temper = -4444;
					break;
					
				case eUNUSED:
				case eNOK:
				default:
					temper = -5555;
			}
			printf("CH-%d : %d C\n", i, temper);

			if(i  == 0)
				packet->temp1 = temper;
			else if(i == 1)
				packet->temp2 = temper;
			else if(i == 2)
				packet->temp3 = temper;
		}
	}

	packet->report_cycle_time = get_report_interval(); // unit : sec
	packet->gpio_status = 0;

	packet->dev_power = power_get_power_source();
	packet->create_cycle_time = get_collection_interval(); // unit : sec

	print_report_data(*packet);
}

void print_report_data(lotte_packet_t packet)
{
	int i;
	printf("created report data ====>\n");
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
	printf("\ttemp1 = [%d]\n", packet.temp1);
	printf("\ttemp2 = [%d]\n", packet.temp2);
	printf("\ttemp3 = [%d]\n", packet.temp3);
	printf("\treport_cycle_time = [%d]\n", packet.report_cycle_time);
	printf("\tgpio_status = [%d]\n", packet.gpio_status);
	printf("\tdev_power = [%d]\n", packet.dev_power);
	printf("\tcreate_cycle_time = [%d]\n", packet.create_cycle_time);
}

