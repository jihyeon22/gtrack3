<<<<<<< HEAD
#ifndef __MDT_TACO_DATA_STORE_DEFINE_HEADER__
#define __MDT_TACO_DATA_STORE_DEFINE_HEADER__


#define MDT_MAX_ONE_DATA_COUNT	1 //MAX_INNO_DATA
#define MDT_DATA_PACK_EMPTY		0
#define MDT_DATA_PACK_AVAILABLE	1
#define MDT_DATA_PACK_FULL		2

//caller : create_store_taco(sizeof(tacom_inno_data_t), MAX_INNO_DATA_PACK
int create_mdt_data_store(int max_pack_size);
void destory_mdt_store();
void mdt_store_recv_bank(unsigned char *buf, int size);
void saved_mdt_data_recovery(char *file_name);
void save_mdt_record_data_taco(char *file_name);

int get_mdt_current_count();
int mdt_data_clear();
int mdt_dtg_data(TACOM *tm, int dest_idx, int create_period);


#endif
=======
#ifndef __MDT_TACO_DATA_STORE_DEFINE_HEADER__
#define __MDT_TACO_DATA_STORE_DEFINE_HEADER__


#define MDT_MAX_ONE_DATA_COUNT	1 //MAX_INNO_DATA
#define MDT_DATA_PACK_EMPTY		0
#define MDT_DATA_PACK_AVAILABLE	1
#define MDT_DATA_PACK_FULL		2

//caller : create_store_taco(sizeof(tacom_inno_data_t), MAX_INNO_DATA_PACK
int create_mdt_data_store(int max_pack_size);
void destory_mdt_store();
void mdt_store_recv_bank(unsigned char *buf, int size);
void saved_mdt_data_recovery(char *file_name);
void save_mdt_record_data_taco(char *file_name);

int get_mdt_current_count();
int mdt_data_clear();
int mdt_dtg_data(TACOM *tm, int dest_idx, int create_period);


#endif
>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
