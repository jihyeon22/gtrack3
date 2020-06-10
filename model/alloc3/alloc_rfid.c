#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <alloc_rfid.h>

#include <board/uart.h>
#include "Ftp_ClientCmd.h"

#include "alloc_util.h"

#include <sys/ioctl.h>

#include "debug.h"
#include "logd/logd_rpc.h"


int g_rfid_fd = -1;
extern int g_tl500_state;
char g_rfid_filename[32];    

static int _wait_read(int fd, unsigned char *buf, int buf_len, int ftime)
{
    fd_set reads;
    struct timeval tout;
    int result = 0;
    int len = 0;

    FD_ZERO(&reads);
    FD_SET(fd, &reads);

    while (1) {
        tout.tv_sec = 0;
        tout.tv_usec = ftime;
        result = select(fd + 1, &reads, 0, 0, &tout);
        if(result <= 0) //time out & select error
            return -1;

        if ( (len = read(fd, buf, buf_len)) <= 0)
            return -1;

        break; //success
    }

    return len;
}	
// -------------------------------------------
//  HIR2000B 와의 통신을 하기 위해 초기화 
//  BAUD : 115200 bps /dev/ttyHSL1 사용함.
// -------------------------------------------
int init_alloc_rfid_reader()
{
	g_rfid_fd = init_uart(UART_DEV_DEFAULT_PATH, UART_DEV_DEFAULT_BAUDRATE);

	return g_rfid_fd;
}
// -------------------------------------------
//  HIR2000B 로 읽은 UART Data 를 Header, Data, 분리함.
//  command 는 H1, H2, H3, H4 등
//  buff 는 Data 부분에 해당 됨. 
// -------------------------------------------
int get_alloc_rfid_reader(char *command, char* buff)
{
	int readcnt = 0;
	
	char *start_idx = 0;
	char *end_idx = 0;
	
	char read_buff[512] = {0,};
	
	char tmp[32] = {0,};
	
	// --------------------------------
	int data_lenth = 0;
	// --------------------------------
	
	readcnt = uart_read(g_rfid_fd, (unsigned char *) read_buff , 512, 60);

	//printf("readcnt : %d\r\n", readcnt);
	if ( !(readcnt > 0) )
	{
		return -1;
	}
	
	printf ("get_alloc_rfid_reader() [%s]\r\n", read_buff);
	
	start_idx = strstr(read_buff, RFID_READER_STX);
	
	if ( !start_idx )
	{
		printf("not found rfid start_idx cmd!!\r\n");
		return -1;
	}
	
	end_idx = strstr(read_buff, "\r\n");
	
	if ( !end_idx )
	{
		printf("not found rfid end_idx cmd!!\r\n");
		return -1;
	}	
	
	strncpy(tmp, start_idx + 9, 7);
	
	data_lenth = atoi(tmp);
	
	memset(command, 0x0, 16);
	strncpy(command, start_idx + 9 + 7, 2);

	memset(buff, '0', data_lenth);
	strncpy(buff, start_idx + 9 + 7 + 2, data_lenth);

	printf("data_lenth : %d buff : %s\n", data_lenth, buff);
	// data의 길이는 없지만, data_lenth 이 0인 경우에는 command 처리를 못 하므로 임의 넣어줌.
	if (strcmp(command, "H4") == 0)
	{
		data_lenth = 1;
	}

	return data_lenth;	
}
// -------------------------------------------
//  TL500 -> HIR2000B 
//  Alive Check에 time 및 상태 정보를 준다. 
// -------------------------------------------
int get_alloc_rfid_alivecheck(char* buff)
{
	char version[8];
	char filename[32];

	strncpy(version, buff, 8);

	memset(filename, '0', 24);
	memset(g_rfid_filename, '0', 24);
	strncpy(filename, buff + 8, 24);
	sprintf(g_rfid_filename, "%s", filename);
	//sprintf(filename, "%.24s", buff + 8);
	
	printf("get_alloc_rfid_alivecheck filename : %s\n",filename);
	printf("g_rfid_filename : %s\n",g_rfid_filename);

	set_circulating_bus(0);
	return 0;
}

