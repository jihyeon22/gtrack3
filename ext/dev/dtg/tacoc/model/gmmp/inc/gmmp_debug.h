<<<<<<< HEAD
#ifndef __GMMP_DEBUG_MSG_HEADER__
#define __GMMP_DEBUG_MSG_HEADER__

enum {
	DTG_DEBUG_ALWAYS,
	DTG_DEBUG_ERROR,
	DTG_DEBUG_INFO,
	DTG_DEBUG_NET_PACKET,
	DTG_DEBUG_FUNC_TRACE,
	DTG_DEBUG_UART_PACKET_DUMP,
	DTG_DEBUG_REAL_TIME_DATA_PRINT, //조영프로토콜 실시간 data 출력
	DTG_DEBUG_ALL,
}DTG_DEBUG_LEVEL;

#define DEBUG_ALWAYS(fmt, arg...)				dtg_printf(DTG_DEBUG_ALWAYS, fmt, ##arg)
#define DEBUG_FUNC_TRACE(fmt, arg...)			dtg_printf(DTG_DEBUG_FUNC_TRACE, fmt, ##arg)
#define DEBUG_INFO(fmt, arg...)					dtg_printf(DTG_DEBUG_INFO, fmt, ##arg)
#define DEBUG_ERROR(fmt, arg...)				dtg_printf(DTG_DEBUG_ERROR, fmt, ##arg)
#define DEBUG_UART_PACKET_DUMP(fmt, arg...)		dtg_printf(DTG_DEBUG_UART_PACKET_DUMP, fmt, ##arg)
#define PACKET_MSG_TRACE(fmt, arg...)			dtg_printf(DTG_DEBUG_NET_PACKET, fmt, ##arg)
#define DEBUG_JOYOUNG_UART_MSG_PRINT(fmt, arg...) dtg_printf(DTG_DEBUG_REAL_TIME_DATA_PRINT, fmt, ##arg)

=======
#ifndef __GMMP_DEBUG_MSG_HEADER__
#define __GMMP_DEBUG_MSG_HEADER__

enum {
	DTG_DEBUG_ALWAYS,
	DTG_DEBUG_ERROR,
	DTG_DEBUG_INFO,
	DTG_DEBUG_NET_PACKET,
	DTG_DEBUG_FUNC_TRACE,
	DTG_DEBUG_UART_PACKET_DUMP,
	DTG_DEBUG_REAL_TIME_DATA_PRINT, //조영프로토콜 실시간 data 출력
	DTG_DEBUG_ALL,
}DTG_DEBUG_LEVEL;

#define DEBUG_ALWAYS(fmt, arg...)				dtg_printf(DTG_DEBUG_ALWAYS, fmt, ##arg)
#define DEBUG_FUNC_TRACE(fmt, arg...)			dtg_printf(DTG_DEBUG_FUNC_TRACE, fmt, ##arg)
#define DEBUG_INFO(fmt, arg...)					dtg_printf(DTG_DEBUG_INFO, fmt, ##arg)
#define DEBUG_ERROR(fmt, arg...)				dtg_printf(DTG_DEBUG_ERROR, fmt, ##arg)
#define DEBUG_UART_PACKET_DUMP(fmt, arg...)		dtg_printf(DTG_DEBUG_UART_PACKET_DUMP, fmt, ##arg)
#define PACKET_MSG_TRACE(fmt, arg...)			dtg_printf(DTG_DEBUG_NET_PACKET, fmt, ##arg)
#define DEBUG_JOYOUNG_UART_MSG_PRINT(fmt, arg...) dtg_printf(DTG_DEBUG_REAL_TIME_DATA_PRINT, fmt, ##arg)

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
#endif