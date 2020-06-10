#ifndef __DTG_DEBUG_DEFINE_HEADER__
#define __DTG_DEBUG_DEFINE_HEADER__


#include "stdio.h"

enum {
	DTG_DEBUG_ALWAYS,
	DTG_DEBUG_ERROR,
	DTG_DEBUG_INFO,
	DTG_DEBUG_NET_PACKET,
	DTG_DEBUG_FUNC_TRACE,
	DTG_DEBUG_UART_PACKET_DUMP,
	DTG_DEBUG_ALL,
}DTG_DEBUG_LEVEL;


void dtg_printf(int level, char *fmt, ...);
void set_dtg_debug_level(int level);
int get_dtg_debug_level();


#define DEBUG_LOG_FILE	0
#define CONFIG_DEBUG_PATH		"/system/mds/system/bin/log/dtg_debug.log"

/*
#define DEBUG_ALWAYS(fmt, arg...)				dtg_printf(DTG_DEBUG_ALWAYS, fmt, ##arg)
#define DEBUG_FUNC_TRACE(fmt, arg...)			dtg_printf(DTG_DEBUG_FUNC_TRACE, fmt, ##arg)
#define DEBUG_INFO(fmt, arg...)					dtg_printf(DTG_DEBUG_INFO, fmt, ##arg)
#define DEBUG_ERROR(fmt, arg...)				dtg_printf(DTG_DEBUG_ERROR, fmt, ##arg)
#define DEBUG_UART_PACKET_DUMP(fmt, arg...)		dtg_printf(DTG_DEBUG_UART_PACKET_DUMP, fmt, ##arg)
#define PACKET_MSG_TRACE(fmt, arg...)			dtg_printf(DTG_DEBUG_NET_PACKET, fmt, ##arg)
*/

#define DEBUG_ALWAYS(fmt, arg...) \
   printf( "[%s]-%s > ", __FILE__ , __FUNCTION__); \
	printf( fmt, ##arg)

#define DEBUG_INFO(fmt, arg...) \
   printf( "[%s]-%s > ", __FILE__ , __FUNCTION__); \
   printf( fmt, ##arg)

#define DEBUG_ERROR(fmt, arg...) \
   printf( "[%s]-%s > ", __FILE__ , __FUNCTION__); \
	printf( fmt, ##arg)

#define DEBUG_UART_PACKET_DUMP(fmt, arg...)	\
   printf( "[%s]-%s > " , __FILE__ , __FUNCTION__); \
   printf( fmt, ##arg)

#define PACKET_MSG_TRACE(fmt, arg...) \
   printf( "[%s]-%s > ", __FILE__ , __FUNCTION__); \
   printf( fmt, ##arg)

#define DEBUG_FUNC_TRACE(fmt, arg...) \
   printf( "[%s]-%s > ", __FILE__ , __FUNCTION__); \
   printf( fmt, ##arg)


#endif