// -------------------------------------------
//  TL500 -> HIR2000B 
//  Alive Check에 time 및 상태 정보를 준다. 
// -------------------------------------------
int set_alloc_rfid_alivecheck(int state)
{
	char cmd[1024] = {0,};
	int cmd_size = 0;
	time_t system_time;
	struct tm *timeinfo;
	
	cmd_size += sprintf(cmd + cmd_size, RFID_READER_STX);
	cmd_size += sprintf(cmd + cmd_size, "0000015"); 	// length
	cmd_size += sprintf(cmd + cmd_size, "H1"); 		// command
	
	time(&system_time);
	timeinfo = localtime ( &system_time );

	cmd_size += sprintf(cmd + cmd_size, "%04d%02d%02d%02d%02d%02d", 
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); // time

	cmd_size += sprintf(cmd + cmd_size,"%d", state); 		// state 1byte
	cmd_size += sprintf(cmd + cmd_size, "\r\n"); 		// stop : 0d0a

	uart_write(g_rfid_fd, cmd, cmd_size);

	printf("set_alloc_rfid_alivecheck!!!! [%s]\r\n", cmd);
	
	return 0;
}
// -------------------------------------------
//  TL500 -> HIR2000B 
//  Alive Check에 time 및 상태 정보를 준다. 
// -------------------------------------------
int get_alloc_rfid_circulating_bus(char* buff)
{
	char version[8];
	char filename[32];

	strncpy(version, buff, 8);

	memset(filename, '0', 24);
	memset(g_rfid_filename, '0', 24);
	strncpy(filename, buff + 8, 24);
	sprintf(g_rfid_filename, "%s", filename);
	//sprintf(filename, "%.24s", buff + 8);
	
	printf("get_alloc_rfid_alivecheck filename : %s\n",filename);
	printf("get_alloc_rfid_circulating_bus : %s\n",g_rfid_filename);

	set_circulating_bus(1);
	return 0;
}

// -------------------------------------------
//  TL500 -> HIR2000B 
//  순환 버스 일 때 time 및 상태 정보를 준다. 
// -------------------------------------------
int set_alloc_rfid_circulating_bus(int state)
{
	char cmd[1024] = {0,};
	int cmd_size = 0;
	time_t system_time;
	struct tm *timeinfo;
	
	cmd_size += sprintf(cmd + cmd_size, RFID_READER_STX);
	cmd_size += sprintf(cmd + cmd_size, "0000015"); 	// length
	cmd_size += sprintf(cmd + cmd_size, "H5"); 		// command
	
	time(&system_time);
	timeinfo = localtime ( &system_time );

	cmd_size += sprintf(cmd + cmd_size, "%04d%02d%02d%02d%02d%02d", 
		timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, 
		timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec); // time

	cmd_size += sprintf(cmd + cmd_size,"%d", state); 		// state 1byte
	cmd_size += sprintf(cmd + cmd_size, "\r\n"); 		// stop : 0d0a

	uart_write(g_rfid_fd, cmd, cmd_size);

	printf("set_alloc_rfid_circulating_bus!!!! [%s]\r\n", cmd);
	LOGI(LOG_TARGET,"set_alloc_rfid_circulating_bus!!!! [%s]\r\n", cmd);
	
	return 0;
}

