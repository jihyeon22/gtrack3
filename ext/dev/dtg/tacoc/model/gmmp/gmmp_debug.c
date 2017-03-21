#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define CONFIG_DEBUG_PATH "./gmmp_test_debug.log"
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

	//if(level != DTG_DEBUG_ALWAYS) {
	//	if(level > g_dtg_debug_level)
	//		return;
	//}

	va_start(args, fmt);
	vprintf(fmt, args);
	vsprintf(msg, fmt, args);
	va_end(args);

	//if(level == DTG_DEBUG_ERROR) {
	//	add_dtg_msg_buffer(msg); //원격 진단용
	//}

	//if(get_debug_file_log_enable()) {
	//	debug_msg(msg); //파일 저장
	//}

	
}