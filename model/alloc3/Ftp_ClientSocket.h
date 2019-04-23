#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEMP_BUFFER_SIZE 1024

int connectServer(char *serverIp, short port);
void sendProtocol(int sock, char *protocol);
void recvProtocol(int sock, char *recvBuffer, int bufferSize);
unsigned int downloadFile(int sock, char *filePath, unsigned int fileSize);
// unsigned int uploadFile(int sock, char *filePath, int hashFlag);

