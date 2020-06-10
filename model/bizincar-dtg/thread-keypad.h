#ifndef __MODEL_THREAD_H__
#define __MODEL_THREAD_H__

void thread_keypad(void);
void exit_thread_keypad(void);

#define KEY_RESULT_FALSE	-1
#define KEY_RESULT_TRUE		1

int keypad_server_result__clr();
int keypad_server_result__chk_set(int num, char* echo_str);
int keypad_server_result__set_result(int op, int result);
int keypad_server_result__get_result(int num, char* echo_str);

#endif

