#ifndef __BOARD_POWER_H__
#define __BOARD_POWER_H__

#define POWER_SRC_DC				(0)
#define POWER_SRC_BATTERY			(1)

#define POWER_IGNITION_ON		(1)
#define POWER_IGNITION_OFF	(0)

int power_get_ignition_status(void);
int power_get_power_source(void);

#endif
