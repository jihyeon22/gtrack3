
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
void dtglogd(const char *format, ...);

#define DTG_LOGD(msg...)	dtglogd(msg)
#define DTG_LOGW(msg...)	dtglogd(msg)	// warning  : brown
#define DTG_LOGI(msg...)	dtglogd(msg)	// info  	: green
#define DTG_LOGE(msg...)	dtglogd(msg)	// error 	: red
#define DTG_LOGT(msg...)	dtglogd(msg)	// trace 	: brightmagenta


#endif
