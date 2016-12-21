#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>


#include <base/devel.h>
#include <base/thread.h>
#include <board/led.h>
#include <util/poweroff.h>
#include <util/tools.h>
#include <util/storage.h>
#include <include/defines.h>
#include <logd_rpc.h>
#include "error.h"

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_COMMON

static void _error_check_working_properly(void);

int gError_state = eERROR_NONE;

//add jwrho ++
int insert_log_history(char *file, void *pmsg, int log_unit_size, int max_count)
{
	
	int fd;
	int nread;
	int nhist_cnt;
	unsigned char *pdata;

	if(pmsg == NULL) {
		printf("%s:%d> log msg is NULL\n", __func__, __LINE__);
		return -1;
	}

	pdata = (unsigned char *)malloc(log_unit_size*max_count);
	if(pdata == NULL) {
		printf("%s:%d> malloc error\n", __func__, __LINE__);
		return -2;
	}
	memset(pdata, 0x00, log_unit_size*max_count);

	fd = open(file, O_RDWR | O_CREAT , 0644);
	if(fd < 0) {
		free(pdata);
		printf("%s:%d> file open fail : err[%d]\n", __func__, __LINE__, errno);
		return -1;
	}

	nread = read(fd, pdata, log_unit_size*max_count);
	if(nread < 0) {
		printf("%s:%d> file read fail : err[%d]\n", __func__, __LINE__, errno);
		free(pdata);
		close(fd);
		return -1;
	}
	nhist_cnt = nread / log_unit_size;

	lseek(fd, 0, SEEK_SET);
	
	write(fd, pmsg, log_unit_size);

	if( (nhist_cnt + 1) > max_count )
		nhist_cnt -= 1;

	write(fd, pdata, log_unit_size*nhist_cnt);

		
	free(pdata);
	close(fd);

	return 0;

}

int insert_log_msg(char *file, char *msg, int max_count)
{
	struct timeval tv;
	struct tm ttm;
	MSG_LOG data;
	int ret;

	memset(&data, 0x00, sizeof(data));

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ttm);
	
	snprintf(data.date, sizeof(data.date)-1, "%04d-%02d-%02d %02d:%02d:%02d\n", 
			ttm.tm_year + 1900, ttm.tm_mon + 1, ttm.tm_mday, 
			ttm.tm_hour, ttm.tm_min, ttm.tm_sec);

	if(msg != NULL)
		snprintf(data.contents, sizeof(data.contents)-1, "%s\n\n", msg);

	ret = insert_log_history(file, &data, sizeof(MSG_LOG), max_count);

	return ret;
	
}
//add jwrho --

void error_make_no_mon(void)
{
	FILE *fptr = NULL;

	fptr = fopen(FLG_ERR_PATH, "wb");

	if(fptr != NULL)
	{
		fclose(fptr);
	}
}

void error_rm_no_mon(void)
{
	remove(FLG_ERR_PATH);
}

void error_reset_datafile(void)
{
	errorDataFile_t error_data_file;
	
	error_data_file.chr_no_exit = '0';
	error_data_file.chr_no_reboot = '0';

	storage_save_file(DAT_ERR_PATH, &error_data_file, sizeof(error_data_file));
}

void error_read_datafile(errorData_t *err_data)
{
	int ret = 0;
	errorDataFile_t error_data_file = {'0', '0'};
	
	ret = storage_load_file(DAT_ERR_PATH, &error_data_file, sizeof(error_data_file));
	
	if( ret < 0 )
	{
		err_data->no_exit = 0;
		err_data->no_reboot = 0;
	}
	else
	{
		err_data->no_exit = error_data_file.chr_no_exit - '0';
		err_data->no_reboot = error_data_file.chr_no_reboot - '0';
	}
	
	LOGE(LOG_TARGET, "error_read_datafile exit:%d reboot:%d\n", err_data->no_exit, err_data->no_reboot);
}

