#ifndef __MODEL_HDLC_ASYNC_H__
#define __MODEL_HDLC_ASYNC_H__

#include <stdint.h>

int hdlc_async_encode(uint8_t *dst, const uint8_t *src, int len);
int hdlc_async_decode(uint8_t *dst, const uint8_t *src, int len);
void hdlc_async_print_data(const uint8_t *base, int len);

#endif
