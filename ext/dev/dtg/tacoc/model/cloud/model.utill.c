#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#include <wrapper/dtg_log.h>
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_packet.h"
#include "dtg_data_manage.h"
#include "dtg_utill.h"
#include "dtg_net_com.h"

#define REGISTRATION	1
#define DEREGISTRATION	0
#define INVALID_REG		-1
static int regist_flag = 0;

REGIST_DATA regist_data = {
	.xml_starter = "<xml>\n",
	.xml_header_starter = "<H>\n",
	.xml_header_ender = "</H>\n",
	.xml_body_starter = "<B>\n",
	.xml_imei_starter = "<I>",
	.imei = {0,},
	.xml_imei_ender = "</I>\n",
	.xml_dtgenv_starter = "<D>",
	.dtg_env ={0,},
	.xml_dtgenv_ender = "</D>\n",
	.xml_dtgstat_starter = "<DS>",
	.dtg_stat = {'0', 'x', '0', '0', '0', '0'},
	.xml_dtgstat_ender = "</DS>\n",
	.xml_dtgrl_starter = "<RL>",
	.dtg_rl = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0'},
	.xml_dtgrl_ender = "</RL>\n",
	.xml_dtgvrn_starter = "<VR>",
	.dtg_vrn = {'#', '#', '#', '#', '0', '0', '#', '#', '0', '0', '0', '0'},
	.xml_dtgvrn_ender = "</VR>\n",
	.xml_body_ender = "</B>\n",
	.xml_ender = "</xml>\n",
};

void send_device_registration()
{
	int ret, fd;
	UPDATE_INFO_RESPONSE Packet_update_info;
	char dtg_status[128] = {0};
	char *ptr;

	if (regist_flag == INVALID_REG) {
		return ;					// 
	}

	strncpy(regist_data.imei, atcmd_get_imei(),15);
	if (getenv(DTG_ENV_VAL))
		strncpy(regist_data.dtg_env, getenv(DTG_ENV_VAL),2);
	else
		strncpy(regist_data.dtg_env, "-9",2);

	do {
		fd = open("/var/dtg_status", O_RDONLY , 0644);
		sleep(1);
	} while (fd < 0 && errno != ENOENT);
	if (fd > 0) {
		read(fd, dtg_status, 128);
		ptr = strtok(dtg_status, "/");
		strncpy(regist_data.dtg_stat, ptr, 6);
		ptr = strtok(NULL, "/");
		strncpy(regist_data.dtg_rl, ptr, 10);
		ptr = strtok(NULL, "/");
		if (ptr != NULL)
			strncpy(regist_data.dtg_vrn, ptr, 12);
	}

	ret = send_to_reg_server(MSG_TYPE_REGISTRATION, &regist_data, 
			sizeof(REGIST_DATA), 0, &Packet_update_info);
	if(ret == NET_SUCCESS_OK) {
		regist_flag = REGISTRATION;
		DTG_LOGI("DEVICE REGISTRATION OK[%d]", ret);
	}else {
		DTG_LOGE("DEVICE REGISTRATION FAILURE[%d]", ret);
	}
}

void send_device_de_registration()
{
	int ret;

	if (regist_flag != REGISTRATION) {
		regist_flag = INVALID_REG;
		return ;
	}

	ret = send_to_reg_server(MSG_TYPE_DE_REGISTRATION, NULL, 0, 0, NULL);
	if (ret == NET_SUCCESS_OK) {
		DTG_LOGI("DEVICE DEREGISTRATION OK[%d]", ret);
	} else {
		DTG_LOGE("DEVICE DEREGISTRATION FAILURE[%d]", ret);
	}
}
