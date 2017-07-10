#include "validation.h"

int validation_model_interval(int interval)
{
	if(interval <= 65535)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int validation_model_maxpacket(int max_packet)
{
	if(max_packet <= 10)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int validation_model_port(int port)
{
	if(port >= 0 && port <=65535)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

