<<<<<<< HEAD



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
//#include <wrapper/dtg_log.h>
#include <wrapper/dtg_log.h>

#include <logd/logd_rpc.h>

#define LOG_TARGET eSVC_COMMON

//extern FILE *g_debug_file = NULL;
void dtglogd(int debug_level, const char *format, ...)
{
	char tmp[1024] = {0};
	char tmp2[1024] = {0};
	
	va_list va;
	va_start(va, format);
	vsprintf(tmp, format, va);
	va_end(va);

	sprintf(tmp2, "[DTG MSG] %s \n", tmp);
/*	
	if(g_debug_file != NULL) {
		fprintf(g_debug_file, tmp);
		fflush(g_debug_file);
	}
*/
	switch (debug_level)
	{
		case DTG_LOGLEVEL_DEBUG:
			//LOGD(LOG_TARGET, tmp2);
			printf(tmp2);
		break;
		case DTG_LOGLEVEL_WARNNING:
			LOGW(LOG_TARGET, tmp2);
		break;
		case DTG_LOGLEVEL_INFO:
			LOGI(LOG_TARGET, tmp2);
		break;
		case DTG_LOGLEVEL_ERROR:
			LOGE(LOG_TARGET, tmp2);
		break;
		case DTG_LOGLEVEL_TRACE:
			LOGT(LOG_TARGET, tmp2);
		break;
		default:
			LOGD(LOG_TARGET, tmp2);
		break;
	}
    //printf("[dtg debug] %s\r\n",tmp);
	//LOGD(eSVC_AT, tmp);
=======



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
//#include <wrapper/dtg_log.h>
#include <wrapper/dtg_log.h>

#include <logd/logd_rpc.h>

#define LOG_TARGET eSVC_COMMON

//extern FILE *g_debug_file = NULL;
void dtglogd(int debug_level, const char *format, ...)
{
	char tmp[1024] = {0};
	char tmp2[1024] = {0};
	
	va_list va;
	va_start(va, format);
	vsprintf(tmp, format, va);
	va_end(va);

	sprintf(tmp2, "[DTG MSG] %s \n", tmp);
/*	
	if(g_debug_file != NULL) {
		fprintf(g_debug_file, tmp);
		fflush(g_debug_file);
	}
*/
	switch (debug_level)
	{
		case DTG_LOGLEVEL_DEBUG:
			//LOGD(LOG_TARGET, tmp2);
			printf(tmp2);
		break;
		case DTG_LOGLEVEL_WARNNING:
			LOGW(LOG_TARGET, tmp2);
		break;
		case DTG_LOGLEVEL_INFO:
			LOGI(LOG_TARGET, tmp2);
		break;
		case DTG_LOGLEVEL_ERROR:
			LOGE(LOG_TARGET, tmp2);
		break;
		case DTG_LOGLEVEL_TRACE:
			LOGT(LOG_TARGET, tmp2);
		break;
		default:
			LOGD(LOG_TARGET, tmp2);
		break;
	}
    //printf("[dtg debug] %s\r\n",tmp);
	//LOGD(eSVC_AT, tmp);
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
}