#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <time.h>

#include <at/at_util.h>
#include <base/gpstool.h>
#include <base/thermtool.h>
#include <board/battery.h>
#include <board/power.h>
#include <util/debug.h>

#include "gps_utill.h"
#include "gpsmng.h"
#include "packet.h"
#include "logd/logd_rpc.h"
#include "debug.h"


//#define TEST_NISSO_TEST_NUM

int create_report2_divert_buffer(unsigned char **buf, int num)
{
	*buf = malloc( sizeof(nisso_packet2_t) * 2 * num);
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

int get_main_power_volt(int* car_volt_p)
{
	int car_voltage;
    static int last_car_voltage;

    static time_t last_cycle = 0;
    time_t cur_time = 0;
	struct timeval tv;
	struct tm ttm;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);

    cur_time = mktime(&ttm);

    if ( ( (cur_time - last_cycle) > 30 ) || ( last_cycle == 0 ) || ( last_car_voltage == 0 ))
    {
	    if ( at_get_adc_main_pwr(&car_voltage) == AT_RET_SUCCESS )
        {
            last_car_voltage = car_voltage;
        }
        last_cycle = cur_time;
    }
    else
    {
        printf("voltage calc skip..\r\n");
    }

	printf("/ [%d] sec / last sec [%d] / last vol [%d] \r\n",(cur_time - last_cycle), last_cycle, last_car_voltage);
    *car_volt_p = last_car_voltage;

	return AT_RET_SUCCESS;
}

unsigned char convert_angle(float azimuth)
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

	return 0;
}

static unsigned short saved_speed = 0;

int create_report2_data(int ev_code, nisso_packet2_t *packet, gpsData_t gpsdata, char *record, int rec_len)
{
//	char *ver = PRG_VER;
	char phonenum[MAX_DEV_ID_LED] = {0,};
    unsigned int tmp_uint = 0;

	memset(packet, 0x00, sizeof(nisso_packet2_t));

	if ( ( ev_code == eGEO_FENCE_NUM0_EXIT_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM1_EXIT_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM2_EXIT_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM3_EXIT_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM4_EXIT_EVT ) )
	{
		devel_webdm_send_log("geofence evt exit : [%d]\n", ev_code);
		set_geo_event_flag(1);
	}

	if ( ( ev_code == eGEO_FENCE_NUM0_ENTRY_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM1_ENTRY_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM2_ENTRY_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM3_ENTRY_EVT ) ||
		 ( ev_code == eGEO_FENCE_NUM4_ENTRY_EVT ) )
	{
		devel_webdm_send_log("geofence evt entry : [%d]\n", ev_code);
		set_geo_event_flag(0);
	}

	packet->msg_id = MDT800_PROTOCOL_ID;
	packet->msg_type = MDT800_MESSAGE_TYPE2;

	memset(packet->dev_id, 0x20, MAX_DEV_ID_LED);
#ifndef TEST_NISSO_TEST_NUM
	at_get_phonenum(phonenum, MAX_DEV_ID_LED);
#else
	strcpy(phonenum, "01223865316"); // 테스트번호
#endif
	memcpy(packet->dev_id, phonenum, strlen(phonenum));
	
	packet->evcode = ev_code;
	
	packet->date.year = gpsdata.year;
	packet->date.mon  = gpsdata.mon;
	packet->date.day  = gpsdata.day;
	packet->date.hour = gpsdata.hour;
	packet->date.min  = gpsdata.min;
	packet->date.sec  = gpsdata.sec;

	if(gpsdata.active == 1)
    {
		packet->gps_status = eSAT_GSP;
    }
	else
    {
		packet->gps_status = eWCDMA_GSP;
    }
/*
    // report data sync.. only add mdt routine
    if ( ( gpsdata.lat == 0 ) || (gpsdata.lon == 0) )
    {
        // last data..
        gpsData_t last_gpsdata = {0,};
		gps_valid_data_get(&last_gpsdata);
		gpsdata.lat = last_gpsdata.lat;
		gpsdata.lon = last_gpsdata.lon;
    }
*/
	packet->gps_pos.latitude = gpsdata.lat * 10000000.0;
	packet->gps_pos.longitude = gpsdata.lon * 10000000.0;
    // 닛소에서 데이터 밀리는게 이것때문일지도..
    // 서버변경을 절대 않할놈들이니 이런저런짓;;
    if ( packet->gps_pos.latitude > 0 )
        packet->gps_pos.latitude += 1;
    
    if ( packet->gps_pos.longitude > 0 )
        packet->gps_pos.longitude += 1;
	//packet->gps_pos.latitude = 37400305;
	//packet->gps_pos.longitude = 127102385;

	packet->gps_dir = convert_angle(gpsdata.angle);


    if ( ( gpsdata.speed >= 0 ) && ( gpsdata.speed < 180 ) )
    {
        saved_speed = gpsdata.speed;
        printf(" >> current gps speed vaild [%d]\r\n", saved_speed);
    }
    else
    {
        printf(" >> current gps speed invaild [%d]\r\n", saved_speed);
    }

    packet->speed = (unsigned short)saved_speed;


	if(get_server_mileage() == MILEAGE_NOT_INIT)
    {
		tmp_uint = 0;
    }
	else
    {
        tmp_uint = get_server_mileage() + get_gps_mileage();
    }

    packet->vehicle_odo = (unsigned int) tmp_uint;

	packet->report_cycle_time = get_report_interval(); // unit : sec

	{
		char power_stat = get_nisso_pkt__external_pwr();

		if ( ev_code == eBUTTON_NUM0_EVT )
			packet->gpio_status |= ( 1 << 0 );
		else if ( ev_code == eBUTTON_NUM1_EVT )
			packet->gpio_status |= ( 1 << 1 );
		else if ( ev_code == eBUTTON_NUM2_EVT )
			packet->gpio_status |= ( 1 << 2 );
		else if ( ev_code == eBUTTON_NUM3_EVT )
			packet->gpio_status |= ( 1 << 3 );

		packet->gpio_status |= ( power_stat << 4 );
		LOGI(LOG_TARGET, " --> make pkt :: gpio status [0x%02x]\r\n", packet->gpio_status);
		
	}

	{
		int car_voltage = 0;
		static int saved_car_voltage = 0;
		if ( get_main_power_volt(&car_voltage) == AT_RET_SUCCESS )
		{
			if ( ( car_voltage > 0 ) &&  ( car_voltage < 52 ) )
				saved_car_voltage = car_voltage;
		}
		
		packet->dev_power_level = saved_car_voltage*10; // (b-2) 차량배터리전압 : 0.1 volt 단위

		LOGI(LOG_TARGET, " --> make pkt :: power level [%d]\r\n", packet->dev_power_level);
	}

	packet->create_cycle_time = get_collection_interval(); // unit : sec

	//snprintf(packet->version, sizeof(packet->version) - 1 , "%c%c%c", ver[1], ver[2], ver[3]);
	strcpy(packet->version, "002");

#ifdef SERVER_ABBR_NIS0
    {
	    packet->reseved = get_nisso_pkt__invoice_info();
    }
#elif SERVER_ABBR_NIS1
    // invoice info..
    {
        invoice_info_2_t invoice_info = {0,};
        get_nisso_pkt__invoice_info_2(&invoice_info);
	    packet->reseved = invoice_info.invoice;
        strncpy(packet->invoice_date, invoice_info.p_str_invoice_date, 8);
        strncpy(packet->invoice_num_1, invoice_info.p_str_invoice_info_num_1, 8);
        strncpy(packet->invoice_num_2, invoice_info.p_str_invoice_info_num_2, 8);
    }
#endif

	print_report2_data(*packet);

	return sizeof(nisso_packet2_t);
}

