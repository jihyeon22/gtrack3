
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>


#include <base/config.h>
#include <at/at_util.h>
#include <board/power.h>
#include <board/led.h>
#include <util/tools.h>
#include <util/list.h>
#include <util/debug.h>
#include <util/transfer.h>
#include "logd/logd_rpc.h"

#include <callback.h>
#include "netcom.h"
#include "katech-packet.h"
#include <at/at_util.h>

#include "data-list.h"
#include "logd/logd_rpc.h"

#include "base/sender.h"

#include "seco_obd_1.h"
#define LOG_TARGET eSVC_NETWORK

void _pkt_data_convert(const int input_data, const int out_type, char* buff)
{
	unsigned char tmp_buff[8] = {0,};
	
	int copy_to_size = 0;

	unsigned char tmp_unsigned_1_byte;
	char tmp_signed_1_byte;

	unsigned short tmp_unsigned_2_byte;
	short tmp_signed_2_byte;

	unsigned int tmp_unsigned_4_byte;
	int tmp_signed_4_byte;


	switch (out_type)
	{
		case UNSIGNED_1_BYTE:
		{
			tmp_unsigned_1_byte = (unsigned char*) input_data;
			memcpy(buff, &tmp_unsigned_1_byte, sizeof(char));
			break;
		}
		case SIGNED_1_BYTE:
		{
			tmp_signed_1_byte = (char*) input_data;
			memcpy(buff, &tmp_signed_1_byte, sizeof(char));
			break;
		}
		case UNSIGNED_2_BYTE:
		{
			tmp_unsigned_2_byte = (unsigned short*) input_data;
			memcpy(buff, &tmp_unsigned_2_byte, sizeof(short));
			break;
		}
		case SIGNED_2_BYTE:
		{
			tmp_signed_2_byte = (short*) input_data;
			memcpy(buff, &tmp_signed_2_byte, sizeof(short));
			break;
		}
		case UNSIGNED_4_BYTE:
		{
			tmp_unsigned_4_byte = (unsigned int*) input_data;
			memcpy(buff, &tmp_unsigned_4_byte, sizeof(int));
			break;
		}
		case SIGNED_4_BYTE:
		{
			tmp_signed_4_byte = (int*) input_data;
			memcpy(buff, &tmp_signed_4_byte, sizeof(int));
			break;
		}
		default:
		{
			break;
		}
	}
}

