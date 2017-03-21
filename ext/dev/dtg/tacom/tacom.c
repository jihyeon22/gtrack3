#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <math.h>
#include <termios.h>
#include <errno.h>

#include <tacom/tacom_inc.h>
#include <tacom/tacom_std_protocol.h>
#include <wrapper/dtg_log.h>


#include <tacom/tacom_internal.h>

#include <mdsapi/mds_api.h>

static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
int dtg_uart_fd = -1;

pthread_mutex_t cmd_mutex = PTHREAD_MUTEX_INITIALIZER;

static TACOM* p_tacom_contex = NULL;

static struct tacom_list tm_list = {
#if defined(DEVICE_MODEL_SINHUNG)
		.name = "sinhung",
		.ops = &sh_ops,
		.setup = &sh_setup,
#elif defined(DEVICE_MODEL_UCAR)
		.name = "ucar",
		.ops = &ucar_ops,
		.setup = &ucar_setup,
#elif defined(DEVICE_MODEL_LOOP)
		.name = "loop",
		.ops = &loop_ops,
		.setup = &loop_setup,
#elif defined(DEVICE_MODEL_LOOP2)
		.name = "loop2",
		.ops = &loop2_ops,
		.setup = &loop2_setup,
#elif defined(DEVICE_MODEL_CHOYOUNG)
		.name = "choyoung",
		.ops = &cy_ops,
		.setup = &cy_setup,
#elif defined(DEVICE_MODEL_IREAL)
		.name = "ireal",
		.ops = &ireal_ops,
		.setup = &ireal_setup,
#elif defined(DEVICE_MODEL_KDT)
		.name = "kdt",
		.ops = &cy_ops,
		.setup = &cy_setup,
#elif defined(DEVICE_MODEL_INNOCAR)
		.name = "innocar",
		.ops = &inno_ops,
		.setup = &inno_setup,
#elif defined(DEVICE_MODEL_DAESIN)
		.name = "daesin",
		.ops = &daesin_ops,
		.setup = &daesin_setup,
#elif defined(DEVICE_MODEL_CJ)
		.name = "cj",
		.ops = &cj_ops,
		.setup = &cj_setup,
#elif defined(DEVICE_MODEL_INNOSNS)
		.name = "innosns",
		.ops = &inno_ops,
		.setup = &inno_setup,
#elif defined(DEVICE_MODEL_INNOSNS_DCU)
		.name = "innosns_dcu",
		.ops = &inno_ops,
		.setup = &inno_setup,
#else
	#error "DEVICE MODEL NOT DEFINE ERROR"
#endif
};

// for gtrack feature
TACOM* tacom_get_cur_context()
{
	if (p_tacom_contex != NULL)
		return p_tacom_contex;


	p_tacom_contex = (TACOM *) malloc(sizeof(TACOM));

	p_tacom_contex->tm_ops = tm_list.ops;
	p_tacom_contex->tm_setup = tm_list.setup;

	return p_tacom_contex;
}


char *tacom_get_stream()
{
	TACOM *tm = tacom_get_cur_context();
	DTG_LOGD("%s: %s>>", __FILE__, __func__);
	return tm->tm_strm.stream;
}

int tacom_get_errno()
{
	TACOM *tm = tacom_get_cur_context();
	DTG_LOGD("%s: %s>>errno %d\n", __FILE__, __func__, tm->tm_err.tm_errno);
	return tm->tm_err.tm_errno;
}

int tacom_get_status () {
	TACOM *tm = tacom_get_cur_context();
	return tm->status;
}

void tacom_set_status (enum tacom_stat stat)
{
	TACOM *tm = tacom_get_cur_context();
	pthread_mutex_lock(&status_mutex);
	tm->status = stat;
	pthread_mutex_unlock(&status_mutex);
}



dtg_status_t dtg_status;

