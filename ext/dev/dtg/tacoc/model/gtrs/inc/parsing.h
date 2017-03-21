#pragma once

int bulk_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *pack_buf);
int current_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int ev);
void set_current_dtg_data(unsigned char *std_buff, int std_buff_len);
