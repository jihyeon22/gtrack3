#ifndef __MOVON_ADAS_PROTOCOL_H__
#define __MOVON_ADAS_PROTOCOL_H__

#include <adas_common.h>

int movon_adas__uart_chk();


#define MOVON_DATA_FRAME__PREFIX    0x5b
#define MOVON_DATA_FRAME__DATA1     0x49
#define MOVON_DATA_FRAME__DATA2     0x41
#define MOVON_DATA_FRAME__DATA3     ' '
#define MOVON_DATA_FRAME__SUFFIX    '_'

int movon_get_evt_data(ADAS_EVT_DATA_T* evt_data, MOVON_DATA_FRAME_T* movon_data);


#endif



