#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "lila_packet.h"
#include "lila_tools.h"
#include "dtg_gtrack_tool.h"

void lila_dtg_debug__print_frame_header(LILA_PKT_FRAME__HEADER_T* p_frame_header)
{
    printf("--------------------------- frame header -------------------------------\r\n");
    printf("  - pkt_prefix [0x%04x]\r\n", p_frame_header->pkt_prefix );
    printf("  - dev_id [0x%x]\r\n", p_frame_header->dev_id);
    printf("  - phone_no [%d] / [0x%x]\r\n", p_frame_header->phone_no,p_frame_header->phone_no);
    printf("  - pkt_command [%s]\r\n", lila_tools__conv_buff_to_str(p_frame_header->pkt_command, 4));
    printf("  - idx [%d]\r\n", p_frame_header->idx);
    printf("  - data_cnt [%d]\r\n", p_frame_header->data_cnt);
    printf("  - data_len [%d]\r\n", p_frame_header->data_len);
    printf("-----------------------------------------------------------------------\r\n");
}


void lila_dtg_debug__print_dtg_info(LILA_PKT_DATA__DTG_INFO_T* p_dtg_info)
{
    printf("--------------------------- dtg info pkt -------------------------------\r\n");
    printf("  - reserved_1 [%s]\r\n", p_dtg_info->reserved_1);
    printf("  - reserved_2 [%s]\r\n", p_dtg_info->reserved_2);
    printf("  - driver_name [%s]\r\n", p_dtg_info->driver_name);
    // printf("  - driver_code [%d]\r\n", lila_tools__conv_buff_to_str(p_dtg_info->driver_code, 18)); TODO: fix me
    printf("  - car_regi_no [0x%s]\r\n", p_dtg_info->car_regi_no);
    printf("  - car_code [%s]\r\n", lila_tools__conv_buff_to_str(p_dtg_info->car_code , 17));
    printf("  - company_name [%s]\r\n", p_dtg_info->company_name);
    printf("  - company_regi_no_1 [%s]\r\n", lila_tools__conv_buff_to_str(p_dtg_info->company_regi_no_1, 10));
    printf("  - company_regi_no_2 [%s]\r\n", p_dtg_info->company_regi_no_2);
    printf("  - serial_no [%s]\r\n", p_dtg_info->serial_no);
    printf("  - model_no [%s]\r\n", lila_tools__conv_buff_to_str(p_dtg_info->model_no, 20));
    printf("  - k_factor [%d] \r\n", p_dtg_info->k_factor);
    printf("  - rpm_factor [%d] \r\n", p_dtg_info->rpm_factor);
    printf("  - reserved_3 [%d] \r\n", p_dtg_info->reserved_3);
    printf("  - firmware_ver [%s] \r\n", p_dtg_info->firmware_ver);
    printf("  - reserved_4 [%s] \r\n", p_dtg_info->reserved_4);
    printf("  - reserved_5 [%s] \r\n", p_dtg_info->reserved_5);
    printf("  - reserved_6 [%s] \r\n", p_dtg_info->reserved_6);
    printf("  - reserved_7 [%s] \r\n", p_dtg_info->reserved_7);
    printf("  - reserved_8 [%s] \r\n", p_dtg_info->reserved_8);
    printf("  - reserved_9 [%s] \r\n", p_dtg_info->reserved_9);
    printf("  - reserved_10 [%s] \r\n", p_dtg_info->reserved_10);
    printf("-----------------------------------------------------------------------\r\n");
}

void lila_dtg_debug__print_frame_tail(LILA_PKT_FRAME__TAIL_T* p_frame_tail)
{
    printf("--------------------------- frame tail -------------------------------\r\n");
    printf("  - crc [%d]\r\n", p_frame_tail->crc);
    printf("  - pkt_suffix [0x%04x]\r\n", p_frame_tail->pkt_suffix );
    printf("-----------------------------------------------------------------------\r\n");
}

