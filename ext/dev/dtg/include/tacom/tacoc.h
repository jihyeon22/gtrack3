<<<<<<< HEAD

#ifndef __TACOC_H__
#define __TACOC_H__

#include <unistd.h>

struct tacoc_stream {
    int type;
    size_t size;
    char *data;
};
typedef struct tacoc_stream tacoc_stream_t;


=======

#ifndef __TACOC_H__
#define __TACOC_H__

#include <unistd.h>

struct tacoc_stream {
    int type;
    size_t size;
    char *data;
};
typedef struct tacoc_stream tacoc_stream_t;


>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
#endif // __TACOC_H__