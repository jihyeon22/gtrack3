#ifndef __CL_RFID_PKT_H__
#define __CL_RFID_PKT_H__

int make_clrfid_pkt__req_passenger(unsigned char **pbuf, unsigned short *packet_len);
int parse_clrfid_pkt__req_passenger(unsigned char * buff, int len_buff);

#endif // __CL_RFID_PKT_H__