void lila_dtg_debug__print_tacom_std_hdr(tacom_std_hdr_t* p_current_std_hdr)
{
    printf("--------------------------- tacom std hdr-------------------------------\r\n");
	printf("p_current_std_hdr->vehicle_model [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_hdr->vehicle_model,20));
	printf("p_current_std_hdr->vehicle_id_num [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_hdr->vehicle_id_num,17));
	printf("p_current_std_hdr->vehicle_type [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_hdr->vehicle_type,2));
	printf("p_current_std_hdr->registration_num [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_hdr->registration_num,12));
	printf("p_current_std_hdr->business_license_num [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_hdr->business_license_num,10));
	printf("p_current_std_hdr->driver_code [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_hdr->driver_code,18));
    printf("-----------------------------------------------------------------------\r\n");
}

void lila_dtg_debug__print_tacom_std_data(tacom_std_data_t* p_current_std_data)
{
    printf("--------------------------- tacom std data-------------------------------\r\n");
    printf("p_current_std_data->day_run_distance [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->day_run_distance, 4));			// 일일 주행거리
    printf("p_current_std_data->cumulative_run_distance [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->cumulative_run_distance, 7));	// 누적주행거리
    printf("p_current_std_data->date_time [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->date_time, 14));					// 데이터발생 일시
    printf("p_current_std_data->speed [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->speed, 3));						// 속도
    printf("p_current_std_data->rpm [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->rpm, 4));						// RPM
    printf("p_current_std_data->bs [%c]\r\n", p_current_std_data->bs); 							//))브레이크 신호
    printf("p_current_std_data->gps_x [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->gps_x, 9));						// 차량위치 (X)
    printf("p_current_std_data->gps_y [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->gps_y, 9));						// 차량위치 (Y)
    printf("p_current_std_data->azimuth [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->azimuth, 3));					// 방위각
    printf("p_current_std_data->accelation_x [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->accelation_x, 6));				// 가속도 (Vx)
    printf("p_current_std_data->accelation_y [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->accelation_y, 6));				// 가속도 (Vy)
    printf("p_current_std_data->status [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->status, 2));						// 상태코드
    printf("p_current_std_data->day_oil_usage [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->day_oil_usage, 9));				// 일일 유류사용량
    printf("p_current_std_data->cumulative_oil_usage [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->cumulative_oil_usage, 9));		// 누적 유류 사용량
    printf("p_current_std_data->temperature_A [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->temperature_A, 5));
    printf("p_current_std_data->temperature_B [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->temperature_B, 5));
    printf("p_current_std_data->residual_oil [%s]\r\n", lila_tools__conv_buff_to_str(p_current_std_data->residual_oil, 7));				// 유류잔량
    printf("-----------------------------------------------------------------------\r\n");
}

void lila_dtg_debug__print_dtg_data(LILA_PKT_DATA__DATA_T* p_dtg_data)
{
    printf("--------------------------- dtg info data -------------------------------\r\n");
    printf(" - utc_time : [%d]\r\n", p_dtg_data->utc_time);                 // Time 	4	u32 
    printf(" - speed : [%d]\r\n", p_dtg_data->speed);                    // Speed 	1	u8 
    printf(" - speed_float : [%d]\r\n", p_dtg_data->speed_float);              // Speed Float 	1	u8 
    printf(" - rpm : [%d]\r\n", p_dtg_data->rpm);                     // RPM 	2	u16 
    printf(" - car_signal : [%d]\r\n", p_dtg_data->car_signal);              // Signal 	2	u16
    printf(" - car_status : [%d]\r\n", p_dtg_data->car_status);              // Status 	2	u16
    printf(" - gps_lat : [%d]\r\n", p_dtg_data->gps_lat);                   // Latitude 	4	u32 
    printf(" - gps_lon : [%d]\r\n", p_dtg_data->gps_lon);                   // Longitude 	4	u32 
    printf(" - gps_azimuth : [%d]\r\n", p_dtg_data->gps_azimuth);             // Azimuth 	2	u16 
    printf(" - gps_status : [%d]\r\n", p_dtg_data->gps_status);               // GPS Status 	1	char
    printf(" - gps_speed : [%d]\r\n", p_dtg_data->gps_speed);                // GPS Speed 	1	u8 
    printf(" - acceleration_x : [%d]\r\n", p_dtg_data->acceleration_x);          // 가속도 Vx 	2	s16 
    printf(" - acceleration_y : [%d]\r\n", p_dtg_data->acceleration_y);          // 가속도 Vy 	2	s16 
    printf(" - trip_cnt : [%d]\r\n", p_dtg_data->trip_cnt);                 // Trip count 	1	u8 
    printf(" - driver_no : [%d]\r\n", p_dtg_data->driver_no);                // Driver No 	1	u8 
    printf(" - modem_rssi : [%d]\r\n", p_dtg_data->modem_rssi);               // RSSI 	1	u8
    printf(" - reserved_1 : [%d]\r\n", p_dtg_data->reserved_1);               // reserved 	1	u8
    printf(" - cur_distance : [%d]\r\n", p_dtg_data->cur_distance);             // 주행거리 	4	float 
    printf(" - day_distance : [%d]\r\n", p_dtg_data->day_distance);             // 일주행거리 	4	float 
    printf(" - total_distance : [%d]\r\n", p_dtg_data->total_distance);           // 총주행거리 	4	float 
    printf(" - cur_fuel_consumption : [%d]\r\n", p_dtg_data->cur_fuel_consumption);     // 연료소모량 	4	float 
    printf(" - day_fuel_consumption : [%d]\r\n", p_dtg_data->day_fuel_consumption);     // 일연료소모량 	4	float 
    printf(" - total_fuel_consumption : [%d]\r\n", p_dtg_data->total_fuel_consumption);   // 총연료소모량 	4	float 
    printf(" - batt_volt : [%d]\r\n", p_dtg_data->batt_volt);               // Battery volt 	2	u16
    printf(" - adas_aebs : [%d]\r\n", p_dtg_data->adas_aebs);               // AEBS	1	u8
    printf(" - adas_ldw : [%d]\r\n", p_dtg_data->adas_ldw);                // LDW	1	u8
    printf(" - temp_1 : [%d]\r\n", p_dtg_data->temp_1);                  // Temp1	2	s16
    printf(" - temp_2 : [%d]\r\n", p_dtg_data->temp_2);                  // Temp2	2	s16
    printf("-----------------------------------------------------------------------\r\n");

}
