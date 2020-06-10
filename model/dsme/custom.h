#ifndef __MODEL_CUSTOM_H__
#define __MODEL_CUSTOM_H__

#include <board/board_system.h>

typedef enum msg_ev_code_custom msg_ev_code_custom_t;
enum msg_ev_code_custom{
	eREPORT_USER_DATA	= 77,
	eMAX_EVENT_CODE     = 78, //LAST_EVENT_CODE + 1
};

#endif
