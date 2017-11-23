#ifndef __BASE_THREAD_H___
#define __BASE_THREAD_H___

#include <pthread.h>

typedef struct threadData threadData_t;
struct threadData
{
	void *thread;
	int stack_size;
};

#define MAX_THREAD_COUNT		10

#ifdef USE_GPS_MODEL
void *thread_gps(void *args);
#endif
void *thread_btn_pwr(void *args);
void *thread_network(void *args);
#ifdef USE_NET_THREAD2
void *thread_network2(void *args);
#endif

void exit_thread_network(void);
#ifdef USE_NET_THREAD2
void exit_thread_network2(void);
#endif
void exit_thread_btn_pwr(void);
void exit_thread_gps(void);
void exit_thread_all(void);
void thread_network_set_warn_timeout(int time);

#define DEFAULT_NETWORK_FAIL_RESET_CNT_1			60*10	// 10MIN... : user setable value
#define DEFAULT_NETWORK_FAIL_RESET_CNT_2			60	// boot network chk : 1MIN... : default value : do not setting value
#define DEFAULT_NETWORK_FAIL_RESET_CNT_3			4	// runtime network chk : 4 * 30sec = 2MIN... : default value : do not setting value

void set_max_network_fail_reset_cnt(int cnt);

extern pthread_t tid[MAX_THREAD_COUNT];
extern threadData_t threads[MAX_THREAD_COUNT];

// ----------------------------------------
// model thread
extern pthread_t tid_model;
extern void (*thread_model)(void);
extern void (*exit_thread_model)(void);
// ----------------------------------------


#endif