TACOM *tacom_init ()
{
	static TACOM *tm = NULL;
	int size;

	DTG_LOGD("%s - %d\r\n", __func__, __LINE__);
	// gtrack feature..
	tm = tacom_get_cur_context();
	if ( tm == NULL )
	{
		return NULL;
	}

	tm->tm_setup->conf_flag |= 0x1 << STANDARD_FLOW_BIT;

	size = (sizeof(tacom_std_hdr_t) + 
			sizeof(tacom_std_data_t) * tm->tm_setup->max_records_size);

	dtg_uart_fd = mds_api_init_uart(DTG_TTY_DEV_NAME, B115200);	// TODO: api fix
	if (dtg_uart_fd < 0) {
		DTG_LOGE("uart open fail");
		goto failed;
	}
	memset(&dtg_status, 0, sizeof(dtg_status_t));

	tm->tm_strm.stream = (char*) malloc(size);
	memset(tm->tm_strm.stream, 0, size);
	tm->tm_strm.size = size;

	tacom_set_status( TACOM_IDLE);
	DTG_LOGD("CHANGE STATUS - TACOM_IDLE");

	pthread_mutex_init(&cmd_mutex, NULL);
	fprintf(stderr, "cmd_mutex===========================> Init\n");
	if (tm->tm_ops->tm_init_process != NULL)
		tm->tm_ops->tm_init_process(tm);

	DTG_LOGT("======================================================================>\n");
	DTG_LOGT("======================================================================>\n");
	DTG_LOGT( "%sINIT SUCCESS %x", tm->tm_setup->tacom_dev, tm->tm_setup->conf_flag);
	DTG_LOGT("======================================================================>\n");
	DTG_LOGT("======================================================================>\n");
	return tm;

failed:
	DTG_LOGT("======================================================================>\n");
	DTG_LOGT("======================================================================>\n");
	DTG_LOGE( "%sINIT FAILED", tm->tm_setup->tacom_dev);
	DTG_LOGT("======================================================================>\n");
	DTG_LOGT("======================================================================>\n");
	free(tm);
	tm = NULL;
	return NULL;
}

void store_dtg_status()
{
	int fd = 0;
	char f_stat[128] = {0};
	sprintf(f_stat, "0x%04x/%010d/%12s/", dtg_status.status, dtg_status.records, dtg_status.vrn);
	do {
		fd = open("/var/dtg_status", O_WRONLY | O_CREAT, 0644);
		sleep(1);
	} while (fd < 0);
	write(fd, f_stat, strlen(f_stat));
	close(fd);
}

void load_dtg_status()
{
	int fd = 0;
	char f_stat[128] = {0};
	char *ptr = NULL;
	do {
		fd = open("/var/dtg_status", O_RDONLY, 0644);
		sleep(1);
	} while (fd < 0 && errno != ENOENT);
	if(fd > 0) {
		read(fd, f_stat, 128);
		ptr = strtok(f_stat, "/");
		dtg_status.status = strtol(ptr, NULL, 16);
		ptr = strtok(NULL, "/");
		dtg_status.records = strtol(ptr, NULL, 10);
		ptr = strtok(NULL, "/");
		if (ptr != NULL)
			memcpy(dtg_status.vrn, ptr, 12);
		close(fd);
	}
}

int tacom_control (int type, char *data)
{
	int ret;
	//TACOM *tm = tacom_get_cur_context();
	/* check power */

	/* check reset */

	ret = 1;
	
	return ret;
}

int tacom_read_summary()
{
	int ret;
	TACOM *tm = tacom_get_cur_context();

	if ( tm->tm_ops->tm_read_summary == NULL )
		return -1;

	ret = tm->tm_ops->tm_read_summary();
	DTG_LOGD("%s: %s>> ret : %d", __FILE__, __func__, ret);
	
	return ret;
}

int tacom_ack_summary()
{
	int ret;
	TACOM *tm = tacom_get_cur_context();
	
	if ( tm->tm_ops->tm_ack_summary == NULL )
		return -1;

	ret = tm->tm_ops->tm_ack_summary();
	DTG_LOGD("%s: %s>> ret : %d", __FILE__, __func__, ret);
	
	return ret;
}

int tacom_read_current()
{
	int ret;
	TACOM *tm = tacom_get_cur_context();
	
	if ( tm->tm_ops->tm_read_current == NULL )
		return -1;

	ret = tm->tm_ops->tm_read_current();
	DTG_LOGD("%s: %s>> ret : %d", __FILE__, __func__, ret);
	
	return ret;
}

