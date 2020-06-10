#pragma once

//int bulk_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *pack_buf);
//int current_dtg_parsing(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int ev);
//void set_current_dtg_data(unsigned char *std_buff, int std_buff_len);


int status_data_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, int ev);

int dtg_data_header_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, unsigned short *dtg_record_pack_cnt);
int dtg_data_pack_parse(unsigned char *std_buff, int std_buff_len, unsigned char *dest, int dest_len, unsigned short dtg_record_pack_cnt);
void set_dtg_key_status(int flag);
int get_dtg_key_status();
