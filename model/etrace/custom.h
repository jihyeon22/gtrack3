#ifndef __MODEL_CUSTOM_H__
#define __MODEL_CUSTOM_H__

typedef enum msg_ev_code_custom msg_ev_code_custom_t;
enum msg_ev_code_custom{
	eREPORT_BATT	= 77,
	eMAX_EVENT_CODE                 = 78, //LAST_EVENT_CODE + 1
};

#define MILEAGE_FILE	"/data/etr_mileage.dat"

#endif
