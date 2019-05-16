#ifndef __S_PRINT_FORMAT_H__
#define __S_PRINT_FORMAT_H__
 
#define S_COLOR_RED     "\x1b[31m"
#define S_COLOR_GREEN   "\x1b[32m"
//#define S_COLOR_YELLOW  "\x1b[33m"
#define S_COLOR_BLUE    "\x1b[34m"
#define S_COLOR_MAGENTA "\x1b[35m"
#define S_COLOR_CYAN    "\x1b[36m"
#define S_COLOR_RESET   "\x1b[0m"

#define S_COLOR_YELLOW  "\x1b[33m"

#define print_yellow(...) \
			printf("%c[1;33m",27); \
			printf(__VA_ARGS__); \
			printf("%c[0m\n",27);

#define print_red(...) \
			printf("%c[1;31m",27); \
			printf(__VA_ARGS__); \
			printf("%c[0m\n",27); 

#define print_blue(...) \
			printf("%c[1;34m",27); \
			printf(__VA_ARGS__); \
			printf("%c[0m\n",27); 
#endif /* __S_PRINT_FORMAT_H__ */

