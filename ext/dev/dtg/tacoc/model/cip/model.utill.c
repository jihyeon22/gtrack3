#include <stdio.h>
#include <stdlib.h>
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
#include <wrapper/dtg_atcmd.h>
#include "dtg_type.h"
#include "dtg_packet.h"
#include "dtg_data_manage.h"
#include "dtg_utill.h"
#include "dtg_net_com.h"

#define REGISTRATION	1
#define DEREGISTRATION	0
#define INVALID_REG		-1
int regist_flag = 0;

int cip_regist_flag = DEREGISTRATION;
int mds_regist_flag = DEREGISTRATION;

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

	if (cip_regist_flag == DEREGISTRATION) {
		ret = send_to_server(MSG_TYPE_REGISTRATION, &regist_data, 
				sizeof(REGIST_DATA), 0, &Packet_update_info, MDS_IP);
		if(ret == NET_SUCCESS_OK) {
			cip_regist_flag = REGISTRATION;
			DTG_LOGI("DEVICE CIP REGISTRATION OK[%d]", ret);
		} else {
			DTG_LOGE("DEVICE CIP REGISTRATION FAILURE[%d]", ret);
		}
	}

	if (mds_regist_flag == DEREGISTRATION) {
		ret = send_to_server(MSG_TYPE_REGISTRATION, &regist_data, 
				sizeof(REGIST_DATA), 0, &Packet_update_info, CIP_IP);
		if(ret == NET_SUCCESS_OK) {
			mds_regist_flag = REGISTRATION;
			DTG_LOGI("DEVICE MDS REGISTRATION OK[%d]", ret);
		}else {
			DTG_LOGE("DEVICE MDS REGISTRATION FAILURE[%d]", ret);
		}
	}
}

void send_device_de_registration()
{
#ifdef TEST_JW
	int fd = open("/system/mds/system/bin/cip.dereg.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
	time_t cur_time;
	time(&cur_time);
	char tmp[64] = {0};
	sprintf(tmp, "++ %s \n", ctime(&cur_time));
	write(fd, tmp, strlen(tmp));
#endif
	int ret;

#ifdef TEST_JW
	time(&cur_time);
	memset(tmp, 0, 64);
	sprintf(tmp, "-- %s \n", ctime(&cur_time));
	write(fd, tmp, strlen(tmp));
	close(fd);
#endif

	if (cip_regist_flag == REGISTRATION) {
		ret = send_to_server(MSG_TYPE_DE_REGISTRATION, NULL, 0, 0, NULL, MDS_IP);
		if (ret == NET_SUCCESS_OK) {
			cip_regist_flag = DEREGISTRATION;
			DTG_LOGI("DEVICE CIP DEREGISTRATION1 OK[%d]", ret);
		} else {             
			DTG_LOGE("DEVICE CIP DEREGISTRATION1 FAILURE[%d]", ret);
		}
	}

	if (mds_regist_flag == REGISTRATION) {
		ret = send_to_server(MSG_TYPE_DE_REGISTRATION, NULL, 0, 0, NULL, CIP_IP);
		if (ret == NET_SUCCESS_OK) {
			mds_regist_flag = DEREGISTRATION;
			DTG_LOGI("DEVICE MDS DEREGISTRATION0 OK[%d]", ret);
		} else {             
			DTG_LOGE("DEVICE MDS DEREGISTRATION0 FAILURE[%d]", ret);
		}
	}

}

