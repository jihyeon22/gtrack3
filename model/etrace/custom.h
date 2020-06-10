#ifndef __MODEL_CUSTOM_H__
#define __MODEL_CUSTOM_H__

#include <board/board_system.h>

typedef enum msg_ev_code_custom msg_ev_code_custom_t;
enum msg_ev_code_custom{
	eREPORT_BATT	= 77,
	eMAX_EVENT_CODE                 = 78, //LAST_EVENT_CODE + 1
};

//jwrho persistant data path modify++
//#define MILEAGE_FILE	"/data/mds/data/mdt800_etr_mileage.dat"
#define MILEAGE_FILE	CONCAT_STR(USER_DATA_DIR, "/mdt800_etr_mileage.dat")
//jwrho persistant data path modify--

#endif