void print_report2_data(nisso_packet2_t packet)
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
	printf("\tgpio_status = [0x%02x]\n", packet.gpio_status);
	printf("\tdev_power = [%d]\n", packet.dev_power_level);
	printf("\tcreate_cycle_time = [%d]\n", packet.create_cycle_time);

	printf("\tversion = [%.3s]\n", packet.version);
    printf("\teseved = [%d]\n", packet.reseved);
#ifdef SERVER_ABBR_NIS1
    printf("\trinvoice_date = [%.3s]\n", packet.invoice_date);
    printf("\trinvoice_num_1 = [%.3s]\n", packet.invoice_num_1);
    printf("\trinvoice_num_2 = [%.3s]\n", packet.invoice_num_2);
#endif

}

static char _g_nisso_invoice_val = 0;
char set_nisso_pkt__invoice_info(int invoice)
{
	_g_nisso_invoice_val = (char*)invoice;
    return _g_nisso_invoice_val;
}

char get_nisso_pkt__invoice_info()
{
	return _g_nisso_invoice_val;
}

static invoice_info_2_t _g_nisso_invoice_info_2 = {0,};
int init_nisso_pkt__invoice_info_2()
{
    memset(&_g_nisso_invoice_info_2, 0x20, sizeof(invoice_info_2_t));
    _g_nisso_invoice_info_2.invoice = 0;
    return 1;
}

int set_nisso_pkt__invoice_info_2(invoice_info_2_t* p_invoice_info_2)
{
    if (p_invoice_info_2->invoice == 1 )
    {
        memcpy(&_g_nisso_invoice_info_2, p_invoice_info_2, sizeof(invoice_info_2_t));
    }
    return 1;
}

int get_nisso_pkt__invoice_info_2(invoice_info_2_t* p_invoice_info_2)
{
	memcpy(p_invoice_info_2, &_g_nisso_invoice_info_2, sizeof(invoice_info_2_t));
    return 1;
}

void print_nisso_pkt__invoice_info_2()
{
    printf("invoice print -> invoice [%d]\r\n",_g_nisso_invoice_info_2.invoice);
    printf("invoice print -> p_str_invoice_date [%s]\r\n",_g_nisso_invoice_info_2.p_str_invoice_date);
    printf("invoice print -> p_str_invoice_info_num_1 [%s]\r\n",_g_nisso_invoice_info_2.p_str_invoice_info_num_1);
    printf("invoice print -> p_str_invoice_info_num_2 [%s]\r\n",_g_nisso_invoice_info_2.p_str_invoice_info_num_2);
}




static char _g_nisso_powerstat = 0;
char set_nisso_pkt__external_pwr(char pwr_stat)
{
	_g_nisso_powerstat = pwr_stat;
    return _g_nisso_powerstat;
}

char get_nisso_pkt__external_pwr()
{
	return _g_nisso_powerstat;
}
