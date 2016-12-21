#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#include <logd_rpc.h>

#include "tools.h"

char * tools_strnstr(const char *str, const char *find, size_t str_len)
{
	char c, sc;
	size_t len;

	if((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if((sc = *str++) == '\0' || str_len-- < 1) {
					return (NULL);
				}
			} while(sc != c);
			if(len > str_len) {
				return (NULL);
			}
		} while(strncmp(str, find, len) != 0);
		str--;
	}
	return ((char *)str);
}

char * tools_strnchr(char *str, const char ch, const int str_len)
{
	int i;
	for(i = 0; i < str_len; i++)
	{
		if(str[i] == ch)
		{
			return &str[i];
		}
	}
	return NULL;
}

#define RETRY_GET_KERNELTIME 5
time_t tools_get_kerneltime()
{
	int n_try = RETRY_GET_KERNELTIME;
	int ret = 0;
	struct timespec ts;
	memset(&ts, 0x00, sizeof(struct timespec));

	while(n_try-- > 0)
	{
		ret = clock_gettime(CLOCK_MONOTONIC, &ts);
		if(ret == 0 && ts.tv_sec > 0)
		{
			break;
		}
		usleep(50000);
	}

	if(n_try < 0)
	{
		LOGE(eSVC_BASE, "%s> can't get kernel time over 10 times.\n", __FUNCTION__);
		ts.tv_sec = 0;
	}
#if 1
	else if(n_try < RETRY_GET_KERNELTIME-1)
	{
		LOGE(eSVC_BASE, "%s> warn %d %u\n", __FUNCTION__, n_try, ts.tv_sec);
	}
#endif

	if(ts.tv_sec <= 0)
	{
		LOGE(eSVC_BASE, "%s> ts.tv_sec is 0 %d\n", __FUNCTION__, n_try);
	}
	
	return ts.tv_sec;
}

int tools_write_data(const char *filename, unsigned char *buff, const int data_len, const BOOL append)
{
	FILE *fp;

	if(append == false)
	{
		fp = fopen(filename, "w+");
	}
	else
	{
		fp = fopen(filename, "a");
	}
	if(fp != NULL)
	{
		fwrite(buff, data_len, 1, fp);
		fflush(fp);
		fclose(fp);
		return 0;
	}
	return -1;
}

int tools_read_data(const char *filename, unsigned char *buff, const int buff_len)
{
	FILE *fp;
	int res = 0;
	fp = fopen(filename, "r");
	if(fp != NULL)
	{
		res = fread(buff, 1, buff_len, fp);
		fclose(fp);
		return res;
	}
	return -1;
}

int tools_null2space(char *buff, const int data_len)
{
	int i, res = 0;
	for(i = 0; i < data_len; i ++)
	{
		if(buff[i] == 0)
		{
			res++;
			buff[i] = 0x20;
		}
	}
	return res;
}

unsigned char tools_checksum_xor(const unsigned char *buff, const int data_len)
{
	int i;
	unsigned char ret = buff[0];
	for(i = 1 ; i < data_len ; i++)
	{
		ret ^= buff[i];
	}
	return ret;
}

int tools_check_exist_file(const char *filename, int timeout)
{
        while(timeout-- > 0) {
                if(access(filename, F_OK) == 0)
                        return 0;
                sleep(1);
        }
        return -1;
}

int tools_cp(const char *from, const char *to,  int overwrite)
{
	int fd_to = -1, fd_from = -1;
	char buf[4096] = {0};
	ssize_t nread = 0;
	int saved_errno = 0;
	struct stat stat_buf;

	fd_from = open(from, O_RDONLY | O_EXCL);
	if(fd_from < 0)
	{
		return -1;
	}

	fstat(fd_from, &stat_buf);

	if(overwrite)
	{
		fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, stat_buf.st_mode);
	}
	else
	{
		fd_to = open(to, O_WRONLY | O_CREAT, stat_buf.st_mode);
	}

	if(fd_to < 0)
	{
		goto out_error;
	}

	while(nread = read(fd_from, buf, sizeof buf), nread > 0)
	{
		char *out_ptr = buf;
		ssize_t nwritten;

		do {
			nwritten = write(fd_to, out_ptr, nread);

			if(nwritten >= 0)
			{
				nread -= nwritten;
				out_ptr += nwritten;
			}
			else if(errno != EINTR)
			{
				goto out_error;
			}
		} while(nread > 0);
	}

	if(nread == 0)
	{
		if(close(fd_to) < 0)
		{
			fd_to = -1;
			goto out_error;
		}
		close(fd_from);

		/* Success! */
		return 0;
	}

out_error:
	saved_errno = errno;

	if(fd_from >= 0)
	{
		close(fd_from);
	}
	
	if(fd_to >= 0)
	{
		close(fd_to);
	}

	errno = saved_errno;

	return -1;
}


//return -1 : error
//       other : available memory In KB
int tools_get_available_memory(void)
{
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if(meminfo == NULL)
        return -1;

    char line[256];
    while(fgets(line, sizeof(line), meminfo))
    {
        int ram;
        if(sscanf(line, "MemFree: %d kB", &ram) == 1)
        {
            fclose(meminfo);
            return ram;
        }
    }

    fclose(meminfo);
    return -1;
}

int tools_get_module_list(const char* module_name)
{
    FILE *moduleinfo = fopen("/proc/modules", "r");
    if(moduleinfo == NULL)
        return -1;

    char line[256];
    while(fgets(line, sizeof(line), moduleinfo))
    {
        if( strstr(line, module_name) != NULL)
        {
            fclose(moduleinfo);
            return 0;
        }
    }

    fclose(moduleinfo);
    return -1;
}

// tools_itoa doesn't set NULL at end of string.
int tools_itoa_11(char *buf, int out_str_len, const char *format, int value)
{
	//integer 32bit 's max length is 11.
	char tmp_buf[12] = {0};

	if(out_str_len <= 0)
	{
		return -1;
	}
	
	snprintf(tmp_buf, sizeof(tmp_buf)-1, format, value);
	memcpy(buf, tmp_buf, out_str_len);

	return 0;
}

// tools_itoa doesn't set NULL at end of string.
int tools_lftoa_19(char *buf, int out_str_len, const char *format, double value)
{
	//only support 19 characters
	//In the future, if you need more character then you increase array size.
	char tmp_buf[20] = {0};

	if(out_str_len <= 0)
	{
		return -1;
	}
	
	snprintf(tmp_buf, sizeof(tmp_buf)-1, format, value);
	memcpy(buf, tmp_buf, out_str_len);

	return 0;
}

void tools_write_procfs(char* value, char* procfs_path)
{
	int fd;
	fd = open(procfs_path, O_RDWR);
	if (fd > 0)
	{
		write(fd, value, strlen(value));
		close(fd);
	}
}

void tools_rm_all(char *dir)
{
        DIR *theFolder;
        struct dirent *next_file;
        char filepath[256];

        if(dir == NULL)
        {
                return;
        }

        theFolder = opendir(dir);
        if(theFolder == NULL)
        {
                return;
        }

        while ( (next_file = readdir(theFolder)) != NULL )
        {
                if (0==strcmp(next_file->d_name, ".") || 0==strcmp(next_file->d_name, "..")) { continue; }

                // build the path for each file in the folder
                snprintf(filepath, sizeof(filepath)-1, "%s/%s", dir, next_file->d_name);
                remove(filepath);
        }
		
		closedir(theFolder);
}

void tools_alive_end(void)
{
	system("/system/sbin/alive.notifier end");
}

