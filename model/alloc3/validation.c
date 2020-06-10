#include "validation.h"
#include "debug.h"

int validation_model_report_interval(unsigned int period_interval, unsigned int send_interval)
{
	if (	period_interval >= 5 &&
		period_interval <= 6500 &&
		send_interval >= 5 &&
		send_interval <= 6500)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int validation_model_report_off_interval(unsigned int period_interval, unsigned int send_interval)
{
	if (	period_interval >= 30 &&
		period_interval <= 6500 &&
		send_interval >= 30 &&
		send_interval <= 6500)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

