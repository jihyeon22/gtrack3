#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "dtg_type.h"
#include "dtg_debug.h"
#include "dtg_data_manage.h"

//int g_dtg_debug_level = DTG_DEBUG_ALWAYS;
int g_dtg_debug_level = DTG_DEBUG_ALL;

void set_dtg_debug_level(int level)
{
	g_dtg_debug_level = level;
}

int get_dtg_debug_level()
{
	return g_dtg_debug_level;
}

void debug_msg(char *msg)
{
	int fd = open( CONFIG_DEBUG_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if(fd <= 0) {
		printf("debug file open error : %s\n", CONFIG_DEBUG_PATH);
		return;
	}
	write(fd, msg, strlen(msg));
	close(fd);
	return;
}

void dtg_printf(int level, char *fmt, ...)
{
	va_list args;
	char msg[512];

	if(level != DTG_DEBUG_ALWAYS) {
		if(level > g_dtg_debug_level)
			return;
	}

	va_start(args, fmt);
	vprintf(fmt, args);
	vsprintf(msg, fmt, args);
	va_end(args);



	if(DEBUG_LOG_FILE)
	{
		debug_msg(msg); //파일 저장
	}
}
