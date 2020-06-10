<<<<<<< HEAD
#ifndef __GMMP_UTILLITY_FUNC_DEF_HEADER__
#define __GMMP_UTILLITY_FUNC_DEF_HEADER__

int is_file_exist(char *filename);
int gzlib_compress(unsigned char *buf_src, unsigned int orgsize);
unsigned char *get_encode_buffer();
unsigned long get_encode_length();

int load_ini_file();
void write_ini_initdata();
int read_ini_initdata();
int read_ini_config(void);
int write_ini_config(void);
int write_ini_tid(void);

#define MAX_BUFFER_SIZE	512*1024
#endif

=======
#ifndef __GMMP_UTILLITY_FUNC_DEF_HEADER__
#define __GMMP_UTILLITY_FUNC_DEF_HEADER__

int is_file_exist(char *filename);
int gzlib_compress(unsigned char *buf_src, unsigned int orgsize);
unsigned char *get_encode_buffer();
unsigned long get_encode_length();

int load_ini_file();
void write_ini_initdata();
int read_ini_initdata();
int read_ini_config(void);
int write_ini_config(void);
int write_ini_tid(void);

#define MAX_BUFFER_SIZE	512*1024
#endif

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
