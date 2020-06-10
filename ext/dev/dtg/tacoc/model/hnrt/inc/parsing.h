#pragma once

int hnrt_dtg_parsing(char *buf, int buf_len, char *stdbuf, int num_dtg_data);
int term_info_parsing(unsigned char *destbuf, tacom_std_hdr_t *srcbuf);
int mdt_parsing(char *destbuf, tacom_std_data_t *srcbuf);
