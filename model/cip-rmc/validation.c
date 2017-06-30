#include "validation.h"

int validation_model_report_interval(unsigned int period_interval, unsigned int send_interval)
{
	if (send_interval / period_interval >= 1 &&
		send_interval % period_interval == 0 &&
		period_interval <= 65000 &&
		send_interval <= 65000)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}