// -------------------------------------------
//  TL500 -> HIR2000B 
//  서버로 부터 다운 받은 DB 파일에 정보를 준다.
// -------------------------------------------
int set_alloc_rfid_download_DBInfo(int fileSize, char *filename)
{
	char cmd[1024] = {0,};
	int cmd_size = 0;
	char name[32] = {0,};
	// char name1[32] = "20190515.db";

	cmd_size += sprintf(cmd + cmd_size, "at$$rfid=");
	cmd_size += sprintf(cmd + cmd_size, "0000038"); 			// length
	cmd_size += sprintf(cmd + cmd_size, "H2"); 					// command

	cmd_size += sprintf(cmd + cmd_size, "%010d", fileSize); 	// size

	getfilenameformat24(name, filename);
	// memset(name, '0', 24);
	// strncpy(name,name1, strlen(name1));

	cmd_size += sprintf(cmd + cmd_size, "%24s", name); 	// filename
	cmd_size += sprintf(cmd + cmd_size, "0000");  				// reserved
	cmd_size += sprintf(cmd + cmd_size, "\r\n"); 				// stop : 0d0a
	printf("cmd rfid reader!!!! [%s]\r\n", cmd);
	uart_write(g_rfid_fd, cmd, cmd_size);

	return 0;
}
// -------------------------------------------
//  TL500 -> HIR2000B 
//  서버로 부터 다운 받은 DB 파일을 나누어서 보낸다.
// -------------------------------------------
int set_alloc_rfid_download_DBfile(char *downloadfile, char *filename)
{	
	unsigned short total_count;
	unsigned short cur_count;
	int r_size;
	FILE *fp;
	unsigned char data_buf[FILE_DIV_VAL];
	unsigned char packet_buf[1024];
	
	int file_size;
	int pack_idx;
	int uresult;

	fp = fopen(downloadfile, "r"); 	
	g_rfid_fd = init_alloc_rfid_reader();

	fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

	printf("alloc file_size = [%d]\n", file_size);

	total_count = file_size / FILE_DIV_VAL;
	if(file_size % FILE_DIV_VAL != 0)
		total_count += 1;
	
	printf("total_count = [%d]\n", total_count);
	
	set_alloc_rfid_download_DBInfo(file_size, filename);

	cur_count = 1;
	fseek(fp, 0, SEEK_SET);

	while(1) {
				
		pack_idx = 0;
		if(cur_count > total_count) {
			printf("while exit #1 cur_count : %d\n", cur_count);
			break;
		}
		
		memset(data_buf, 0x00, sizeof(data_buf));
		r_size = fread(data_buf, 1, FILE_DIV_VAL, fp);	

		memcpy(&packet_buf[pack_idx], data_buf, r_size);
		pack_idx += r_size;
		
		uresult = uart_write(g_rfid_fd, packet_buf, pack_idx);
		int count = 0;
		while(1){
			ioctl(g_rfid_fd,TIOCOUTQ, &count);
			if(count == 0) break;
			usleep(1);
		}		

		cur_count += 1;	

	}
	fclose(fp);	

	return 0;
}
// -------------------------------------------
//  HIR2000B -> TL500 
//  DB 파일을 다 받았는지 확인한다. 
// -------------------------------------------
int get_alloc_rfid_download_DBAck(char *buff)
{
	int ret = 0;

	printf("get_alloc_rfid_download_DBAck : %s \r\n", buff);
	if(strcmp(buff,"succ") == 0)	
	{
		printf("buff successs\n");
		ret = 0;
	} 
	else
		ret = -1;

	return ret;
}
// -------------------------------------------
//  HIR2000B -> TL500 
//  리더기에 읽은 RF 카드나 QR 코드의 정보를 받는다.
// -------------------------------------------
int get_alloc_rfid_tagging(char *buff, int datalen)
{
	int tagginglen = 0;
	char time[14] = {0,};
	int time_len = 14;
	char tagginginfo[32] = {0,};

	printf("get_alloc_rfid_tagging : %s \r\n", buff);
	LOGI(LOG_TARGET, "get_alloc_rfid_tagging : %s \r\n", buff);
	memset(time, 0x00, time_len);
	strncpy(time, buff, time_len);
	

	memset(tagginginfo, 0x00, 32);
	tagginglen = datalen-time_len;
	strncpy(tagginginfo, buff+time_len, tagginglen);
	//printf(" -- time: %s  tagging data: %s  tagginglen : %d \r\n", time, tagginginfo, tagginglen);

	find_rfid(tagginginfo, tagginglen, time);
	//tagging_add_rfid(tagginginfo, get_recent_geo_fence(), time);

	set_alloc_rfid_taggingAck();
	return tagginglen;
}
// -------------------------------------------
//  TL500 -> HIR2000B 
//  Tagging 정보를 정상적으로 받았다고 Ack 를 보낸다. 
// -------------------------------------------
int set_alloc_rfid_taggingAck()
{
	char cmd[1024] = {0,};
	int cmd_size = 0;
	
	cmd_size += sprintf(cmd + cmd_size, RFID_READER_STX);
	cmd_size += sprintf(cmd + cmd_size, "0000000"); 	// length
	cmd_size += sprintf(cmd + cmd_size, "H3"); 		// command
	cmd_size += sprintf(cmd + cmd_size, "\r\n"); 		// stop : 0d0a

	printf("cmd rfid reader!!!! [%s]\r\n", cmd);
	uart_write(g_rfid_fd, cmd, cmd_size);
		
	return 0;
}
int set_alloc_rfid_request_DBAck(int state)
{
	char cmd[1024] = {0,};
	int cmd_size = 0;
	
	cmd_size += sprintf(cmd + cmd_size, RFID_READER_STX);
	cmd_size += sprintf(cmd + cmd_size, "0000001"); 	// length
	cmd_size += sprintf(cmd + cmd_size, "H4"); 		// command
	cmd_size += sprintf(cmd + cmd_size, "%d", state); 		// state
	cmd_size += sprintf(cmd + cmd_size, "\r\n"); 		// stop : 0d0a

	printf("cmd rfid reader!!!! [%s]\r\n", cmd);
	LOGI(LOG_TARGET,"cmd rfid reader!!!! [%s]\r\n", cmd);
	uart_write(g_rfid_fd, cmd, cmd_size);

	return 0;
}
// -------------------------------------------
//  파일 사이즈를 계산한다.
// -------------------------------------------
int get_file_size(char *filename) 
{
	int size;

    FILE *fp = fopen(filename, "r");
	if(fp == NULL) {
		printf("%s> fp null\n", __func__);
		return -1;
	}

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);

    printf("%s> file size %d\n", __func__, size);

    fclose(fp);
	
	return size;
}
