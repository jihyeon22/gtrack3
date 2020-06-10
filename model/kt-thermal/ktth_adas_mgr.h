#ifndef __H_KTTH_ADAS_MGR__
#define __H_KTTH_ADAS_MGR__

int ktth_adas_mgr__init();

typedef struct ktthAdasData ktthAdasData_t;
struct ktthAdasData
{
	int event_code;
	char adas_data_str[32];
};


#endif
