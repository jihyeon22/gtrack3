#ifndef __UTIL_STORAGE_H__
#define __UTIL_STORAGE_H__

#define	ERR_NONE				0
#define ERR_FILE_NOT_EXIST		1
#define ERR_FILE_OPEN			2
#define ERR_MEM_ALLOC			3
#define ERR_FILE_READ			4
#define ERR_DATA_CRC			5

int storage_load_file(char *file, void *pdata, int length);
int storage_save_file(char *file, void *pdata, int length);

#endif
