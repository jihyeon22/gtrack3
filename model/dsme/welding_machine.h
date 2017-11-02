#pragma once

typedef struct {
	int enable; //IVSSTP
	int interval;
	time_t prev_time;
}IVS_cmd_t;

typedef struct {
	int enable;
	int interval;
	time_t inv_prev_time;
	time_t t_prev_time;
	//int spot_inv_data;
	//int spot_t_data;
}InV_T_cmd_t;

typedef struct {
	int initial_flag;
	char ID[24];
}ID_cmd_t;

typedef struct {
	int run_flag;
}IVSSTP_cmd_t;

typedef struct {
	int enable;
	char user_cmd[128];
}ADDCLT_cmd_t;

typedef struct {

	IVS_cmd_t     IVS_cmd;
	InV_T_cmd_t     InV_T_cmd;
	ID_cmd_t      ID_cmd;
	IVSSTP_cmd_t  IVSSTP_cmd;
	ADDCLT_cmd_t  ADDCLT_cmd;
}WELDING_MACHINE_CMD_t;

typedef enum command_type command_type_t;
enum command_type{
	eID_CMD_CODE	  = 0,
	eIVS_CMD_CODE	  = 1,
	eInV_CMD_CODE     = 2,
	eT_CMD_CODE       = 3,
	eUnknown_CMD_CODE = 4,
};

int welding_machine_process(int debug_num);
void power_off_collect_stop();
int welding_machine_config_setup();
char *get_welding_machine_id();

