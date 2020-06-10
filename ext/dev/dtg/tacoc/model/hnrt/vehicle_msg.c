#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <util/storage.h>
#include <board/board_system.h>
#include <wrapper/dtg_atcmd.h>
#include <board/modem-time.h>
#include "vehicle_msg.h"
#include <board/thermometer.h>
#include "dtg_data_manage.h"
#include "dtg_net_com.h"
#include "dtg_regist_process.h"
#include <wrapper/dtg_log.h>

#include <board/power.h>
#include <board/battery.h>
extern resp_pck_t msg_resp[];
vehicle_vrn_t g_vhcInfo = {
	.vrn = "####00##0000",
};

vehicle_gps_t g_vgpsInfo = {
	.gps_x = 0,
	.gps_y = 0,
	.azimuth = 0,
	.speed = 0,
	.distantall = 0
};

void load_vrn_info()
{
	memset(&g_vhcInfo, 0x00, sizeof(vehicle_vrn_t));
	strcpy(g_vhcInfo.vrn, "####00##0000");
	storage_load_file(VRN_INFO_FILE_PATH, &g_vhcInfo, sizeof(vehicle_vrn_t));
}

void set_vrn_info(char *vrn)
{
	if(memcmp(vrn, g_vhcInfo.vrn, 12))
	{
		memcpy(g_vhcInfo.vrn, vrn, 12);
		storage_save_file(VRN_INFO_FILE_PATH, &g_vhcInfo, sizeof(vehicle_vrn_t));
	}
}

int get_vrn_info(char *vrn, int vrn_len)
{
	if(vrn == NULL)
		return -1;

	if(vrn_len < sizeof(g_vhcInfo.vrn))
		return -2;

	memcpy(vrn, g_vhcInfo.vrn, sizeof(g_vhcInfo.vrn));
	return 0;
}

void load_vgps_info()
{
	memset(&g_vgpsInfo, 0x00, sizeof(vehicle_gps_t));
	storage_load_file(VGPS_INFO_FILE_PATH, &g_vgpsInfo, sizeof(vehicle_gps_t));
}

void save_vgps_info(vehicle_gps_t *data)
{
	if(data != NULL)
		memcpy(&g_vgpsInfo, data, sizeof(vehicle_gps_t));

	storage_save_file(VGPS_INFO_FILE_PATH, &g_vgpsInfo, sizeof(vehicle_gps_t));
}

void set_vgps_info_memory(vehicle_gps_t data)
{
	memcpy(&g_vgpsInfo, &data, sizeof(vehicle_gps_t));
}

void get_vgps_info_memory(vehicle_gps_t *data)
{
	memcpy(data, &g_vgpsInfo, sizeof(vehicle_gps_t));
}

int wait_delay_status(int sec)
{
	int i;
	for(i = 0; i < sec; i++)
	{
		if(power_get_ignition_status() == POWER_IGNITION_ON)
			return -1;
		sleep(1);
	}
	return 0;
}

#if defined(BOARD_TL500S) || defined(BOARD_TL500K)

void _printf_vehicle_msg(vhc_pck_t* vehicle_msg)
{
	int i;
	printf("header : prtc_id   [0x%08x]\n", ntohs(vehicle_msg->header.prtc_id));
	printf("header : msg_id    [0x%08x]\n", vehicle_msg->header.msg_id);
	printf("header : svc_id    [0x%08x]\n", vehicle_msg->header.svc_id);
	printf("header : msglength [0x%04x%02x%02x]\n", vehicle_msg->header.msg_len_mark[0],
												    vehicle_msg->header.msg_len_mark[1],
										           	vehicle_msg->header.msg_len_mark[2]);

	printf("body : vrn [");
	for(i = 0; i < sizeof(vehicle_msg->body.vrn); i++)
		printf("%c", vehicle_msg->body.vrn[i]);

	printf(" hex : ");
	for(i = 0; i < sizeof(vehicle_msg->body.vrn); i++)
		printf("[0x%02x] ", vehicle_msg->body.vrn[i]);
	printf("]\n");

	printf("body : tid [0%d]\n", ntohl(vehicle_msg->body.tid));
	printf("body : opationflag [0x%04x%02x%02x]\n", vehicle_msg->body.opationflag[0],
												    vehicle_msg->body.opationflag[1],
										           	vehicle_msg->body.opationflag[2]);

	printf("body : timestamp [%d]\n", ntohl(vehicle_msg->body.timestamp));
	printf("body : timestapmMsec [%d]\n", vehicle_msg->body.timestapmMsec);
	printf("body : EventCode [%d]\n", ntohs(vehicle_msg->body.EventCode));


	printf("body : gps_x [%d]\n", ntohl(vehicle_msg->body.gps_x));
	printf("body : gps_y [%d]\n", ntohl(vehicle_msg->body.gps_y));
	printf("body : azimuth [%d]\n", ntohs(vehicle_msg->body.azimuth));
	printf("body : speed [%d]\n", vehicle_msg->body.speed);
	printf("body : distantal [%d]\n", ntohl(vehicle_msg->body.distantall));

	printf("body : batteryVoltage [%d]\n", ntohs(vehicle_msg->body.batteryVoltage));
	printf("body : temperature1 [%d]\n", (short)ntohs(vehicle_msg->body.temp1));
	printf("body : temperature2 [%d]\n", (short)ntohs(vehicle_msg->body.temp2));

}

