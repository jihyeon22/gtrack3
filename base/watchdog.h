#ifndef __BASE_WATCHDOG_H__
#define __BASE_WATCHDOG_H__

#define WATCHDOG_MIN_SEC			300
#define WATCHDOG_TCP_OVERHEAD_SEC	300
#define WATCHDOG_DEFAULT_SEC 		600 //use pipe timeout

typedef enum eWdType eWdType_t;
enum eWdType
{
	eWdNet1 = 0,

#ifdef USE_NET_THREAD2
	eWdNet2,
#endif

#ifdef USE_BUTTON_THREAD
	eWdPwr,
#endif

	eWdMain,

#ifdef USE_GPS_MODEL
	eWdGps,
#endif

	eWdLast,
};

int watchdog_set_cur_ktime(eWdType_t wd_type);
int watchdog_set_init_ktime(eWdType_t wd_type);
int watchdog_set_time(eWdType_t wd_type, unsigned int secs);
void watchdog_process(void);

extern int wd_dbg[eWdLast];
void get_watchdog_status();
#endif

