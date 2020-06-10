#ifndef __CUST2_RFID_SENARIO_H__
#define __CUST2_RFID_SENARIO_H__

void cust2_rfid__init();
void cust2_rfid___main_senario();

int cust2_rfid__get_senario();
void cust2_rfid__set_senario(int flag);

int cust2_rfid__rfid_read_proc(char* buff);


#define CUST2_RFID_SENARIO__NEED_INIT           0
#define CUST2_RFID_SENARIO__DOING_INIT          1
#define CUST2_RFID_SENARIO__INIT_SUCCESS        2

#endif