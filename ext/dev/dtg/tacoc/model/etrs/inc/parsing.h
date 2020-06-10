#pragma once

int etrs_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *pack_buf, int data_id);
int term_info_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest, eTrace_vehicle_status_t vs);
//int term_info_parsing(char *destbuf, tacom_std_hdr_t *srcbuf);
//int mdt_parsing(char *destbuf, tacom_std_data_t *srcbuf);
