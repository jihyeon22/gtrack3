
#ifndef AT_LOG_H
#define AT_LOG_H 1


//#define LOGD(svc,msg...)	logd(svc, eDebug, msg)	// debug 	: gray
// workaround!! 
/*
#define ATLOGD(svc,msg...)	printf(msg)
#define ATLOGW(svc,msg...)	logd(svc,eWarning, msg)	// warning  : brown
#define ATLOGI(svc,msg...)	logd(svc,eInfo, msg)	// info  	: green
#define ATLOGE(svc,msg...)	logd(svc,eError, msg)	// error 	: red
#define ATLOGT(svc,msg...)	logd(svc,eTrace, msg)	// trace 	: brightmagenta
*/
#define DTG_LOGLEVEL_DEBUG      0
#define DTG_LOGLEVEL_WARNNING   1
#define DTG_LOGLEVEL_INFO       2
#define DTG_LOGLEVEL_ERROR      3
#define DTG_LOGLEVEL_TRACE      4  


void dtglogd(int debug_level, const char *format, ...);

#define DTG_LOGD(msg...)	dtglogd(DTG_LOGLEVEL_DEBUG, msg)
#define DTG_LOGW(msg...)	dtglogd(DTG_LOGLEVEL_WARNNING, msg)	// warning  : brown
#define DTG_LOGI(msg...)	dtglogd(DTG_LOGLEVEL_INFO, msg)	    // info  	: green
#define DTG_LOGE(msg...)	dtglogd(DTG_LOGLEVEL_ERROR, msg)	// error 	: red
#define DTG_LOGT(msg...)	dtglogd(DTG_LOGLEVEL_TRACE, msg)	// trace 	: brightmagenta


#endif
