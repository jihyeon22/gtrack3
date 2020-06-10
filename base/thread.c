#include <pthread.h>

#include <at/at_util.h>
#include <callback.h>
#include "thread.h"

pthread_t tid[MAX_THREAD_COUNT] = {0, };
threadData_t threads[] = {
#ifdef USE_GPS_MODEL
	{thread_gps, 512 * 1024},
#endif
#ifdef USE_BUTTON_THREAD
	{thread_btn_pwr, 256 * 1024},
#endif
	{thread_network, 512 * 1024},
#ifdef USE_NET_THREAD2
	{thread_network2, 512 * 1024},
#endif
	{NULL,0}
};


// ----------------------------------------
// model thread
pthread_t tid_model = 0;

void (*thread_model)(void) = NULL;
void (*exit_thread_model)(void) = NULL;
// ----------------------------------------

void exit_thread_all(void)
{
	exit_thread_network();
#ifdef USE_NET_THREAD2
	exit_thread_network2();
#endif
#ifdef USE_BUTTON_THREAD
	exit_thread_btn_pwr();
#endif
#ifdef USE_GPS_MODEL
	exit_thread_gps();
#endif

	if(exit_thread_model != NULL)
		exit_thread_model();

	exit_main_loop();
}

