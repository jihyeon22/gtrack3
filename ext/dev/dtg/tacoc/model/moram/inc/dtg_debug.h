#ifndef __DTG_DEBUG_DEFINE_HEADER__
#define __DTG_DEBUG_DEFINE_HEADER__

#include "wrapper/dtg_log.h"

#define DEBUG_ALWAYS(fmt, arg...)				DTG_LOGI(fmt, ##arg)
#define DEBUG_FUNC_TRACE(fmt, arg...)			DTG_LOGD(fmt, ##arg)
#define DEBUG_INFO(fmt, arg...)					DTG_LOGD(fmt, ##arg)
#define DEBUG_ERROR(fmt, arg...)				DTG_LOGE(fmt, ##arg)
#define DEBUG_UART_PACKET_DUMP(fmt, arg...)		DTG_LOGD(fmt, ##arg)
#define PACKET_MSG_TRACE(fmt, arg...)			DTG_LOGD(fmt, ##arg)

#endif
