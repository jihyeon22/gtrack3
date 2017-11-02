#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <base/devel.h>
#include <base/error.h>
#include <util/pipe.h>
#include <logd_rpc.h>
#include <netcom.h>
#include "sender.h"


// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

pipeDevice_t pipe_1;
#ifdef USE_NET_THREAD2
pipeDevice_t pipe_2;
#endif

int gNetworkTriger_empty = 0;
#ifdef USE_NET_THREAD2
int gNetworkTriger2_empty = 0;
#endif

#ifdef USE_NET_THREAD2
int dbg_pfd[2] = {-1,-1};
#else
int dbg_pfd[1] = {-1};
#endif

int sender_init(void)
{
	if(pipe_init(&pipe_1) < 0)
	{
		return -1;
	}

#ifdef USE_NET_THREAD2	
	if(pipe_init(&pipe_2) < 0)
	{
		sender_deinit();
		return -1;
	}
#endif
	
	dbg_pfd[0] = pipe_1.fd[0];

#ifdef USE_NET_THREAD2
	dbg_pfd[1] = pipe_2.fd[0];
#endif
	
	return 0;
}

void sender_deinit(void)
{
	if(pipe_1.fd[0] >= 0)
	{
		close(pipe_1.fd[0]);
	}
	if(pipe_1.fd[1] >= 0)
	{
		close(pipe_1.fd[1]);
	}

#ifdef USE_NET_THREAD2
	if(pipe_2.fd[0] >= 0)
	{
		close(pipe_2.fd[0]);
	}
	if(pipe_2.fd[1] >= 0)
	{
		close(pipe_2.fd[1]);
	}
#endif

}

int sender_add_data_to_buffer(const char no_event, const void *param, pipeSelection_t type)
{
	unsigned char *pdata = NULL;
	unsigned short size;
	int res = 0;

	if(make_packet(no_event, &pdata, &size, param) < 0)
	{
		LOGE(LOG_TARGET, "make_packet error [%d]\n", no_event);
		return -1;
	}

	//pdata buffer null check by jwrho++
	if(pdata == NULL) {
		LOGE(LOG_TARGET, "make_packet buffer is NULL, length[%d]\n", size);
		return -1;
	}
	//pdata buffer null check by jwrho--

	if(type == ePIPE_1)
	{
		res = pipe_add_data(&pipe_1, no_event, pdata, size);
	}
#ifdef USE_NET_THREAD2
	else
	{
		res = pipe_add_data(&pipe_2, no_event, pdata, size);
	}
#endif

	if(res < 0)
	{
		LOGE(LOG_TARGET, "pipe_add_data error [%d]\n", no_event);
		if(pdata != NULL)
		{
			free_packet(pdata);
		}
		return -1;
	}

	LOGT(LOG_TARGET, "sender_add_data_to_buffer ok. size[%d] %d:%d\n",
		size, type, sender_get_num_remaindata(type));

	return 0;
}

int sender_tx_data(pipeData_t *opdata, pipeSelection_t type)
{
	int res = 0;

	devel_webdm_send_status_current(opdata->op);
	LOGT(LOG_TARGET, "%s> evt type:%d\n", __func__, opdata->op);

	res = send_packet(opdata->op, opdata->data, opdata->size);
	if(res < 0)
	{
		if(type == ePIPE_1)
		{
			pipe_re_add_data(&pipe_1, opdata);
		}
#ifdef USE_NET_THREAD2
		else
		{
			pipe_re_add_data(&pipe_2, opdata);
		}
#endif
	}
	else
	{
		free_packet(opdata->data);
	}

	return res;
}

int sender_get_num_remaindata(pipeSelection_t type)
{
	if(type == ePIPE_1)
	{
		return pipe_1.num;
	}
#ifdef USE_NET_THREAD2
	else
	{
		return pipe_2.num;
	}
#endif
	return 0;
}

