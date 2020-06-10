#include <string.h>
#include <pthread.h>

#include <base/thermtool.h>
#include <board/thermometer.h>
#include <util/tools.h>

static THERMORMETER_DATA g_thermdata;
static int is_getting_therm = 0;
static pthread_mutex_t therm_mutex = PTHREAD_MUTEX_INITIALIZER;
static int therm_fault = 0;
static int therm_time_sense = THERM_DEFAULT_TIME_SENSE;

int therm_get_curr_data(THERMORMETER_DATA *out)
{
	if(is_getting_therm == 0)
	{
		return -1;
	}

	pthread_mutex_lock(&therm_mutex);
	memcpy(out, &g_thermdata, sizeof(THERMORMETER_DATA));
	pthread_mutex_unlock(&therm_mutex);

	return 0;
}

void therm_set_curr_data(THERMORMETER_DATA* in)
{
	pthread_mutex_lock(&therm_mutex);
	memcpy(&g_thermdata, in, sizeof(THERMORMETER_DATA));
	pthread_mutex_unlock(&therm_mutex);
}

int _therm_is_timing_sense(void)
{
	static time_t prev_time_sense = 0;

	time_t cur_time =  tools_get_kerneltime();

	if(cur_time <= 0)
	{
		return -1;
	}

	if(prev_time_sense == 0)
	{
		prev_time_sense = cur_time;
		return 1;
	}

	if(cur_time - prev_time_sense >= therm_time_sense)
	{
		prev_time_sense = cur_time;
		return 1;
	}

	return 0;
}

int therm_sense(void)
{
	THERMORMETER_DATA temp_therm;

#ifdef ALWAYS_READ_THERM
#else
	if(therm_fault >= THERM_MAX_FAULT)
		return 0;
#endif

	if(_therm_is_timing_sense() != 1)
		return 0;
	
	memset(&temp_therm, 0, sizeof(THERMORMETER_DATA));
	if(get_tempature(&temp_therm) < 0)
	{
		is_getting_therm = 0;

#ifdef ALWAYS_READ_THERM
#else
		therm_fault++;
#endif

		return -1;
	}
	therm_fault = 0;
	
	therm_set_curr_data(&temp_therm);
	is_getting_therm = 1;
	
	return 0;
}

void therm_clear_fault(void)
{
	therm_fault = 0;
}

void therm_set_sense_cycle(int val)
{
	therm_time_sense = val;
}

int therm_set_dev(char *dev, int len_dev)
{
	return set_therm_device(dev, len_dev);
}