int tacom_unreaded_records_num()
{
    int ret;
	TACOM *tm = tacom_get_cur_context();

	if ( tm->tm_ops->tm_unreaded_records_num == NULL )
		return -1;

    ret = tm->tm_ops->tm_unreaded_records_num ();
    DTG_LOGD("%s: %s>> ret : 0x%08x(%d)", __FILE__, __func__, ret, ret);
    return ret;
}

int tacom_read_records (int r_num)
{
    int ret;
	TACOM *tm = tacom_get_cur_context();

	if ( tm->tm_ops->tm_read_records == NULL )
		return -1;

    ret = tm->tm_ops->tm_read_records(r_num);
    DTG_LOGD("%s: %s>> arg : 0x%08x(%d), ret : 0x%08x(%d)", __FILE__, __func__, r_num, r_num, ret, ret);
    return ret;
}

int tacom_ack_records (int r_num)
{
    int ret;
	TACOM *tm = tacom_get_cur_context();

	if ( tm->tm_ops->tm_ack_records == NULL )
		return -1;

    ret = tm->tm_ops->tm_ack_records(r_num);
    DTG_LOGD("%s: %s>> arg : %d ret : %d", __FILE__, __func__, r_num, ret);
    return ret;
}

int tacom_read_last_records (void)
{
	/* check power */

	/* check reset */

	return 1;
}

char * tacom_get_info ( int type)
{
	char *resp;
//	TACOM *tm = tacom_get_cur_context();
	/* check power */

	/* check reset */
	// kksworks : not used func?
	resp = 1;
	
	return resp;
}

int tacom_get_info_hdr(void *infohdr)
{
	int ret;
//	TACOM *tm = tacom_get_cur_context();

	ret = 1;

	return ret;
}

int tacom_data_build_period = 0;
int tacom_data_type = 1;

int tacom_set_info (int type, char *infodata)
{
	char buf[128] = {0};
	TACOM *tm = tacom_get_cur_context();

	switch (type)
	{
		case TACOM_VEHICLE_MODEL:
			sprintf(buf, "RD01%s", infodata);
			break;
		case TACOM_VEHICLE_ID_NUM:
			sprintf(buf, "RD02%s", infodata);
			break;
		case TACOM_VEHICLE_TYPE:
			sprintf(buf, "RD03%s", infodata);
			break;
		case TACOM_REGISTRAION_NUM:
			sprintf(buf, "RD04%s", infodata);
			break;
		case TACOM_BUSINESS_LICENSE_NUM:
			sprintf(buf, "RD05%s", infodata);
			break;
		case TACOM_DRIVER_CODE:
			sprintf(buf, "RD06%s", infodata);
			break;
		case TACOM_SPEED_COMPENSATION:
			sprintf(buf, "RD07%s", infodata);
			break;
		case TACOM_RPM_COMPENSATION:
			sprintf(buf, "RD08%s", infodata);
			break;
		case TACOM_ACCUMULATED_DISTANCE:
			sprintf(buf, "RD09%s", infodata);
			break;
		case TACOM_OIL_SUPPLY_COMPENSATION:
			sprintf(buf, "RD10%s", infodata);
			break;
		case TACOM_OIL_TIME_COMPENSATION:
			sprintf(buf, "RD11%s", infodata);
			break;
		/* special case*/
		case TACOM_DATA_BUILD_PERIOD:
			tacom_data_build_period = strtol(infodata, NULL, 10);
			DTG_LOGD("Set period to build data [%d]", tacom_data_build_period);
			return tacom_data_build_period;
		case TACOM_DATA_TYPE:
			tacom_data_type = strtol(infodata, NULL, 10);
			DTG_LOGD("Set data type [%d]", tacom_data_type);
			return tacom_data_type;
		default:
			return -1;
	}

	if ( tm->tm_ops->tm_send_cmd == NULL )
		return -1;

	if (tm->tm_ops->tm_send_cmd(buf, tm->tm_setup, "<<tacom_set_info>>") < 0)
		return -1;

	DTG_LOGD("%s: %s>> complete", __FILE__, __func__);
	return 1;
}

