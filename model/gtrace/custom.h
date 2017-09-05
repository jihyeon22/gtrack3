#ifndef __MODEL_CUSTOM_H__
#define __MODEL_CUSTOM_H__

#include <board/board_system.h>

typedef enum msg_ev_code_custom msg_ev_code_custom_t;
enum msg_ev_code_custom{

#if defined(SERVER_ABBR_NEO)
	/* GEO FENCE NUM #2 entry Event Code */
	eNEO_MDM_BTN1_EVT       = 40,
	/* GEO FENCE NUM #2 exit Event Code */
	eNEO_MDM_BTN2_EVT       = 40,
	eMAX_EVENT_CODE           = 43, //LAST_EVENT_CODE + 1
#else
	eMAX_EVENT_CODE           = 39, //LAST_EVENT_CODE + 1
#endif
	
};

//jwrho persistant data path modify++
//#define MILEAGE_FILE	"/data/mds/data/mdt800_hnrt_mileage.dat"
#define MILEAGE_FILE	CONCAT_STR(USER_DATA_DIR, "/mdt800_gtrs_mileage.dat")
//jwrho persistant data path modify--

#endif
