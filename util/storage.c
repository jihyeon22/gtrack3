#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <util/crc16.h>
#include <util/tools.h>
#include "storage.h"

//////////////////////////////////////////////////
//warning ***************************************
//when structure save, it must use pragma pack
//////////////////////////////////////////////////

int _isExistFile(char *file, int timeout)
{
	while(timeout--) {
		if(access(file, F_OK) == 0) {
			return 0;
		}
		printf(".\n");
		sleep(1);
	}
	printf("\n=============================== \n");
	printf("%s> %s can not open : err[%d]\n", __func__, file, errno);
	printf("=============================== \n\n");
	return -1;
}
int _file_recovery(char *file)
{
	char recovery[128] = {0};

	sprintf(recovery, "%s.bak", file);
	if(tools_cp(recovery, file, 1) < 0)
	{
		return -ERR_FILE_NOT_EXIST;
	}

	return ERR_NONE;
}

void _file_backup(char *file)
{
	char backup[128] = {0};
	
	if(_isExistFile(file, 1) >= 0) {
		sprintf(backup, "%s.bak", file);
		tools_cp(file, backup, 1);
	}
}

int _load_file(char *file, void *pdata, int length)
{
		int err = ERR_NONE;
	unsigned short fcrc16;
	unsigned short crc16;
	int fd;
	int nreadbytes;
	int retry;
	unsigned char *pbuf;
	memset(pdata, 0x00, length);

	pbuf = (unsigned char *)malloc(length + 2);
	if(pbuf == NULL)
		return -ERR_MEM_ALLOC;

	retry = 0;
	while(retry++ < 3) {
		fd = open(file, O_RDONLY);
		if(fd <= 0) {
			err = -ERR_FILE_OPEN;
			continue;
		}

		nreadbytes = read(fd, pbuf, (length+2));
		close(fd);

		if(nreadbytes == (length+2))
			break;

		err = -ERR_FILE_READ;
	}

	//read fail
	if(retry >= 3) {
		printf("read file length error!!!\n");
		free(pbuf);
		close(fd);
		return err;
	}
	
	//read ok
	memcpy(&fcrc16, &pbuf[length], 2);
	crc16_get(NULL, 0);		
	crc16 = crc16_get(pbuf, length);

	if(fcrc16 != crc16) { //crc error
		printf("geo fence crc error, but will recovery.\n");
		free(pbuf);
		close(fd);
		return -ERR_DATA_CRC;
	}
		
	memcpy(pdata, pbuf, length);
	close(fd);
	free(pbuf);
	return ERR_NONE;
}

int storage_load_file(char *file, void *pdata, int length)
{
	int ret;

	if(_isExistFile(file, 3) < 0) {
		if(_file_recovery(file) != ERR_NONE)
			return -ERR_FILE_NOT_EXIST;
	}

	ret = _load_file(file, pdata, length);

	if(ret == -ERR_DATA_CRC) {
		if(_file_recovery(file) == ERR_NONE)
			ret = _load_file(file, pdata, length);
	}

	return ret;
}

int storage_save_file(char *file, void *pdata, int length)
{
	int fd;
	unsigned short crc16;

	_file_backup(file);

	fd = open( file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd < 0) {
		printf("%s> %s file open fail : err[%d]\n", __func__, file, errno);
		return -ERR_FILE_OPEN;
	}

	crc16_get(NULL, 0);		
	crc16 = crc16_get((unsigned char *)pdata, length);

	write(fd, pdata, length);
	write(fd, &crc16, 2);
	close(fd);

	return 0;
}

