#ifndef __MODEL_ALLOC_TAGGING_H__
#define __MODEL_ALLOC_TAGGING_H__

typedef struct
{
	int idx_geo_fence;
	int count;
	int tagging_num;
	int tagging_data_len;
	char date[15];
	char tagging_data[1]; //use pointer
} taggingData_t;

#define RFID_LIST_SIZE 350
#define RFID_DATA_LEN 15

#define SEND_NOT_EXIST_RFID 1
#define SEND_DUPLICATION_RFID 1

int tagging_add_rfid(char *rfid_pat, int geo_fence, char *rfid_date);
int tagging_add_count(int geo_fence);
void tagging_print_data(taggingData_t *d);
void tagging_add_list(void);

#endif

