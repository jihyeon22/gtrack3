#ifndef __BASE_SENDER_H__
#define __BASE_SENDER_H__

#include <util/pipe.h>

typedef enum pipeSelection pipeSelection_t;
enum pipeSelection
{
	ePIPE_1 = 0
#ifdef USE_NET_THREAD2
	, ePIPE_2
#endif
};

extern int pfd_1[2];
extern int plen_1;
#ifdef USE_NET_THREAD2
extern int pfd_2[2];
extern int plen_2;
#endif
extern int gNetworkTriger_empty;
#ifdef USE_NET_THREAD2
extern int gNetworkTriger2_empty;
#endif

int sender_init(void);
void sender_deinit(void);
int sender_add_data_to_buffer(const char no_event, const void *param, pipeSelection_t type);
int sender_tx_data(pipeData_t *opdata, pipeSelection_t type);
int sender_get_num_remaindata(pipeSelection_t type);
int sender_get_size_remaindata(pipeSelection_t type);
int sender_wait_empty_network(const int timeout_sec);
void sender_set_network_fds(pipeSelection_t type, int *nfds, fd_set *readfds);
int sender_network_process(pipeSelection_t type, fd_set *readfds);

#endif