int sender_get_size_remaindata(pipeSelection_t type)
{
	if(type == ePIPE_1)
	{
		return pipe_1.size;
	}
#ifdef USE_NET_THREAD2
	else
	{
		return pipe_2.size;
	}
#endif
	return 0;
}

int sender_wait_empty_network(const int timeout_sec)
{
	int timeout = timeout_sec;

	gNetworkTriger_empty = 0;

#ifdef USE_NET_THREAD2
	gNetworkTriger2_empty = 0;
#endif

	while(timeout-- > 0)
	{
		int wait_flag = gNetworkTriger_empty;

#ifdef USE_NET_THREAD2
		wait_flag = gNetworkTriger_empty && gNetworkTriger2_empty;
#endif

		if(wait_flag)
		{
			return 0;
		}
		sleep(1);
	}
	return -1;
}

void sender_set_network_fds(pipeSelection_t type, int *nfds, fd_set *readfds)
{
	if(type == ePIPE_1)
	{
		LOGT(LOG_TARGET, "pfd1 %d %d\n", pipe_1.fd[0], pipe_1.fd[1]);
		*nfds = pipe_1.fd[0] + 1;

		FD_ZERO(readfds);
		FD_SET(pipe_1.fd[0], readfds);
	}
#ifdef USE_NET_THREAD2
	else
	{
		LOGT(LOG_TARGET, "pfd2 %d %d\n", pipe_2.fd[0], pipe_2.fd[1]);
		*nfds = pipe_2.fd[0] + 1;

		FD_ZERO(readfds);
		FD_SET(pipe_2.fd[0], readfds);
	}
#endif
}

int sender_network_process(pipeSelection_t type, fd_set *readfds)
{
	pipeDevice_t *pipe_dev;
	pipeData_t opdata;

	if(type == ePIPE_1)
	{
		pipe_dev = &pipe_1;
	}
#ifdef USE_NET_THREAD2
	else
	{
		pipe_dev = &pipe_2;
	}
#endif

	if(!FD_ISSET(pipe_dev->fd[0], readfds))
	{
		LOGE(LOG_TARGET, "err!! : %s / %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	memset(&opdata, 0, sizeof(opdata));

	if(pipe_read(pipe_dev, &opdata) < 0) {
		LOGE(LOG_TARGET, "[network Thread] type:%d : read error [%d] : %s\n", type, errno, strerror(errno));
#ifdef USE_NET_THREAD2
		error_critical(eERROR_LOG, "[network Thread] read error : type:%d, p1:%d, p2:%d, cur:%d, dbg:%d/%d",
			type, pipe_1.fd[0], pipe_2.fd[0], pipe_dev->fd[0], dbg_pfd[0], dbg_pfd[1]);
#else
		error_critical(eERROR_LOG, "[network Thread] read error : type:%d, p1:%d, cur:%d, dbg:%d",
			type, pipe_1.fd[0], pipe_dev->fd[0], dbg_pfd[0]);
#endif
//		devel_webdm_send_log("[network Thread] read error : type:%d, p1:%d, p2:%d, cur:%d, dbg:%d/%d",
//			type, pipe_1.fd[0], pipe_2.fd[0], pipe_dev->fd[0], dbg_pfd[0], dbg_pfd[1]);

		return -1;
	}
	
	//LOGT(LOG_TARGET, "[network Thread] run operation <%d> <%x> remain <%d> type <%d>\n", opdata.op, (int)opdata.data, *p_plen, type);

	if(pipe_check_csum_data(opdata.data, opdata.size, opdata.csum) < 0)
	{
		free_packet(opdata.data);
		LOGE(LOG_TARGET, "[network Thread] tools_checksum_xor error!!!\n");
		error_critical(eERROR_LOG, "[network Thread] tools_checksum_xor error");
		devel_send_sms_noti("check sum error", strlen("check sum error"), 3);
		return -1;
	}

	if(sender_tx_data(&opdata, type) < 0)
	{
		LOGE(LOG_TARGET, "[network Thread] sender_tx_data error!!!\n");
	}
	
	return 0;
}

