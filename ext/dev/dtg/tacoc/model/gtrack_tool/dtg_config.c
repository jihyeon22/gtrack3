#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include <board/board_system.h>


#include "dtg_type.h"
#include "config.h"


void set_dtg_report_period(int num)
{
	return 0;
}
void set_dtg_create_period(int num)
{
	return 0;
}

int get_dtg_report_period()
{
	int interval_time;
	int ign_on = power_get_ignition_status();
	configurationModel_t *conf = get_config_model();

	if(ign_on == POWER_IGNITION_OFF)
		interval_time = conf->model.dtg_report_interval_keyoff;
	else
		interval_time = conf->model.dtg_report_interval_keyon;

	//printf("report interval_time = [%d]\n", interval_time);
	return interval_time;
}


int get_dtg_create_period()
{
	int interval_time;
	int ign_on = power_get_ignition_status();
	configurationModel_t *conf = get_config_model();

	if(ign_on == POWER_IGNITION_OFF)
		interval_time = conf->model.dtg_collect_interval_keyoff;
	else
		interval_time = conf->model.dtg_collect_interval_keyon;

	//printf("collection interval_time = [%d]\n", interval_time);
	return interval_time;
}