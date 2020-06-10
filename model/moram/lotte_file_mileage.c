#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <util/storage.h>

#include <base/gpstool.h>
#include <base/devel.h>
#include <board/crit-data.h>
#include "logd/logd_rpc.h"

#include "lotte_gps_utill.h"
#include "lotte_geofence.h"
#include "debug.h"
#include "lotte_gpsmng.h"
#include "lotte_file_mileage.h"

void _print_mileage_status(lotte_mileage_t data)
{

	FILE *fp = NULL;
	FILE *log_fd = NULL;
	remove("/var/log/moram_mileage.log");
	fp = fopen("/var/log/moram_mileage.log", "w");
	
	if(fp == NULL)
		log_fd = stderr;
	else
		log_fd = fp;
	
	fprintf(log_fd, "moram device mileage : [%d]\n", data.mileage);

	if(fp != NULL)
		fclose(fp);
}

void _default_mileage_data(lotte_mileage_t *pdata)
{
	pdata->mileage = 0;
}

int load_mileage_file(int *file_mileage)
{
	int ret;
	lotte_mileage_t data;
	unsigned int crit_mileage = 0;
	
	memset(&data, 0x00, sizeof(lotte_mileage_t));
	ret = storage_load_file(LOTTE_DEVICE_MILEAGE_FILE, &data, sizeof(lotte_mileage_t));
	LOGD(LOG_TARGET, "load_mileage_file = [%d]\n", ret);
	if(ret != ERR_NONE)
	{
		_default_mileage_data(&data);
		ret = storage_save_file(LOTTE_DEVICE_MILEAGE_FILE, &data, sizeof(lotte_mileage_t));
		LOGI(LOG_TARGET, "load_mileage_file default file save = [%d]\n", ret);
	}

	crit_get_data_mileage(&crit_mileage);

	if(data.mileage < crit_mileage)
	{
		data.mileage = crit_mileage;
		LOGI(LOG_TARGET, "INFO : Load critical data(mileage).\n");
	}

	//Only for debugging.
	if(data.mileage > crit_mileage)
	{
		crit_set_data_mileage_write(data.mileage);
		LOGE(LOG_TARGET, "WARN : Critical data(mileage) is older than nand data.\n");
		devel_webdm_send_log("WARN : Critical data(mileage) is older than nand data.\n");
	}

	_print_mileage_status(data);

	*file_mileage = data.mileage;
	
	return 0;
}

int save_mileage_file(int mileage)
{
	int ret;
	lotte_mileage_t data;
	data.mileage = mileage;

	ret = storage_save_file(LOTTE_DEVICE_MILEAGE_FILE, &data, sizeof(lotte_mileage_t));

	crit_set_data_mileage_write(mileage);
	
	_print_mileage_status(data);
	
	return ret;
}