void error_write_datafile(errorData_t *err_data)
{
	errorDataFile_t error_data_file;
	
	error_data_file.chr_no_exit = err_data->no_exit + '0';
	error_data_file.chr_no_reboot = err_data->no_reboot +'0';
	
	storage_save_file(DAT_ERR_PATH, &error_data_file, sizeof(error_data_file));
	
	LOGE(LOG_TARGET, "error_write_datafile exit:%c reboot:%c\n", error_data_file.chr_no_exit, error_data_file.chr_no_reboot);
}

void error_critical(const int level, const char *format, ...)
{
	va_list va;
	char log_buff[530] = {0,};
	char text_buff[1024] = {0,};
	errorData_t errdata = {0};

	va_start(va, format);
	vsnprintf(text_buff, sizeof(text_buff) - 1, format, va);
	va_end(va);

#ifdef DEBUG
	memset(log_buff, 0, sizeof(log_buff));
	snprintf(log_buff, sizeof(log_buff)-1, "%.512s level %d\n", text_buff, level);
	insert_log_msg(LOG_ERR_PATH, log_buff, LOG_ERR_MAX_COUNT);

	devel_send_sms_noti(text_buff, sizeof(text_buff), 3);
	
	if(level == eERROR_LOG)
	{
		return;
	}

	while(1)
	{
		led_noti(eLedcmd_ERR_NOTI);
		LOGE(LOG_TARGET, "error_critical %.512s\n", text_buff);
		sleep(1);
	}
#else
	if(level == eERROR_LOG)
	{
		memset(log_buff, 0, sizeof(log_buff));
		snprintf(log_buff, sizeof(log_buff)-1, "%.512s level %d\n", text_buff, level);
		insert_log_msg(LOG_ERR_PATH, log_buff, LOG_ERR_MAX_COUNT);
	}
	else
	{
		_error_check_working_properly();
		error_read_datafile(&errdata);
		memset(log_buff, 0, sizeof(log_buff));
		snprintf(log_buff, sizeof(log_buff)-1, "%.512s level %d exit %d reboot %d\n", text_buff, level, errdata.no_exit, errdata.no_reboot);
		insert_log_msg(LOG_ERR_PATH, log_buff, LOG_ERR_MAX_COUNT);
		devel_webdm_send_log(text_buff);
	}

	int do_level = eERROR_NONE;
	do_level = level;
	
	switch(do_level)
	{
		case eERROR_LOG:
			break;
		case eERROR_EXIT:
			if(errdata.no_exit >= MAX_RETRY_EXIT)
			{
				errdata.no_exit = 0;
				do_level = eERROR_REBOOT;
			}
			else
			{
				errdata.no_exit++;
				error_write_datafile(&errdata);
				break;
			}			
		case eERROR_REBOOT:
			if(errdata.no_reboot >= MAX_RETRY_REBOOT)
			{
				do_level = eERROR_FINAL;
			}
			else
			{
				errdata.no_reboot++;
				error_write_datafile(&errdata);
			}
			break;
		default:
			;
	}

	gError_state = do_level;
	
	switch(do_level)
	{
		case eERROR_LOG:
			break;
		case eERROR_EXIT:
			error_make_no_mon();
			exit_thread_all();
			break;
		case eERROR_REBOOT:
			error_make_no_mon();
			exit_thread_all();
			break;
		case eERROR_FINAL:
			led_noti(eLedcmd_ERR_NOTI);

			memset(log_buff, 0, sizeof(log_buff));
			snprintf(log_buff, sizeof(log_buff)-1, "%.512s\n", text_buff);
			//tools_write_data(LOG_CRITICAL_PATH, log_buff, strlen(log_buff), 1);
			insert_log_msg(LOG_CRITICAL_PATH, log_buff, LOG_CRITICAL_MAX_COUNT);
			devel_webdm_send_log(log_buff);
			error_reset_datafile();
			
			error_make_no_mon();
			exit_thread_all();
			break;
		default:
			;
	}
#endif
}

static void _error_check_working_properly(void)
{
	static int was_checked = 0;

	if(was_checked)
	{
		return;
	}

	was_checked = 1;
	
	if(tools_get_kerneltime() >= WORKING_PROPERLY_SECS)
	{
		error_reset_datafile();
	}
}

