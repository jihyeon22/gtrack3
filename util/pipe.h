#ifndef __UTIL_PIPE_H__
#define __UTIL_PIPE_H__

#include <fcntl.h>

#define PIPE_MAX_NUM 5000
#define PIPE_MAX_SIZE 2*1024*1024

typedef struct pipeDevice pipeDevice_t;
struct pipeDevice
{
	int fd[2];
	int num;
	int size;
};

typedef struct pipeData pipeData_t;
struct pipeData
{
	char op;
	unsigned char *data;
	unsigned short size;
	unsigned char csum;
} __attribute__((packed));

int pipe_init(pipeDevice_t *pipe_dev);
int pipe_init_fd(int *fd);
int pipe_add_data(pipeDevice_t *pipe_dev, const char op, unsigned char *pdata, const unsigned short size);
int pipe_re_add_data(pipeDevice_t *pipe_dev, pipeData_t *opdata);
int pipe_wait_read_signal_sec(int nfds, fd_set *readfds, unsigned int timeout_sec);
int pipe_check_csum_data(const unsigned char *buff, const int buff_len, const unsigned char csum);
int pipe_read(pipeDevice_t *pipe_dev, pipeData_t *opdata);

#endif
