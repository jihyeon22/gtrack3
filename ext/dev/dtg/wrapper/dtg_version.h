#ifndef __PAKCAGE_VERSION_DEF_HEADER__
#define __PAKCAGE_VERSION_DEF_HEADER__

  //#define SW_VERSION  "123456789012345678901234567890"
 // "w200k.hnrt.sinhung-v1.00"
#define MDT_MODEL	"w200k"

#if defined(SERVER_MODEL_HNRT) || defined(SERVER_MODEL_HNRT_DEV) || defined(SERVER_MODEL_HNRT_TB)
	#define SVR_MODEL	"hnrt"
	
	#if defined(DEVICE_MODEL_UCAR)
		#define DTG_MODEL	"uca"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_SINHUNG)
		#define DTG_MODEL	"sih"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP)
		#define DTG_MODEL	"lp1"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP2)
		#define DTG_MODEL	"lp2"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CHOYOUNG)
		#define DTG_MODEL	"cyg"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_KDT)
		#define DTG_MODEL	"kdt"
		#define SW_VERSION	"v1.15"
	#elif defined(DEVICE_MODEL_IREAL)
		#define DTG_MODEL	"irl"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOCAR)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CJ)
		#define DTG_MODEL	"zen"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_DAESIN)
		#define DTG_MODEL	"dsn"
		#define SW_VERSION	"v1.00"
	#else
		#error "device model not support error"
	#endif
#elif defined(SERVER_MODEL_ETRS) || defined(SERVER_MODEL_ETRS_TB)
	#define SVR_MODEL	"etrs"
	
	#if defined(DEVICE_MODEL_UCAR)
		#define DTG_MODEL	"uca"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_SINHUNG)
		#define DTG_MODEL	"sih"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP)
		#define DTG_MODEL	"lp1"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP2)
		#define DTG_MODEL	"lp2"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CHOYOUNG)
		#define DTG_MODEL	"cyg"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_KDT)
		#define DTG_MODEL	"kdt"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_IREAL)
		#define DTG_MODEL	"irl"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOCAR)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CJ)
		#define DTG_MODEL	"zen"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_DAESIN)
		#define DTG_MODEL	"dsn"
		#define SW_VERSION	"v1.05"
	#else
		#error "device model not support error"
	#endif
#elif defined(SERVER_MODEL_GTRS) || defined(SERVER_MODEL_GTRS_TB)
	#define SVR_MODEL	"gtrs"
	
	#if defined(DEVICE_MODEL_UCAR)
		#define DTG_MODEL	"uca"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_SINHUNG)
		#define DTG_MODEL	"sih"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP)
		#define DTG_MODEL	"lp1"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP2)
		#define DTG_MODEL	"lp2"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CHOYOUNG)
		#define DTG_MODEL	"cyg"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_KDT)
		#define DTG_MODEL	"kdt"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_IREAL)
		#define DTG_MODEL	"irl"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOCAR)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.01"
	#elif defined(DEVICE_MODEL_INNOSNS)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CJ)
		#define DTG_MODEL	"zen"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_DAESIN)
		#define DTG_MODEL	"dsn"
		#define SW_VERSION	"v1.00"
	#else
		#error "device model not support error"
	#endif
#elif defined(SERVER_MODEL_NEOGNP)
	#define SVR_MODEL	"ngnp"
	
	#if defined(DEVICE_MODEL_UCAR)
		#define DTG_MODEL	"uca"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_SINHUNG)
		#define DTG_MODEL	"sih"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP)
		#define DTG_MODEL	"lp1"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP2)
		#define DTG_MODEL	"lp2"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CHOYOUNG)
		#define DTG_MODEL	"cyg"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_KDT)
		#define DTG_MODEL	"kdt"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_IREAL)
		#define DTG_MODEL	"irl"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOCAR)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.01"
	#elif defined(DEVICE_MODEL_INNOSNS)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CJ)
		#define DTG_MODEL	"zen"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_DAESIN)
		#define DTG_MODEL	"dsn"
		#define SW_VERSION	"v1.00"
	#else
		#error "device model not support error"
	#endif
#elif defined(SERVER_MODEL_OPENSNS) || defined(SERVER_MODEL_OPENSNS_TB)
	#define SVR_MODEL	"opns"
	
	#if defined(DEVICE_MODEL_UCAR)
		#define DTG_MODEL	"uca"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_SINHUNG)
		#define DTG_MODEL	"sih"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP)
		#define DTG_MODEL	"lp1"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP2)
		#define DTG_MODEL	"lp2"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CHOYOUNG)
		#define DTG_MODEL	"cyg"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_KDT)
		#define DTG_MODEL	"kdt"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_IREAL)
		#define DTG_MODEL	"irl"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOCAR)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.01"
	#elif defined(DEVICE_MODEL_INNOSNS)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOSNS_DCU)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CJ)
		#define DTG_MODEL	"zen"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_DAESIN)
		#define DTG_MODEL	"dsn"
		#define SW_VERSION	"v1.00"
	#else
		#error "device model not support error"
	#endif
#elif defined(SERVER_MODEL_PRINET)
	#define SVR_MODEL	"prnt"
	
	#if defined(DEVICE_MODEL_UCAR)
		#define DTG_MODEL	"uca"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_SINHUNG)
		#define DTG_MODEL	"sih"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP)
		#define DTG_MODEL	"lp1"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_LOOP2)
		#define DTG_MODEL	"lp2"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CHOYOUNG)
		#define DTG_MODEL	"cyg"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_KDT)
		#define DTG_MODEL	"kdt"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_IREAL)
		#define DTG_MODEL	"irl"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_INNOCAR)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.01"
	#elif defined(DEVICE_MODEL_INNOSNS)
		#define DTG_MODEL	"ino"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_CJ)
		#define DTG_MODEL	"zen"
		#define SW_VERSION	"v1.00"
	#elif defined(DEVICE_MODEL_DAESIN)
		#define DTG_MODEL	"dsn"
		#define SW_VERSION	"v1.00"
	#else
		#error "device model not support error"
	#endif
#endif




#define APP_VER_DEV_NUM                      255
#define APP_VER_DEV_NAME                     "/dev/alive"
#define APP_VER_UTIL_MAJOR_NUMBER			 255

#define APP_STR_SIZE						(64)
#define IOCTL_SET_APP_VER                   _IOR(APP_VER_UTIL_MAJOR_NUMBER, 7, char *)

#define APP_VER_SUCCESS						(1)
#define APP_VER_FAIL						(-1)

int set_app_ver(char* app_ver);


#endif //__PAKCAGE_VERSION_DEF_HEADER__
