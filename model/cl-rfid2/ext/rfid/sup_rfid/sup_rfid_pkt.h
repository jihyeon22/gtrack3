#ifndef __SUP_RFID_PKT_H__
#define __SUP_RFID_PKT_H__



#include "ext/rfid/cl_rfid_tools.h"

#define REQ_PASSENGER_DATE_MARGIN_SEC   20

int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len, char* version);
int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff);

int make_clrfid_pkt__set_boarding(unsigned char **pbuf, unsigned short *packet_len, RFID_BOARDING_MGR_T* boarding);
int parse_clrfid_pkt__set_boarding(unsigned char * buff, int len_buff);


#if defined (SERVER_ABBR_CLR0) 
#define HTTP_URL__REQ_PASSENGER         "/cd/getpassengerlist_tiny3.aspx"
#define HTTP_URL__SET_BOARDING_LIST     "/cd/setboardinglist_tiny3.aspx"
#else
#define HTTP_URL__REQ_PASSENGER         "/cd/getpassengerlist_tiny3.aspx"
#define HTTP_URL__SET_BOARDING_LIST     "/cd/setboardinglist.aspx"
#endif

#endif // __SUP_RFID_PKT_H__