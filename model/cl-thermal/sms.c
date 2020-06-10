#include "command.h"
#include "sms.h"

// ----------------------------------------
//  LOGD(LOG_TARGET, LOG_TARGET,  Target
// ----------------------------------------
#define LOG_TARGET eSVC_MODEL

int parse_model_sms(char *time, char *phonenum, char *sms)
{
	process_cmd(sms);
	
	return 0;
}

