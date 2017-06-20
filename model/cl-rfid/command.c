#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <include/defines.h>
#include <base/config.h>
#include <base/sender.h>
#include <base/mileage.h>
#include <base/gpstool.h>
#include <base/thread.h>
#include <board/power.h>
#include <util/poweroff.h>
#include <util/validation.h>
#include <util/tools.h>
#include "config.h"
#include "logd/logd_rpc.h"
#include "cl_mdt_pkt.h"
#include "netcom.h"
#include "validation.h"
#include "command.h"
#include "geofence.h"
#include "section.h"

#include "board/board_system.h"

#define LOG_TARGET eSVC_MODEL

static int is_settting_gio = 0;

int _geofence2packet(CL_ZONE *dest, geo_fence_setup_t *src, int zid)
{
	if(src == NULL)
	{
		return -1;
	}

	if(src->enable == eGEN_FENCE_DISABLE)
	{
		dest->zone_type = 'N';
	}
	else
	{
		switch(src->setup_fence_status)
		{
			case eFENCE_SETUP_ENTRY:
				dest->zone_type = 'E';
				break;
			case eFENCE_SETUP_EXIT:
				dest->zone_type = 'D';
				break;
			case eFENCE_SETUP_ENTRY_EXIT:
				dest->zone_type = 'B';
				break;
			default:
				dest->zone_type = 'N';
		}
	}

	tools_lftoa_19(dest->lat, sizeof(dest->lat), "%09.05f", src->latitude);
	tools_lftoa_19(dest->lon, sizeof(dest->lon), "%09.05f", src->longitude);

	tools_itoa_11(dest->distance, sizeof(dest->distance), "%04d", src->range);
	tools_itoa_11(dest->zone_id, sizeof(dest->zone_id), "%02d", zid);

	return 0;
}

int parse_cmd(char *data, char **model_argv, char **next)
{
	char *base = NULL;
	int model_argc = 0;
	int max_str_cmd = 100;
	
	if(data == NULL)
	{
		printf("data is null!\n");
		*next = NULL;
		return model_argc;
	}
	
	base = data;

	while(model_argc <= 32 && (*base)!=0 && max_str_cmd--) {
		switch(*base) {
			case '(':
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			case ')':
				*base = '\0';
				max_str_cmd = 0;
				break;
			case ',':
				*base = '\0';
				model_argv[model_argc] = base + 1;
				model_argc++;
				break;
			default:
				break;
		}
		base++;
	}
	
	*next = base;
	return model_argc;
}

