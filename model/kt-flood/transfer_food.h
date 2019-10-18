#ifndef __MODEL_TRANSFER_FLOOD_H__
#define __MODEL_TRANSFER_FLOOD_H__

#include <util/list.h>

// char uniqueID[5];

int transfer_packet_recv_etxflood(const transferSetting_t *setting, const unsigned char *tbuff, const int tbuff_len, unsigned char *rbuff, int rbuff_len, int etx);
#endif
