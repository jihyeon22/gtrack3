
#ifndef __TACOC_H__
#define __TACOC_H__

#include <unistd.h>

struct tacoc_stream {
    int type;
    size_t size;
    char *data;
};
typedef struct tacoc_stream tacoc_stream_t;


#endif // __TACOC_H__