
// #define ALLOCTION_FTP_IP "219.251.4.181"
// #define ALLOCTION_FTP_PORT "21"
// #define ALLOCTION_FTP_ID "iotdm"
// #define ALLOCTION_FTP_PW "dmiot"

#define ALLOCTION_FTP_IP "218.153.6.212"
#define ALLOCTION_FTP_PORT "3032"
#define ALLOCTION_FTP_ID "dsme"
#define ALLOCTION_FTP_PW "appadm2012!@#"

#define DOWNLOAD_FILEPATH "dsme"
#define DOWNLOAD_FILENAME "20190320.db"
#define DOWNLOAD_SAVED_FILEINFO		CONCAT_STR(USER_DATA_DIR, "/alloc_ftpinfo.txt")

#define DOWNLOAD_SAVED_FILENAME	"/factory/mds/data/dsme/20190320.db"

#define ALLOCTION_FTP_HOST "virtual.mdstec.com"

#define BUFFER_SIZE 1024
#define FILENAME_SIZE 256
#define END_OF_PROTOCOL "\r\n"

int Ftp_initializeClient();
int Ftp_startClient(char *ip, char *port, char *id, char *pw, char *filename, char *desfilename);

int Ftp_openConnect(char *serverIp, char *serverPort, char *id, char *pw); 
int Ftp_getFile(char *serverfilename, char *desfilename);
int Ftp_quit();
int Ftp_passiveMode(char *ip, int *port);



