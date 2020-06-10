#ifndef __TACO_DATA_STORE_DEFINE_HEADER__
#define __TACO_DATA_STORE_DEFINE_HEADER__


#define MAX_ONE_DATA_COUNT	10 //MAX_INNO_DATA
#define DATA_PACK_EMPTY		0
#define DATA_PACK_AVAILABLE	1
#define DATA_PACK_FULL		2

//caller : create_store_taco(sizeof(tacom_inno_data_t), MAX_INNO_DATA_PACK
int  create_dtg_data_store(int max_pack_size);
void destory_dtg_store();
void store_recv_bank(unsigned char *buf, int size, unsigned char *read_curr_buf);
void saved_data_recovery(char *file_name, unsigned char *curr_data);
void save_record_data_taco(char *file_name);

int  get_dtg_current_count();
int  dtg_data_clear();
int  get_dtg_data(TACOM *tm, int dest_idx);


#endif