void build_mdt_vehicle_msg
(vhc_pck_t* vehicle_msg, unsigned short event_num)
{
	int tmp_msg_mark;
	int tmp_option_flag;
	char *pphoennum;
	char vrn[32];;
	static unsigned char msec = 0;
	//int power_source;
	int batteryVoltage;
	int end_packet_flag = 0x01;
	short temp1;
	short temp2;
	int i;
	int ret;
	int temper;
	THERMORMETER_DATA	thermometer;
	char tmp_buf[20];


	printf("sizeof(msg_vehiclestate_t) = [%d]\n", sizeof(msg_vehiclestate_t));
#ifdef HOST_BYTE_ORDER
	vehicle_msg->header.prtc_id = 0x0003;
#else
	vehicle_msg->header.prtc_id = 0x0300; //byte order
#endif
	vehicle_msg->header.msg_id  = eMsgVechieState;
	vehicle_msg->header.svc_id  = 0x02;
	
	tmp_msg_mark = (sizeof(msg_vehiclestate_t)<< 1)+end_packet_flag;
	vehicle_msg->header.msg_len_mark[0] = ((0x00ff0000&tmp_msg_mark) >> 16);
	vehicle_msg->header.msg_len_mark[1] = ((0x0000ff00&tmp_msg_mark) >> 8);
	vehicle_msg->header.msg_len_mark[2] = (0x000000ff&tmp_msg_mark);
	

	memset(vehicle_msg->body.vrn, 0x00, 0x00);
	memset(vrn, 0x00, sizeof(vrn));
	get_vrn_info(vrn, sizeof(vrn));
	
	memset(tmp_buf, 0x00, sizeof(tmp_buf));
	if (!strncmp(vrn, "####",4)) {
		memset(vehicle_msg->body.vrn, 0, 12);
		memcpy(vehicle_msg->body.vrn, &vrn[4], 8);
	} else if (!strncmp(vrn, "0000",4)) {
		memset(vehicle_msg->body.vrn, 0, 12);
		memcpy(vehicle_msg->body.vrn, &vrn[4], 8);
	} else if(!strncmp(vrn, "����",4)) {
		memset(vehicle_msg->body.vrn, 0, 12);
		memcpy(vehicle_msg->body.vrn, &vrn[4], 8);
	} else	{
		memcpy(vehicle_msg->body.vrn, vrn,12);
	}
	memcpy(tmp_buf, vehicle_msg->body.vrn, 12);
	DTG_LOGT("vehicle_msg vrn = [%s]\n", tmp_buf);

	pphoennum = atcmd_get_phonenum();
	vehicle_msg->body.tid = 0;
	if(pphoennum != NULL) {
#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.tid = atoi(pphoennum);	
#else
	vehicle_msg->body.tid = htonl(atoi(pphoennum));
#endif
	}

	tmp_option_flag = eOptionBatteryVoltage | eOptionalTemperature | eOptionalVehicleInfoExt;
	vehicle_msg->body.opationflag[0] = ((0x00ff0000&tmp_option_flag) >> 16);
	vehicle_msg->body.opationflag[1] = ((0x0000ff00&tmp_option_flag) >> 8);
	vehicle_msg->body.opationflag[2] = (0x000000ff&tmp_option_flag);

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.timestamp = get_modem_time_utc_sec();
#else
	vehicle_msg->body.timestamp = htonl(get_modem_time_utc_sec());
#endif
	vehicle_msg->body.timestapmMsec = msec++;

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.EventCode = event_num;
#else
	vehicle_msg->body.EventCode = htons(event_num);
#endif

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.gps_x = g_vgpsInfo.gps_x;
#else
	vehicle_msg->body.gps_x =htonl(g_vgpsInfo.gps_x);
#endif

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.gps_y = g_vgpsInfo.gps_y;
#else
	vehicle_msg->body.gps_y =htonl(g_vgpsInfo.gps_y);
#endif

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.azimuth = g_vgpsInfo.azimuth;
#else
	vehicle_msg->body.azimuth =htons(g_vgpsInfo.azimuth);
#endif

	vehicle_msg->body.speed = g_vgpsInfo.speed;

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.distantall = g_vgpsInfo.distantall;
#else
	vehicle_msg->body.distantall = htonl(g_vgpsInfo.distantall);
#endif

	batteryVoltage = battery_get_battlevel_car();
	batteryVoltage = batteryVoltage/100;
#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.batteryVoltage = batteryVoltage;
#else
	vehicle_msg->body.batteryVoltage = htons(batteryVoltage);
#endif

	
	//default value setting
	temp1 = -5555;
	temp2 = -5555;

	ret = get_tempature(&thermometer);
	if(ret >= 0) {
		for(i = 0; i < thermometer.channel; i++)
		{
			switch(thermometer.temper[i].status)
			{
				case eOK:
					temper =  thermometer.temper[i].data;
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
			DTG_LOGD("CH-%d : %d C\n", i, temper);
			if(i == 0)
				temp1 = temper;
			else if(i == 1)
				temp2 = temper;
		}
	}

#ifdef HOST_BYTE_ORDER
	vehicle_msg->body.temp1 = temp1;
	vehicle_msg->body.temp2 = temp2;
#else
	vehicle_msg->body.temp1 = htons(temp1);
	vehicle_msg->body.temp2 = htons(temp2);
#endif

	if(msec > 99)
		msec = 0;

	_printf_vehicle_msg(vehicle_msg);
}


#endif


void send_vehicle_msg()
{
	int retry = 0;
	int ret;
	vhc_pck_t vehicle_msg;
	int sock_fd;
	build_mdt_vehicle_msg(&vehicle_msg, 27); //27 si KEY OFF Event

	while(retry++ < 3) {
		sock_fd = connect_to_server(get_server_ip_addr(), get_server_port());
		if (ret < 0) {
			disconnect_to_server(sock_fd);
			if(wait_delay_status(5) < 0)
				break;

			continue;
		}
		ret = send_to_dtg_server(sock_fd, (unsigned char *)&vehicle_msg, sizeof(vhc_pck_t), (char *)__func__, __LINE__, "vehicle_msg send");
		if (ret < 0) {
			disconnect_to_server(sock_fd);
			if(wait_delay_status(5) < 0)
				break;

			continue;
		}


		ret = receive_response(sock_fd, 0);
		if (ret < 0){
			disconnect_to_server(sock_fd);
			if(wait_delay_status(5) < 0)
				break;

			continue;
		}


		if (msg_resp[0].result == 1) //success 
		{
			disconnect_to_server(sock_fd);
			break;
		}
		else if (msg_resp[0].result == HNURI_RESP_ERR_UNKOWN_DEVICE)
		{
			disconnect_to_server(sock_fd);
			send_server_error_report("vehicle status", HNURI_RESP_ERR_UNKOWN_DEVICE);
			if(wait_delay_status(10) < 0)
				break;
			continue;
		}
		else if (msg_resp[0].result == HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE)
		{
			disconnect_to_server(sock_fd);
			send_server_error_report("vehicle status", HNURI_RESP_ERR_NO_SUBSCRIBER_DEVICE);
			if(wait_delay_status(10) < 0)
				break;

			continue;
		}
		else
		{
			disconnect_to_server(sock_fd);
			send_server_error_report("vehicle status", msg_resp[0].result);
			if(wait_delay_status(10) < 0)
				break;

			continue;
		}
	}//end while

}
