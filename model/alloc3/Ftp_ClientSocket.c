<<<<<<< HEAD
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Ftp_ClientSocket.h"


int connectServer(char *serverIp, short port) {
	int sock;
	struct sockaddr_in servAddr;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("sock failed");
		printf("ftp sock failed\n");
		//exit(1);
		return -1;
	}
	
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(serverIp);
	servAddr.sin_port = htons(port);

	if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
		perror("connect failed");
		printf("ftp connect failed\n");
		return -1;
	}
	
	return sock;
}


void sendProtocol(int sock, char *protocol) {

	if (send(sock, protocol, strlen(protocol), 0) != strlen(protocol)) {
		perror("send failed");
		exit(1);
	}	

}

void recvProtocol(int sock, char *recvBuffer, int bufferSize) {
	int recvLen;
		
	if ((recvLen = recv(sock, recvBuffer, bufferSize-1, 0)) <= 0) {
		perror("recv failed");
		exit(1);
	}

	recvBuffer[recvLen] = '\0';

}


unsigned int downloadFile(int sock, char *filePath, unsigned int fileSize) {
	char readBuffer[TEMP_BUFFER_SIZE];
	unsigned int readBytes, totalBytes, numHash;
	struct stat obj;

	//int fd = open(filePath, O_WRONLY | O_CREAT, 0744);
	stat(filePath, &obj);
	int fd = open(filePath, O_CREAT | O_EXCL | O_WRONLY, 0755);

	fileSize = obj.st_size;
	printf("FileSize: %d\n", fileSize);

	totalBytes = numHash = 0;
	//while (totalBytes < fileSize) {
	while ((readBytes = read(sock, readBuffer, TEMP_BUFFER_SIZE)) > 0)	{

		write(fd, readBuffer, readBytes);
		totalBytes += readBytes;
	}
	close(fd);
	printf("\n");
	printf("totalBytes: %d\n", totalBytes);
	return totalBytes;
}



=======
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Ftp_ClientSocket.h"


int connectServer(char *serverIp, short port) {
	int sock;
	struct sockaddr_in servAddr;
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("sock failed");
		printf("ftp sock failed\n");
		//exit(1);
		return -1;
	}
	
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(serverIp);
	servAddr.sin_port = htons(port);

	if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
		perror("connect failed");
		printf("ftp connect failed\n");
		return -1;
	}
	
	return sock;
}


void sendProtocol(int sock, char *protocol) {

	if (send(sock, protocol, strlen(protocol), 0) != strlen(protocol)) {
		perror("send failed");
		exit(1);
	}	

}

void recvProtocol(int sock, char *recvBuffer, int bufferSize) {
	int recvLen;
		
	if ((recvLen = recv(sock, recvBuffer, bufferSize-1, 0)) <= 0) {
		perror("recv failed");
		exit(1);
	}

	recvBuffer[recvLen] = '\0';

}


unsigned int downloadFile(int sock, char *filePath, unsigned int fileSize) {
	char readBuffer[TEMP_BUFFER_SIZE];
	unsigned int readBytes, totalBytes, numHash;
	struct stat obj;

	//int fd = open(filePath, O_WRONLY | O_CREAT, 0744);
	stat(filePath, &obj);
	int fd = open(filePath, O_CREAT | O_EXCL | O_WRONLY, 0755);

	fileSize = obj.st_size;
	printf("FileSize: %d\n", fileSize);

	totalBytes = numHash = 0;
	//while (totalBytes < fileSize) {
	while ((readBytes = read(sock, readBuffer, TEMP_BUFFER_SIZE)) > 0)	{

		write(fd, readBuffer, readBytes);
		totalBytes += readBytes;
	}
	close(fd);
	printf("\n");
	printf("totalBytes: %d\n", totalBytes);
	return totalBytes;
}



>>>>>>> 13cf281973302551889b7b9d61bb8531c87af7bc