int _do_cmd_msi(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{
	int port = 0;
	paramMsi_t param_msi =
	{
		.ip = {0},
		.port = 0,
	};
	configurationModel_t *conf = get_config_model();

	if(model_argc  != 3)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s %s\n", CMD_MSI, model_argv[1], model_argv[2]);

	if(validation_check_ip(model_argv[1], DEFINES_IP_LEN) < 0)
	{
		return -1;
	}

	port = atoi(model_argv[2]);

	if(validation_model_port(port) < 0)
	{
		return -1;
	}
	
	strncpy(param_msi.ip, model_argv[1], sizeof(param_msi.ip)-1);
	param_msi.port = port;

	if(save_config_user("user:report_ip", param_msi.ip) < 0)
	{
		return -1;
	}

	char buff_config[6] = {0};
	snprintf(buff_config, sizeof(buff_config), "%u", param_msi.port);
	if(save_config_user("user:report_port", buff_config) < 0)
	{
		return -1;
	}

	snprintf(conf->model.report_ip, sizeof(conf->model.report_ip), "%s", param_msi.ip);
	conf->model.report_port = param_msi.port;

	if(make_msi_packet(pbuf, len, param_msi.ip, param_msi.port) < 0)
	{
		return -1;
	}

	return 0;
}

int _do_cmd_mit(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{
	unsigned int interval = 0;
	unsigned int max_packet = 0;
	paramMit_t param_mit =
	{
		.interval = 0,
		.max_packet = 0,
	};
	configurationModel_t *conf = get_config_model();

	if(model_argc  != 3)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s %s\n", CMD_MIT, model_argv[1], model_argv[2]);
	
	interval = atoi(model_argv[1]);

	if(validation_model_interval(interval) < 0)
	{
		return -1;
	}
	
	max_packet = atoi(model_argv[2]);

	if(validation_model_maxpacket(max_packet) < 0)
	{
		return -1;
	}
	
	param_mit.interval = interval;
	param_mit.max_packet = max_packet;

	char buff_config[6] = {0};
	snprintf(buff_config, sizeof(buff_config), "%u", param_mit.interval);
	if(save_config_user("user:interval_time", buff_config) < 0)
	{
		return -1;
	}

	snprintf(buff_config, sizeof(buff_config), "%u", param_mit.max_packet);
	if(save_config_user("user:max_packet", buff_config) < 0)
	{
		return -1;
	}

	conf->model.interval_time = param_mit.interval;
	conf->model.max_packet = param_mit.max_packet;
	
	thread_network_set_warn_timeout(conf->model.interval_time*2);

	if(make_mit_packet(pbuf, len, param_mit.interval, param_mit.max_packet) < 0)
	{
		return -1;
	}
	
	return 0;
}

int _do_cmd_gio(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{
	CL_GIO_BODY gio_body;
	float tmp_lat = 0;
	float tmp_lon = 0;
	int tmp_dist = 0;
	int tmp_zid = 0;

	geo_fence_setup_t geo_fence_data;

	if(model_argc  != 6)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s %s %s %s %s\n", CMD_GIO, model_argv[1], model_argv[2],model_argv[3],
		model_argv[4],model_argv[5]);

	switch(model_argv[1][0])
	{
		case 'N':
			geo_fence_data.enable = eGEN_FENCE_DISABLE;
			geo_fence_data.setup_fence_status = eFENCE_SETUP_UNKNOWN;
			break;
		case 'E':
			geo_fence_data.enable = eGEN_FENCE_ENABLE;
			geo_fence_data.setup_fence_status = eFENCE_SETUP_ENTRY;
			break;
		case 'D':
			geo_fence_data.enable = eGEN_FENCE_ENABLE;
			geo_fence_data.setup_fence_status = eFENCE_SETUP_EXIT;
			break;
		case 'B':
			geo_fence_data.enable = eGEN_FENCE_ENABLE;
			geo_fence_data.setup_fence_status = eFENCE_SETUP_ENTRY_EXIT;
			break;
		default:
			return -1;
			;
	}

	tmp_lat = atof(model_argv[2]);
	tmp_lon = atof(model_argv[3]);
	geo_fence_data.latitude = tmp_lat;
	geo_fence_data.longitude = tmp_lon;

	tmp_dist = atoi(model_argv[4]);
	geo_fence_data.range = tmp_dist;

	tmp_zid = atoi(model_argv[5]);

	if(set_geo_fence_setup_info(tmp_zid, &geo_fence_data) < 0)
	{
		return -1;
	}

	gio_body.status = 'Y';
	_geofence2packet(&gio_body.z, &geo_fence_data, tmp_zid);
	
	if(make_gio_packet(pbuf, len, &gio_body) < 0)
	{
		return -1;
	}
	
	return 0;
}

int _do_cmd_css(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{
	unsigned int tmp_stop_time = 0;
	configurationModel_t *conf = get_config_model();

	if(model_argc  != 2)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s\n", CMD_CSS, model_argv[1]);

	tmp_stop_time = atoi(model_argv[1]);

	char buff_config[8] = {0};
	snprintf(buff_config, sizeof(buff_config), "%u", tmp_stop_time);
	if(save_config_user("user:stop_time", buff_config) < 0)
	{
		return -1;
	}

	conf->model.stop_time = tmp_stop_time;
	
	if(make_css_packet(pbuf, len, tmp_stop_time) < 0)
	{
		return -1;
	}	

	return 0;
}

int _do_cmd_mir(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{
	unsigned int m_kmh[11] = {0};
	int i = 0;

	if(model_argc  != 12)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s %s %s %s %s %s %s %s %s %s %s\n", CMD_MIR, model_argv[1], model_argv[2],
		model_argv[3],model_argv[4],model_argv[5],model_argv[6],model_argv[7],model_argv[8],model_argv[9],
		model_argv[10],model_argv[11]);

	for(i=0 ; i< 11; i++)
	{
		m_kmh[i] = atoi(model_argv[i+1]);
		section_setup_set(i, m_kmh[i]);
	}

	section_setup_to_config();

	if(make_mir_packet(pbuf, len, m_kmh) < 0)
	{
		return -1;
	}	

	return 0;
}

int _do_cmd_mgz(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{
	CL_GIO_BODY gio_body;
	geo_fence_setup_t geo_fence_data;
	int tmp_zid = 0;

	if(model_argc  != 2)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s\n", CMD_MGZ, model_argv[1]);
	
	tmp_zid = atoi(model_argv[1]);

	if(get_geo_fence_setup_info(tmp_zid, &geo_fence_data) < 0)
	{
		return -1;
	}

	gio_body.status = 'Y';
	if(_geofence2packet(&(gio_body.z), &geo_fence_data, tmp_zid) < 0)
	{
		return -1;
	}

	if(make_mgz_packet(pbuf, len, &gio_body) < 0)
	{
		return -1;
	}
	
	return 0;
}

int _do_cmd_cp0(char **model_argv, int model_argc)
{
	if(model_argc  != 3)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s %s\n", CMD_CPO, model_argv[1], model_argv[2]);

	return 0;
}

int _do_cmd_rst(char **model_argv, int model_argc)
{
	if(model_argc  != 2)
	{
		return -1;
	}
	
	LOGT(LOG_TARGET, "CMD : %s %s\n", CMD_RST, model_argv[1]);

	mileage_write();
	poweroff("Reset by CMD", sizeof("Reset by CMD"));

	return 0;
}

int _do_cmd_mst(char **model_argv, int model_argc, unsigned char **pbuf, unsigned short *len)
{	
	int i = 0;
	
	if(model_argc  != 2)
	{
		return -1;
	}

	LOGT(LOG_TARGET, "CMD : %s %s\n", CMD_MST, model_argv[1]);

	switch(model_argv[1][0])
	{
		case '1':
		{
			configurationModel_t *config = get_config_model();
			CL_MST_BODY mst_body =
			{
				.interval_dist = "00000",
				.acc_time = "00000",
				.park_time = "0000000",
				.out1 = '0',
				.out2 = '0',
				.out3 = '0',
				.batt_type = '0',
				.batt_level = "000",
				.door_mode = '0',
				.door_active = '0',
				.run_mode = '0',
				.zone_count = "10",
			};
			
			gpsData_t cur_gps;

			gps_get_curr_data(&cur_gps);

			strncpy(mst_body.sw_ver, SW_VER, sizeof(mst_body.sw_ver));
			strncpy(mst_body.hw_ver, HW_VER, sizeof(mst_body.hw_ver));

			strncpy(mst_body.server_ip, config->model.report_ip, sizeof(mst_body.server_ip));
			tools_itoa_11(mst_body.server_port, sizeof(mst_body.server_port), "%05d", config->model.report_port);

			tools_itoa_11(mst_body.interval_time, sizeof(mst_body.interval_time), "%05d", config->model.interval_time);
			tools_itoa_11(mst_body.max_packet, sizeof(mst_body.max_packet), "%02d", config->model.max_packet);

			tools_itoa_11(mst_body.speed, sizeof(mst_body.speed), "%03d", cur_gps.speed);

			if(power_get_ignition_status() == POWER_IGNITION_ON)
			{
				mst_body.acc_flag = '1';
			}
			else
			{
				mst_body.acc_flag = '0';
			}

			tools_itoa_11(mst_body.stop_time, sizeof(mst_body.stop_time), "%07d", config->model.stop_time);

			geo_fence_setup_t geo_fence_data;

			for(i=0 ;i< 10; i++)
			{
				if(get_geo_fence_setup_info(i, &geo_fence_data) < 0)
				{
					continue;
				}

				_geofence2packet(&(mst_body.zone_data[i]), &geo_fence_data, i);
			}
			
			if(make_mst_packet(pbuf, len, &mst_body) < 0)
			{
				return -1;
			}
			break;
		}
		case '2':
			break;
		case '3':
			break;
		default:
			;
	}
	
	return 0;
}

int do_cmd(char **model_argv, int model_argc, char *buf_resp, int buf_size)
{
	unsigned short size_resp = 0;
	unsigned char *pbuf = NULL;
	
	if(model_argc == 0)
	{
		printf("error : argc is 0\n");
		return -1;
	}
	
	if(model_argv[0] == NULL)
	{
		printf("error : argv is NULL\n");
		return -1;
	}
	
	if(strncmp(model_argv[0], CMD_MSI, sizeof(CMD_MSI)) == 0)
	{
		printf("MSI\n");
		if(_do_cmd_msi(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}
	}
	
	else if(strncmp(model_argv[0], CMD_MIT, sizeof(CMD_MIT)) == 0)
	{
		printf("MIT\n");
		if(_do_cmd_mit(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}
	}

	else if(strncmp(model_argv[0], CMD_GIO, sizeof(CMD_GIO)) == 0)
	{
		printf("GIO\n");
		if(_do_cmd_gio(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}		
		is_settting_gio = 1;
	}

	else if(strncmp(model_argv[0], CMD_CSS, sizeof(CMD_CSS)) == 0)
	{
		printf("CSS\n");
		if(_do_cmd_css(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}
	}

	else if(strncmp(model_argv[0], CMD_MIR, sizeof(CMD_MIR)) == 0)
	{
		printf("MIR\n");
		if(_do_cmd_mir(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}
	}

	else if(strncmp(model_argv[0], CMD_RST, sizeof(CMD_RST)) == 0)
	{
		printf("RST\n");
		if(_do_cmd_rst(model_argv, model_argc) < 0)
		{
			return -1;
		}
	}

	else if(strncmp(model_argv[0], CMD_CPO, sizeof(CMD_CPO)) == 0)
	{
		printf("CP0\n");
		if(_do_cmd_cp0(model_argv, model_argc) < 0)
		{
			return -1;
		}
	}
	
	else if(strncmp(model_argv[0], CMD_MGZ, sizeof(CMD_MGZ)) == 0)
	{
		printf("MGZ\n");
		if(_do_cmd_mgz(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}		
	}

	else if(strncmp(model_argv[0], CMD_MST, sizeof(CMD_MST)) == 0)
	{
		printf("MST\n");
		if(_do_cmd_mst(model_argv, model_argc, &pbuf, &size_resp) < 0)
		{
			return -1;
		}		
	}
	
	else
	{
		printf("Can't find [%s] cmd\n", model_argv[0]);
		return -1;
	}
	
	if(pbuf == NULL)
	{
		return -1;
	}

	if(size_resp == 0 || size_resp > buf_size)
	{
		free(pbuf);
		return -1;
	}

	memcpy(buf_resp, pbuf, size_resp);
	free(pbuf);
	
	return size_resp;
}

int process_cmd(char *data)
{
	int model_argc = 0;
	char *model_argv[32] = {0};
	char *base = 0;
	int i = 0;
	int res_size = 0;

	bufData_t buf_resp =
	{
		.buf = {0},
		.used_size = 0,
	};
	
	base = data;

	is_settting_gio = 0;
	
	do
	{
		memset(model_argv, 0, sizeof(model_argv));

		model_argc = parse_cmd(base, model_argv, &base);

		res_size = do_cmd(model_argv, model_argc, buf_resp.buf + buf_resp.used_size, sizeof(buf_resp.buf) - buf_resp.used_size);
		
		if(res_size > 0)
		{
			buf_resp.used_size += res_size;
		}

		for(i = 0; i < model_argc; i++)
		{
			printf("%d %s\n", i, model_argv[i]);
		}
	}
	while(model_argc > 0);
	
	if(is_settting_gio == 1)
	{
		printf("save gio!\n");
		save_geo_fence_setup_info();
	}
	
	printf("[%s] (%d)\n", buf_resp.buf, buf_resp.used_size);

	if(buf_resp.used_size > 0)
	{
		sender_add_data_to_buffer(PACKET_TYPE_RAW, &buf_resp, ePIPE_2);
	}

	return buf_resp.used_size;
}

