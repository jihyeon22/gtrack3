#ifndef __MODEL_ALLOC_BUZZER_H__
#define __MODEL_ALLOC_BUZZER_H__

#define GPIO_SRC_NUM_BUZZER1	11
#define GPIO_SRC_NUM_BUZZER2	13

enum
{
	BUZZER1 = 0,
	BUZZER2
};

int buzzer_init(void);
int buzzer_run(int type, int n_repeat, int buzzer_ms, int delay_ms);

#endif

