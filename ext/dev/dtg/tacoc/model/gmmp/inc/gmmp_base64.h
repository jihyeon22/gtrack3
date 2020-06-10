/*
 * base64.h
 *
 *  Created on: 2012. 4. 17.
 *      Author: ongten
 */

#ifndef GMMP_HEADER_BASE64_H_
#define GMMP_HEADER_BASE64_H_

unsigned char *base64_decode(char *src, unsigned int len, unsigned int *out_len);
char *base64_encode(char *src, unsigned int len, unsigned int *out_len);

#endif /* GMMP_HEADER_BASE64_H_ */
