#ifndef __MOVON_ADAS_PROTOCOL_H__
#define __MOVON_ADAS_PROTOCOL_H__

#include <adas_common.h>

int movon_adas__uart_chk();

int movon_get_evt_data(ADAS_EVT_DATA_T* evt_data, MOVON_DATA_FRAME_T* movon_data);


#endif



