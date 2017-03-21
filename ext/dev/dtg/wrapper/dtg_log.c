


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
//#include <wrapper/dtg_log.h>
#include <wrapper/dtg_log.h>


//extern FILE *g_debug_file = NULL;
void dtglogd(const char *format, ...)
{
	char tmp[1024] = {0};
	
	va_list va;
	va_start(va, format);
	vsprintf(tmp, format, va);
	va_end(va);
/*	
	if(g_debug_file != NULL) {
		fprintf(g_debug_file, tmp);
		fflush(g_debug_file);
	}
*/
    printf("[dtg debug] %s\r\n",tmp);
	//LOGD(eSVC_AT, tmp);
}