<<<<<<< HEAD
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <base/error.h>
#include <util/tools.h>
#include "pipe.h"
#include <logd_rpc.h>
#include <netcom.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

int pipe_init(pipeDevice_t *pipe_dev)
{
	pipe_dev->fd[0] = -1;
	pipe_dev->fd[1] = -1;
	pipe_dev->num = 0;
	pipe_dev->size = 0;
	
	return pipe_init_fd(pipe_dev->fd);
}

int pipe_init_fd(int *fd)
{
	int flags;

	if(pipe(fd) == -1)
	{
		LOGE(LOG_TARGET, "pipe error!!!!!!!\n");
		return -1;
	}

	flags = fcntl(fd[0], F_GETFL);
	if(flags == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd getflag error!!!!!!!\n");
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd[0], F_SETFL, flags) == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd setflag error!!!!!!!\n");
		return -1;
	}

	flags = fcntl(fd[1], F_GETFL);
	if(flags == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd getflag error!!!!!!!\n");
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd[1], F_SETFL, flags) == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd setflag error!!!!!!!\n");
		return -1;
	}
	return 0;
}

int pipe_add_data(pipeDevice_t *pipe_dev, const char op, unsigned char *pdata, const unsigned short size)
{
	int res = 0;
	pipeData_t opdata;

	opdata.op = op;
	opdata.data = pdata;
	opdata.csum = tools_checksum_xor(pdata, size);
	opdata.size = size;

	// argument check
	if (pdata == NULL)
	{
		LOGE(LOG_TARGET, "%s err : argument is NULL \r\n", __FUNCTION__);
		return -1;
	}


	if(pipe_dev->num > PIPE_MAX_NUM || pipe_dev->size > PIPE_MAX_SIZE) {
		int read_size = 0;
		pipeData_t temp_opdata;
		
		read_size = read(pipe_dev->fd[0], &temp_opdata, sizeof(pipeData_t));
		if(read_size == -1) {
			LOGE(LOG_TARGET, "[network Thread] pipe size is max but pipe is empty\n");
		}
		else
		{
			LOGI(LOG_TARGET, "[network Thread] pipe size is full : num[%d], size[%d]\n", pipe_dev->num, pipe_dev->size);
			pipe_dev->num--;
			if(read_size == sizeof(temp_opdata))
			{
				pipe_dev->size -= temp_opdata.size;
				free(temp_opdata.data);
			}
			else
			{
				LOGE(LOG_TARGET, "read_size abnormal %d\n", read_size);
			}
		}
	}

	res = write(pipe_dev->fd[1], &opdata, sizeof(pipeData_t));
	if(res != sizeof(pipeData_t))
	{
		LOGE(LOG_TARGET, "%s> pfd[%d:%d] write pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		return -1;
	}

	//LOGT(LOG_TARGET, "sender_add_data_to_buffer ok : size [%d]\n", size);
	pipe_dev->num++;
	pipe_dev->size += opdata.size;
	
	return 0;
}

int pipe_re_add_data(pipeDevice_t *pipe_dev, pipeData_t *opdata)
{
	int res = 0;

	if(pipe_dev->num > PIPE_MAX_NUM || pipe_dev->size > PIPE_MAX_SIZE) {
		int read_size = 0;
		pipeData_t temp_opdata;
		
		read_size = read(pipe_dev->fd[0], &temp_opdata, sizeof(pipeData_t));
		if(read_size == -1) {
			LOGE(LOG_TARGET, "[network Thread] FATAL pipe size is max but pipe is empty\n");
		}
		else
		{
			LOGI(LOG_TARGET, "[network Thread] FATAL pipe size is full\n");
			pipe_dev->num--;
			if(read_size == sizeof(temp_opdata))
			{
				pipe_dev->size -= temp_opdata.size;
				free(temp_opdata.data);
			}
			else
			{
				LOGE(LOG_TARGET, "read_size abnormal %d\n", read_size);
			}
		}
	}

	res = write(pipe_dev->fd[1], opdata, sizeof(pipeData_t));
	if(res != sizeof(pipeData_t))
	{
		LOGE(LOG_TARGET, "%s> pfd[%d:%d] write pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		return -1;
	}

	pipe_dev->num++;
	pipe_dev->size += opdata->size;
	
	LOGT(LOG_TARGET, "%s success\n", __func__);
	return 0;
}

int pipe_wait_read_signal_sec(int nfds, fd_set *readfds, unsigned int timeout_sec)
{
	int ready = 0;
	struct  timeval tv;

	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	if( timeout_sec == 0 )
	{
		ready = select(nfds, readfds, NULL, NULL, NULL);
	}
	else
	{
		ready = select(nfds, readfds, NULL, NULL, &tv);
	}
	
	return ready;
}

int pipe_check_csum_data(const unsigned char *buff, const int buff_len, const unsigned char csum)
{
	if(buff == NULL)
	{
		return -1;
	}

	if(csum != tools_checksum_xor(buff, buff_len))
	{
		return -1;
	}
	
	return 0;
}

int pipe_read(pipeDevice_t *pipe_dev, pipeData_t *opdata)
{
	int read_size =0;
	
	if((read_size = read(pipe_dev->fd[0], opdata, sizeof(pipeData_t))) == -1) {
		LOGE(LOG_TARGET, "%s> pfd[%d:%d] read pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		error_critical(eERROR_LOG, "%s> pfd[%d:%d] read pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		return -1;
	}

	if(read_size == sizeof(pipeData_t))
	{
		pipe_dev->size -= opdata->size;
	}
	else
	{
		error_critical(eERROR_LOG, "%s> pfd[%d:%d] read pipe size error : [%dB]\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], read_size);
		return -1;
	}
	pipe_dev->num--;
	
	return 0;
}

=======
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <base/error.h>
#include <util/tools.h>
#include "pipe.h"
#include <logd_rpc.h>
#include <netcom.h>

// ----------------------------------------
//  LOGD Target
// ----------------------------------------
#define LOG_TARGET eSVC_NETWORK

int pipe_init(pipeDevice_t *pipe_dev)
{
	pipe_dev->fd[0] = -1;
	pipe_dev->fd[1] = -1;
	pipe_dev->num = 0;
	pipe_dev->size = 0;
	
	return pipe_init_fd(pipe_dev->fd);
}

int pipe_init_fd(int *fd)
{
	int flags;

	if(pipe(fd) == -1)
	{
		LOGE(LOG_TARGET, "pipe error!!!!!!!\n");
		return -1;
	}

	flags = fcntl(fd[0], F_GETFL);
	if(flags == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd getflag error!!!!!!!\n");
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd[0], F_SETFL, flags) == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd setflag error!!!!!!!\n");
		return -1;
	}

	flags = fcntl(fd[1], F_GETFL);
	if(flags == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd getflag error!!!!!!!\n");
		return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(fd[1], F_SETFL, flags) == -1) {
		LOGE(LOG_TARGET, "[network Thread] fd setflag error!!!!!!!\n");
		return -1;
	}
	return 0;
}

int pipe_add_data(pipeDevice_t *pipe_dev, const char op, unsigned char *pdata, const unsigned short size)
{
	int res = 0;
	pipeData_t opdata;

	opdata.op = op;
	opdata.data = pdata;
	opdata.csum = tools_checksum_xor(pdata, size);
	opdata.size = size;

	// argument check
	if (pdata == NULL)
	{
		LOGE(LOG_TARGET, "%s err : argument is NULL \r\n", __FUNCTION__);
		return -1;
	}


	if(pipe_dev->num > PIPE_MAX_NUM || pipe_dev->size > PIPE_MAX_SIZE) {
		int read_size = 0;
		pipeData_t temp_opdata;
		
		read_size = read(pipe_dev->fd[0], &temp_opdata, sizeof(pipeData_t));
		if(read_size == -1) {
			LOGE(LOG_TARGET, "[network Thread] pipe size is max but pipe is empty\n");
		}
		else
		{
			LOGI(LOG_TARGET, "[network Thread] pipe size is full : num[%d], size[%d]\n", pipe_dev->num, pipe_dev->size);
			pipe_dev->num--;
			if(read_size == sizeof(temp_opdata))
			{
				pipe_dev->size -= temp_opdata.size;
				free(temp_opdata.data);
			}
			else
			{
				LOGE(LOG_TARGET, "read_size abnormal %d\n", read_size);
			}
		}
	}

	res = write(pipe_dev->fd[1], &opdata, sizeof(pipeData_t));
	if(res != sizeof(pipeData_t))
	{
		LOGE(LOG_TARGET, "%s> pfd[%d:%d] write pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		return -1;
	}

	//LOGT(LOG_TARGET, "sender_add_data_to_buffer ok : size [%d]\n", size);
	pipe_dev->num++;
	pipe_dev->size += opdata.size;
	
	return 0;
}

int pipe_re_add_data(pipeDevice_t *pipe_dev, pipeData_t *opdata)
{
	int res = 0;

	if(pipe_dev->num > PIPE_MAX_NUM || pipe_dev->size > PIPE_MAX_SIZE) {
		int read_size = 0;
		pipeData_t temp_opdata;
		
		read_size = read(pipe_dev->fd[0], &temp_opdata, sizeof(pipeData_t));
		if(read_size == -1) {
			LOGE(LOG_TARGET, "[network Thread] FATAL pipe size is max but pipe is empty\n");
		}
		else
		{
			LOGI(LOG_TARGET, "[network Thread] FATAL pipe size is full\n");
			pipe_dev->num--;
			if(read_size == sizeof(temp_opdata))
			{
				pipe_dev->size -= temp_opdata.size;
				free(temp_opdata.data);
			}
			else
			{
				LOGE(LOG_TARGET, "read_size abnormal %d\n", read_size);
			}
		}
	}

	res = write(pipe_dev->fd[1], opdata, sizeof(pipeData_t));
	if(res != sizeof(pipeData_t))
	{
		LOGE(LOG_TARGET, "%s> pfd[%d:%d] write pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		return -1;
	}

	pipe_dev->num++;
	pipe_dev->size += opdata->size;
	
	LOGT(LOG_TARGET, "%s success\n", __func__);
	return 0;
}

int pipe_wait_read_signal_sec(int nfds, fd_set *readfds, unsigned int timeout_sec)
{
	int ready = 0;
	struct  timeval tv;

	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;

	if( timeout_sec == 0 )
	{
		ready = select(nfds, readfds, NULL, NULL, NULL);
	}
	else
	{
		ready = select(nfds, readfds, NULL, NULL, &tv);
	}
	
	return ready;
}

int pipe_check_csum_data(const unsigned char *buff, const int buff_len, const unsigned char csum)
{
	if(buff == NULL)
	{
		return -1;
	}

	if(csum != tools_checksum_xor(buff, buff_len))
	{
		return -1;
	}
	
	return 0;
}

int pipe_read(pipeDevice_t *pipe_dev, pipeData_t *opdata)
{
	int read_size =0;
	
	if((read_size = read(pipe_dev->fd[0], opdata, sizeof(pipeData_t))) == -1) {
		LOGE(LOG_TARGET, "%s> pfd[%d:%d] read pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		error_critical(eERROR_LOG, "%s> pfd[%d:%d] read pipe error : [%d] %s\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], errno, strerror(errno));
		return -1;
	}

	if(read_size == sizeof(pipeData_t))
	{
		pipe_dev->size -= opdata->size;
	}
	else
	{
		error_critical(eERROR_LOG, "%s> pfd[%d:%d] read pipe size error : [%dB]\n", __func__, pipe_dev->fd[0], pipe_dev->fd[1], read_size);
		return -1;
	}
	pipe_dev->num--;
	
	return 0;
}

>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