void _debug_print_report1_pkt(REPORT_DATA_1_BODY_DATA* pkt_body)
{
	printf("report 1 : mdm_date [%d] / [0x%x]\r\n", pkt_body->mdm_date, pkt_body->mdm_date);                 
	printf("report 1 : mdm_time [%d] / [0x%x]\r\n", pkt_body->mdm_time, pkt_body->mdm_time);                 
	printf("report 1 : obd_trip_num[3] [%d] / [0x%x] \r\n", pkt_body->obd_trip_num[0], pkt_body->obd_trip_num[0]); 
	printf("report 1 : obd_trip_elapsed[3] [%d] / [0x%x]\r\n", pkt_body->obd_trip_elapsed[0], pkt_body->obd_trip_elapsed[0]);      
	printf("report 1 : mdm_gps_num_of_sat [%d] / [0x%x]\r\n", pkt_body->mdm_gps_num_of_sat, pkt_body->mdm_gps_num_of_sat);       
	printf("report 1 : mdm_gps_time [%d] / [0x%x]\r\n", pkt_body->mdm_gps_time, pkt_body->mdm_gps_time);             
	printf("report 1 : mdm_gps_long [%d] / [0x%x]\r\n", pkt_body->mdm_gps_long, pkt_body->mdm_gps_long);             
	printf("report 1 : mdm_gps_lat [%d] / [0x%x]\r\n", pkt_body->mdm_gps_lat, pkt_body->mdm_gps_lat);              
	printf("report 1 : mdm_gps_attitude [%d] / [0x%x]\r\n", pkt_body->mdm_gps_attitude, pkt_body->mdm_gps_attitude);         
	printf("report 1 : mdm_gps_heading [%d] / [0x%x]\r\n", pkt_body->mdm_gps_heading, pkt_body->mdm_gps_heading);          
	printf("report 1 : mdm_gps_speed [%d] / [0x%x]\r\n", pkt_body->mdm_gps_speed, pkt_body->mdm_gps_speed);            
	printf("report 1 : obd_basic_spare_1 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_1, pkt_body->obd_basic_spare_1);        
	printf("report 1 : obd_basic_spare_2 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_2, pkt_body->obd_basic_spare_2);        
	printf("report 1 : obd_basic_spare_3 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_3, pkt_body->obd_basic_spare_3);        
	printf("report 1 : obd_basic_spare_4 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_4, pkt_body->obd_basic_spare_4);        
	printf("report 1 : obd_basic_spare_5 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_5, pkt_body->obd_basic_spare_5);        
	printf("report 1 : obd_basic_spare_6 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_6, pkt_body->obd_basic_spare_6);        
	printf("report 1 : obd_basic_spare_7 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_7, pkt_body->obd_basic_spare_7);        
	printf("report 1 : obd_basic_spare_8 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_8, pkt_body->obd_basic_spare_8);        
	printf("report 1 : obd_basic_spare_9 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_9, pkt_body->obd_basic_spare_9);        
	printf("report 1 : obd_basic_spare_10 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_10, pkt_body->obd_basic_spare_10);       
	printf("report 1 : obd_basic_spare_11 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_11, pkt_body->obd_basic_spare_11);       
	printf("report 1 : obd_basic_spare_12 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_12, pkt_body->obd_basic_spare_12);       
	printf("report 1 : obd_basic_spare_13 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_13, pkt_body->obd_basic_spare_13);       
	printf("report 1 : obd_basic_spare_14 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_14, pkt_body->obd_basic_spare_14);       
	printf("report 1 : obd_basic_spare_15 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_15, pkt_body->obd_basic_spare_15);       
	printf("report 1 : obd_basic_spare_16 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_16, pkt_body->obd_basic_spare_16);       
	printf("report 1 : obd_basic_spare_17 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_17, pkt_body->obd_basic_spare_17);       
	printf("report 1 : obd_basic_spare_18 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_18, pkt_body->obd_basic_spare_18);       
	printf("report 1 : obd_basic_spare_19 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_19, pkt_body->obd_basic_spare_19);       
	printf("report 1 : obd_basic_spare_20 [%d] / [0x%x]\r\n", pkt_body->obd_basic_spare_20, pkt_body->obd_basic_spare_20);       
	printf("report 1 : obd_sd_card_capacity [%d] / [0x%x]\r\n", pkt_body->obd_sd_card_capacity, pkt_body->obd_sd_card_capacity);     
	printf("report 1 : obd_err_code [%d] / [0x%x]\r\n", pkt_body->obd_err_code, pkt_body->obd_err_code);             
	printf("report 1 : obd_spare [%d] / [0x%x]\r\n", pkt_body->obd_spare, pkt_body->obd_spare);                
	printf("report 1 : obd_calc_load_val [%d] / [0x%x]\r\n", pkt_body->obd_calc_load_val, pkt_body->obd_calc_load_val);        
	printf("report 1 : obd_intake_map [%d] / [0x%x]\r\n", pkt_body->obd_intake_map, pkt_body->obd_intake_map);           
	printf("report 1 : obd_rpm [%d] / [0x%x]\r\n", pkt_body->obd_rpm, pkt_body->obd_rpm);                  
	printf("report 1 : obd_speed [%d] / [0x%x]\r\n", pkt_body->obd_speed, pkt_body->obd_speed);                
	printf("report 1 : obd_air_flow_rate [%d] / [0x%x]\r\n", pkt_body->obd_air_flow_rate, pkt_body->obd_air_flow_rate);        
	printf("report 1 : obd_abs_throttle_posi [%d] / [0x%x]\r\n", pkt_body->obd_abs_throttle_posi, pkt_body->obd_abs_throttle_posi);    
	printf("report 1 : obd_lambda [%d] / [0x%x]\r\n", pkt_body->obd_lambda, pkt_body->obd_lambda);               
	printf("report 1 : obd_ctrl_module_vol [%d] / [0x%x]\r\n", pkt_body->obd_ctrl_module_vol, pkt_body->obd_ctrl_module_vol);      
	printf("report 1 : obd_acc_pedal_posi [%d] / [0x%x]\r\n", pkt_body->obd_acc_pedal_posi, pkt_body->obd_acc_pedal_posi);       
	printf("report 1 : obd_engine_fule_rate [%d] / [0x%x]\r\n", pkt_body->obd_engine_fule_rate, pkt_body->obd_engine_fule_rate);     
	printf("report 1 : obd_actual_engine [%d] / [0x%x]\r\n", pkt_body->obd_actual_engine, pkt_body->obd_actual_engine);        
	printf("report 1 : obd_command_egr [%d] / [0x%x]\r\n", pkt_body->obd_command_egr, pkt_body->obd_command_egr);          
	printf("report 1 : obd_actual_egr_duty [%d] / [0x%x]\r\n", pkt_body->obd_actual_egr_duty, pkt_body->obd_actual_egr_duty);      
	printf("report 1 : obd_engine_friction [%d] / [0x%x]\r\n", pkt_body->obd_engine_friction, pkt_body->obd_engine_friction);      
	printf("report 1 : obd_engine_coolant_temp [%d] / [0x%x]\r\n", pkt_body->obd_engine_coolant_temp, pkt_body->obd_engine_coolant_temp);  
	printf("report 1 : obd_intake_air_temp [%d] / [0x%x]\r\n", pkt_body->obd_intake_air_temp, pkt_body->obd_intake_air_temp);      
	printf("report 1 : obd_catalyst_temp_1 [%d] / [0x%x]\r\n", pkt_body->obd_catalyst_temp_1, pkt_body->obd_catalyst_temp_1);      
	printf("report 1 : obd_time_engine_start [%d] / [0x%x]\r\n", pkt_body->obd_time_engine_start, pkt_body->obd_time_engine_start);    
	printf("report 1 : obd_barometic_press [%d] / [0x%x]\r\n", pkt_body->obd_barometic_press, pkt_body->obd_barometic_press);      
	printf("report 1 : obd_ambient_air_temp [%d] / [0x%x]\r\n", pkt_body->obd_ambient_air_temp, pkt_body->obd_ambient_air_temp);     
	printf("report 1 : obd_ref_torque [%d] / [0x%x]\r\n", pkt_body->obd_ref_torque, pkt_body->obd_ref_torque);           
	printf("report 1 : obd_mon_st_since_dtc_clr [%d] / [0x%x]\r\n", pkt_body->obd_mon_st_since_dtc_clr, pkt_body->obd_mon_st_since_dtc_clr); 
	printf("report 1 : obd_dist_travled_mil [%d] / [0x%x]\r\n", pkt_body->obd_dist_travled_mil, pkt_body->obd_dist_travled_mil);     
	printf("report 1 : obd_dist_travled_dtc [%d] / [0x%x]\r\n", pkt_body->obd_dist_travled_dtc, pkt_body->obd_dist_travled_dtc);     
	printf("report 1 : obd_spare_1_egr_cmd2 [%d] / [0x%x]\r\n", pkt_body->obd_spare_1_egr_cmd2, pkt_body->obd_spare_1_egr_cmd2);              
	printf("report 1 : obd_spare_2_egr_err [%d] / [0x%x]\r\n", pkt_body->obd_spare_2_egr_err, pkt_body->obd_spare_2_egr_err);              
	printf("report 1 : obd_spare_3 [%d] / [0x%x]\r\n", pkt_body->obd_spare_3, pkt_body->obd_spare_3);              
	printf("report 1 : obd_spare_4 [%d] / [0x%x]\r\n", pkt_body->obd_spare_4, pkt_body->obd_spare_4);              
	printf("report 1 : obd_spare_5 [%d] / [0x%x]\r\n", pkt_body->obd_spare_5, pkt_body->obd_spare_5);              
	printf("report 1 : obd_spare_6 [%d] / [0x%x]\r\n", pkt_body->obd_spare_6, pkt_body->obd_spare_6);              
	printf("report 1 : obd_spare_7 [%d] / [0x%x]\r\n", pkt_body->obd_spare_7, pkt_body->obd_spare_7);              
	printf("report 1 : obd_spare_8 [%d] / [0x%x]\r\n", pkt_body->obd_spare_8, pkt_body->obd_spare_8);              
	printf("report 1 : obd_spare_9 [%d] / [0x%x]\r\n", pkt_body->obd_spare_9, pkt_body->obd_spare_9);              
	printf("report 1 : obd_spare_10 [%d] / [0x%x]\r\n", pkt_body->obd_spare_10, pkt_body->obd_spare_10);             
	printf("report 1 : obd_spare_11 [%d] / [0x%x]\r\n", pkt_body->obd_spare_11, pkt_body->obd_spare_11);             
	printf("report 1 : obd_spare_12 [%d] / [0x%x]\r\n", pkt_body->obd_spare_12, pkt_body->obd_spare_12);             
	printf("report 1 : obd_spare_13 [%d] / [0x%x]\r\n", pkt_body->obd_spare_13, pkt_body->obd_spare_13);             
	printf("report 1 : obd_spare_14 [%d] / [0x%x]\r\n", pkt_body->obd_spare_14, pkt_body->obd_spare_14);             
	printf("report 1 : obd_fuel_flow_rate [%d] / [0x%x]\r\n", pkt_body->obd_fuel_flow_rate, pkt_body->obd_fuel_flow_rate);       
	printf("report 1 : obd_engine_brake_torq [%d] / [0x%x]\r\n", pkt_body->obd_engine_brake_torq, pkt_body->obd_engine_brake_torq);    
	printf("report 1 : obd_engine_brake_pwr [%d] / [0x%x]\r\n", pkt_body->obd_engine_brake_pwr, pkt_body->obd_engine_brake_pwr);     
	printf("report 1 : obd_exhaust_gas_flowrate [%d] / [0x%x]\r\n", pkt_body->obd_exhaust_gas_flowrate, pkt_body->obd_exhaust_gas_flowrate); 
	printf("report 1 : obd_acc_power [%d] / [0x%x]\r\n", pkt_body->obd_acc_power, pkt_body->obd_acc_power);            
	printf("report 1 : obd_engine_spare_1 [%d] / [0x%x]\r\n", pkt_body->obd_engine_spare_1, pkt_body->obd_engine_spare_1);       
	printf("report 1 : obd_engine_spare_2 [%d] / [0x%x]\r\n", pkt_body->obd_engine_spare_2, pkt_body->obd_engine_spare_2);       
	printf("report 1 : obd_engine_spare_3 [%d] / [0x%x]\r\n", pkt_body->obd_engine_spare_3, pkt_body->obd_engine_spare_3);       
	printf("report 1 : obd_acceleration [%d] / [0x%x]\r\n", pkt_body->obd_acceleration, pkt_body->obd_acceleration);         
	printf("report 1 : obd_cor_speed [%d] / [0x%x]\r\n", pkt_body->obd_cor_speed, pkt_body->obd_cor_speed);            
	printf("report 1 : obd_road_gradient [%d] / [0x%x]\r\n", pkt_body->obd_road_gradient, pkt_body->obd_road_gradient);        
	printf("report 1 : obd_driving_spare_1 [%d] / [0x%x]\r\n", pkt_body->obd_driving_spare_1, pkt_body->obd_driving_spare_1);      
	printf("report 1 : obd_driving_spare_2 [%d] / [0x%x]\r\n", pkt_body->obd_driving_spare_2, pkt_body->obd_driving_spare_2);      
	printf("report 1 : obd_driving_spare_3 [%d] / [0x%x]\r\n", pkt_body->obd_driving_spare_3, pkt_body->obd_driving_spare_3);      
	printf("report 1 : obd_after_spare_1 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_1, pkt_body->obd_after_spare_1);        
	printf("report 1 : obd_after_spare_2 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_2, pkt_body->obd_after_spare_2);        
	printf("report 1 : obd_after_spare_3 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_3, pkt_body->obd_after_spare_3);        
	printf("report 1 : obd_after_spare_4 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_4, pkt_body->obd_after_spare_4);        
	printf("report 1 : obd_after_spare_5 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_5, pkt_body->obd_after_spare_5);        
	printf("report 1 : obd_after_spare_6 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_6, pkt_body->obd_after_spare_6);        
	printf("report 1 : obd_after_spare_7 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_7, pkt_body->obd_after_spare_7);        
	printf("report 1 : obd_after_spare_8 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_8, pkt_body->obd_after_spare_8);        
	printf("report 1 : obd_after_spare_9 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_9, pkt_body->obd_after_spare_9);        
	printf("report 1 : obd_after_spare_10 [%d] / [0x%x]\r\n", pkt_body->obd_after_spare_10, pkt_body->obd_after_spare_10);       
}


void _debug_print_report2_pkt(REPORT_DATA_2_BODY_DATA* pkt_body)
{
	printf("report 2 :mdm_dev_id [%d]\r\n", pkt_body->mdm_dev_id);
	printf("report 2 :mdm_char_vin [%s]\r\n", pkt_body->mdm_char_vin);
	printf("report 2 :tripdata_payload [%d]\r\n", pkt_body->tripdata_payload);
	printf("report 2 :tripdata_total_time [%d]\r\n", pkt_body->tripdata_total_time);
	printf("report 2 :tripdata_driving_time [%d]\r\n", pkt_body->tripdata_driving_time);
	printf("report 2 :tripdata_stop_time [%d]\r\n", pkt_body->tripdata_stop_time);
	printf("report 2 :tripdata_driving_dist [%d]\r\n", pkt_body->tripdata_driving_dist);
	printf("report 2 :tripdata_num_of_stop [%d]\r\n", pkt_body->tripdata_num_of_stop);
	printf("report 2 :tripdata_mean_spd_w_stop [%d]\r\n", pkt_body->tripdata_mean_spd_w_stop);
	printf("report 2 :tripdata_mean_spd_wo_stop [%d]\r\n", pkt_body->tripdata_mean_spd_wo_stop);
	printf("report 2 :tripdata_acc_rate [%d]\r\n", pkt_body->tripdata_acc_rate);
	printf("report 2 :tripdata_dec_rate [%d]\r\n", pkt_body->tripdata_dec_rate);
	printf("report 2 :tripdata_cruise_rate [%d]\r\n", pkt_body->tripdata_cruise_rate);
	printf("report 2 :tripdata_stop_rate [%d]\r\n", pkt_body->tripdata_stop_rate);
	printf("report 2 :tripdata_pke [%d]\r\n", pkt_body->tripdata_pke);
	printf("report 2 :tripdata_rpa [%d]\r\n", pkt_body->tripdata_rpa);
	printf("report 2 :tripdata_mean_acc [%d]\r\n", pkt_body->tripdata_mean_acc);
	printf("report 2 :tripdata_cold_rate [%d]\r\n", pkt_body->tripdata_cold_rate);
	printf("report 2 :tripdata_warm [%d]\r\n", pkt_body->tripdata_warm);
	printf("report 2 :tripdata_hot [%d]\r\n", pkt_body->tripdata_hot);
	printf("report 2 :tripdata_fuel_usage [%d]\r\n", pkt_body->tripdata_fuel_usage);
	printf("report 2 :tripdata_fuel_eco [%d]\r\n", pkt_body->tripdata_fuel_eco);
	printf("report 2 :tripdata_trip_spare_1 [%d]\r\n", pkt_body->tripdata_trip_spare_1);
	printf("report 2 :tripdata_trip_spare_2 [%d]\r\n", pkt_body->tripdata_trip_spare_2);
	printf("report 2 :tripdata_trip_spare_3 [%d]\r\n", pkt_body->tripdata_trip_spare_3);
	printf("report 2 :tripdata_trip_spare_4 [%d]\r\n", pkt_body->tripdata_trip_spare_4);
	printf("report 2 :tripdata_trip_spare_5 [%d]\r\n", pkt_body->tripdata_trip_spare_5);
	printf("report 2 :tripdata_trip_spare_6 [%d]\r\n", pkt_body->tripdata_trip_spare_6);
	printf("report 2 :tripdata_trip_spare_7 [%d]\r\n", pkt_body->tripdata_trip_spare_7);
	printf("report 2 :tripdata_trip_spare_8 [%d]\r\n", pkt_body->tripdata_trip_spare_8);
	printf("report 2 :tripdata_trip_spare_9 [%d]\r\n", pkt_body->tripdata_trip_spare_9);
	printf("report 2 :tripdata_trip_spare_10 [%d]\r\n", pkt_body->tripdata_trip_spare_10);
	printf("report 2 :tripdata_trip_spare_11 [%d]\r\n", pkt_body->tripdata_trip_spare_11);
	printf("report 2 :tripdata_trip_spare_12 [%d]\r\n", pkt_body->tripdata_trip_spare_12);
	printf("report 2 :tripdata_trip_spare_13 [%d]\r\n", pkt_body->tripdata_trip_spare_13);
	printf("report 2 :tripdata_trip_spare_14 [%d]\r\n", pkt_body->tripdata_trip_spare_14);
	printf("report 2 :tripdata_trip_spare_15 [%d]\r\n", pkt_body->tripdata_trip_spare_15);
	printf("report 2 :tripdata_trip_spare_16 [%d]\r\n", pkt_body->tripdata_trip_spare_16);
	printf("report 2 :tripdata_trip_spare_17 [%d]\r\n", pkt_body->tripdata_trip_spare_17);
	printf("report 2 :tripdata_trip_spare_18 [%d]\r\n", pkt_body->tripdata_trip_spare_18);
	printf("report 2 :trip_start_date [%d]\r\n", pkt_body->trip_start_date);
	printf("report 2 :trip_start_time [%d]\r\n", pkt_body->trip_start_time);
	printf("report 2 :trip_end_date [%d]\r\n", pkt_body->trip_end_date);
	printf("report 2 :trip_end_time [%d]\r\n", pkt_body->trip_end_time);
	printf("report 2 :trip_spare_23 [%d]\r\n", pkt_body->trip_spare_23);
	printf("report 2 :trip_spare_24 [%d]\r\n", pkt_body->trip_spare_24);






































}

// --------------------------------------------------------------------
// packet common util
// --------------------------------------------------------------------
unsigned char get_checksum(unsigned char* buff, int buff_size)
{
	unsigned char checksum_value = 0;
	int i = 0;
	
	
	for ( i = 0 ; i < buff_size ; i ++)
	{
	//	printf("%d/%d - [%02x] , checksum_value [%d]\r\n", i, buff_size, buff[i], checksum_value);
		checksum_value += buff[i];
		
	}
	
	//printf("check sum val is [%d]\r\n", checksum_value);
	//printf("check sum val is [%d]\r\n", checksum_value);
	//printf("check sum val is [%d]\r\n", checksum_value);
	
	return checksum_value;
}

static KATCH_PKT_STAT g_pkt_stat = {0,};


// --------------------------------------------------------------------
// status api
// --------------------------------------------------------------------
int get_pkt_stat(KATCH_PKT_STAT* pkt_stat)
{
	return 0;
}

int set_pkt_stat(KATCH_PKT_STAT pkt_stat)
{
	//g_pkt_stat.svr_stat 
	return 0;
}

int katech_tools__set_svr_stat(int svr_stat)
{
	g_pkt_stat.svr_stat = svr_stat;
	return g_pkt_stat.svr_stat;
}

int katech_tools__get_svr_stat()
{
	return g_pkt_stat.svr_stat;
}

int katech_tools__get_dev_id(char* dev_id)
{
	char phonenum[AT_LEN_PHONENUM_BUFF] = {0,};
	at_get_phonenum(phonenum, AT_LEN_PHONENUM_BUFF);
	
    //strcpy(dev_id, "01224831154"); // test no : 20170901
    strcpy(dev_id, phonenum);
	// 010-9626-7299
	
	return 0;
}

int katech_tools__get_auth_key(char* auth_key)
{
	strncpy(auth_key, g_pkt_stat.auth_key, 16);
	return 0;
}

int katech_tools__set_auth_key(char* auth_key)
{
	strncpy(g_pkt_stat.auth_key, auth_key, 16);
	return 0;
}

int katech_tools__set_server_ip(char* server_ip)
{
	strncpy(g_pkt_stat.report_ip, server_ip, strlen(server_ip) );
	return 0;
}

int katech_tools__get_server_port(int server_port)
{
	g_pkt_stat.report_port = server_port ;
	return 0;
}



int katech_tools__convert_data_num(const char* data, const char offset_type, const char* off_set_val)
{
    float tmp_float_data = 0;
    float tmp_float_offset = 0;

    float offset_0_5 = 0.5;

    int return_val = -1;
/*
    if ( _chk_obd_invalid_num_data(data) == -1)
    {
        #ifdef DEBUG_MSG_CONVERT_MSG
        printf(" >> convert float : input [%s] is not number\r\n", data);
        #endif
        return 0xffffffff;
    }
*/
    if ( data != NULL )
        tmp_float_data = atof(data);
    
    if ( off_set_val != NULL)
        tmp_float_offset = atof(off_set_val);

    #ifdef DEBUG_MSG_CONVERT_MSG
    printf(" >> convert float : input [%f]\r\n", tmp_float_data);
    printf(" >> convert float : offset [%c] [%f]\r\n", offset_type, tmp_float_offset);
    #endif

    if ( offset_type == '+' )
    {
        tmp_float_data = tmp_float_data + tmp_float_offset;
    }
    else if ( offset_type == '-' )
    {
        tmp_float_data = tmp_float_data - tmp_float_offset;
    }
    else if ( offset_type == '*' )
    {
        tmp_float_data = tmp_float_data * tmp_float_offset;
    }
    else if ( offset_type == '/' )
    {
        tmp_float_data = tmp_float_data / tmp_float_offset;
    }

    tmp_float_data = tmp_float_data + offset_0_5;
    
    return_val = (int)tmp_float_data;
    
    #ifdef DEBUG_MSG_CONVERT_MSG
    printf(" >> convert float : result [%d] \r\n", return_val);
    #endif

    return return_val;
}
