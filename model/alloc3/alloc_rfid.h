#ifndef __ALLOC_RFID_H__
#define __ALLOC_RFID_H__

#define UART_DEV_DEFAULT_PATH          "/dev/ttyHSL1"
#define UART_DEV_DEFAULT_BAUDRATE      115200

#define RFID_READER_STX          "at$$rfid="

#define FILE_DIV_VAL		512

int init_alloc_rfid_reader();
int get_alloc_rfid_reader(char *command, char* buff);


int get_alloc_rfid_alivecheck(char* buff);
int set_alloc_rfid_alivecheck(int state);

int get_alloc_rfid_circulating_bus(char* buff);
int set_alloc_rfid_circulating_bus(int state);

int set_alloc_rfid_download_DBInfo(int fileSize, char *filename);
int set_alloc_rfid_download_DBfile(char *downloadfile, char *filename);

int get_alloc_rfid_download_DBAck(char *buff);
int get_alloc_rfid_tagging(char *buff, int datalen);

int set_alloc_rfid_taggingAck();
int set_alloc_rfid_request_DBAck(int state);

int get_file_size(char *filename);

#endif
