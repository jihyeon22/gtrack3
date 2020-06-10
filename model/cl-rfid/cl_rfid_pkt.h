<<<<<<< HEAD
#ifndef __CL_RFID_PKT_H__
#define __CL_RFID_PKT_H__

#include "cl_rfid_tools.h"

#define REQ_PASSENGER_DATE_MARGIN_SEC   20

int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len, char* version);
int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff);

int make_clrfid_pkt__set_boarding(unsigned char **pbuf, unsigned short *packet_len, RFID_BOARDING_MGR_T* boarding);
int parse_clrfid_pkt__set_boarding(unsigned char * buff, int len_buff);

int clear_req_passenger_fail_cnt();

#endif // __CL_RFID_PKT_H__
=======
#ifndef __CL_RFID_PKT_H__
#define __CL_RFID_PKT_H__

#include "cl_rfid_tools.h"

#define REQ_PASSENGER_DATE_MARGIN_SEC   20

int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len, char* version);
int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff);

int make_clrfid_pkt__set_boarding(unsigned char **pbuf, unsigned short *packet_len, RFID_BOARDING_MGR_T* boarding);
int parse_clrfid_pkt__set_boarding(unsigned char * buff, int len_buff);

int clear_req_passenger_fail_cnt();

#endif // __CL_RFID_PKT_H__
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
