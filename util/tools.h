#ifndef __UTIL_TOOLS_H__
#define __UTIL_TOOLS_H__

#include <stdlib.h>

#include <include/types.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

char * tools_strnstr(const char *str, const char *find, size_t str_len);
char * tools_strnchr(char *str, const char ch, const int str_len);
time_t tools_get_kerneltime(void);
int tools_write_data(const char *filename, unsigned char *buff, const int data_len, const BOOL append);
int tools_read_data(const char *filename, unsigned char *buff, const int buff_len);
int tools_null2space(char *buff, const int data_len);
unsigned char tools_checksum_xor(const unsigned char *buff, const int data_len);
int tools_check_exist_file(const char *filename, int timeout);
int tools_cp(const char *from, const char *to,  int overwrite);
int tools_get_available_memory(void);
int tools_get_module_list(const char* module_name);
int tools_itoa_11(char *buf, int out_str_len, const char *format, int value);
int tools_lftoa_19(char *buf, int out_str_len, const char *format, double value);

void tools_write_procfs(char* value, char* procfs_path);
void tools_rm_all(char *dir);
void tools_alive_end(void);

#define CheckBit(Data, Loc)             (((Data) & (0x01 << (Loc)))?1:0)

#endif

