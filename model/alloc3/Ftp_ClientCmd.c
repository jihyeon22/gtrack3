#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "Ftp_ClientSocket.h"
#include "Ftp_ClientCmd.h"
#include "alloc_packet_tool.h"

// sock - PI socket ,  dtpSock - DTP socket
int sock, dtpSock;
int mode;

int Ftp_initializeClient() {
	printf("initialized\n");
	return 0;
}

// ftp client start 
int Ftp_startClient(char *ip, char *port, char *id, char *pw, char *filename, char *desfilename) {
	int ret = 0;

	ret = Ftp_initializeClient();
	if(ret < 0)
		return ret;
  	
	//Ftp_openConnect(ip, port);
	ret = Ftp_openConnect(ip, port, id, pw);
	if(ret < 0)
		return ret;

	ret = Ftp_getFile(filename, desfilename);
	if(ret < 0)
		return ret;

	return ret;
}

// ftp server connect 
int Ftp_openConnect(char *serverIp, char *serverPort, char *id, char *pw) {
	int ret = 0;
	char sendBuffer[BUFFER_SIZE];
	char recvBuffer[BUFFER_SIZE];

	// connect to server
	sock = connectServer(serverIp, atoi(serverPort));
	if(sock == -1)
	{
		printf("FTP connect failed !!!\n");
		devel_webdm_send_log("FTP sever connect failed !!!");
		return -1;
	}
	recvProtocol(sock, recvBuffer, BUFFER_SIZE-1);

	sprintf(sendBuffer, "User %s\r\n", id);
	sendProtocol(sock, sendBuffer);
	recvProtocol(sock, recvBuffer, BUFFER_SIZE);
	printf(recvBuffer);

	// send password
	sprintf(sendBuffer, "PASS %s\r\n", pw);
	sendProtocol(sock, sendBuffer);
	recvProtocol(sock, recvBuffer, BUFFER_SIZE);
	printf(recvBuffer);
	if(strstr(recvBuffer,"User cannot log in") != NULL)
	{
		printf("[FTP] User cannot log in!!!\n");
		devel_webdm_send_log("[FTP] FTP User cannot log in!!!");
		return -1;
	}

	// get server os information	
	sprintf(sendBuffer, "SYST%s", END_OF_PROTOCOL);
	sendProtocol(sock, sendBuffer);
	recvProtocol(sock, recvBuffer, BUFFER_SIZE-1);
	printf(recvBuffer);

	if(strstr(recvBuffer,"Please login with USER and PASS") != NULL)
	{
		printf("[FTP] Please login with USER and PASS!!!\n");
		devel_webdm_send_log("[FTP] Please login with USER and PASS!!!");
		return -1;
	}

	return ret;
}

// send EPSV or PASS to Server
int Ftp_passiveMode(char *ip, int *port) {
	int ret = 0;
	char sendBuffer[BUFFER_SIZE];
	char recvBuffer[BUFFER_SIZE];
	int host0, host1, host2, host3;
	int port0, port1;
	
	sprintf(sendBuffer, "PASV%s", END_OF_PROTOCOL);
	sendProtocol(sock, sendBuffer);
	recvProtocol(sock, recvBuffer, BUFFER_SIZE-1);
	printf(recvBuffer);
	
	sscanf(strchr(recvBuffer, '(')+1, "%d,%d,%d,%d,%d,%d", &host0, &host1, &host2, &host3, &port0, &port1);
	sprintf(ip, "%d.%d.%d.%d", host0, host1, host2, host3);

	*port = port0*256 + port1;
	
	printf("ip : %s\n", ip);
	printf("dtp port : %d\n", *port);

	return ret;
}

int Ftp_getFile(char *serverfilename, char *desfilename) {
	int ret = 0;
	int port;
	unsigned int fileSize;
	char ip[16], fileName[50];//DestFile[FILENAME_SIZE], fileName[50];
	char sendBuffer[BUFFER_SIZE];
	char recvBuffer[BUFFER_SIZE];

	sprintf(fileName, "%s", serverfilename);
	// sprintf(DestFile, "%s%s", USER_DATA_DIR, serverfilename);

	printf("get fileName: %s\n", fileName);
	printf("get filePath: %s\n", desfilename);

	Ftp_passiveMode(ip, &port);
	
	// connect to DTP
	dtpSock = connectServer(ip, port);
	
	// request server for transfer start - RETR fileName
	sprintf(sendBuffer, "RETR %s%s", fileName, END_OF_PROTOCOL);

	printf("sendBuffer : %s\n",sendBuffer);
	sendProtocol(sock, sendBuffer);
	recvProtocol(sock, recvBuffer, BUFFER_SIZE);
	printf(recvBuffer);
	
	if(strstr(recvBuffer,"cannot find the file") != NULL)
	{
		printf("FTP cannot find the file : %s\n", fileName);
		devel_webdm_send_log("FTP cannot find the file : %s", fileName);
		close(dtpSock);		

		return -1;
	}
	else
	{
		downloadFile(dtpSock, desfilename, fileSize);
	
		// recv complete message from PI server
		recvProtocol(sock, recvBuffer, BUFFER_SIZE);
		printf(recvBuffer);
		close(dtpSock);
	}
	
	return ret;
}

// ftp client exit 
int Ftp_quit() {
	int ret = 0;
	char sendBuffer[BUFFER_SIZE];
	char recvBuffer[BUFFER_SIZE];
	printf("quit\n");
	
	sprintf(sendBuffer, "QUIT%s", END_OF_PROTOCOL);
	sendProtocol(sock, sendBuffer);
	recvProtocol(sock, recvBuffer, BUFFER_SIZE);
	printf(recvBuffer);
	
	close(sock);
	exit(0);

	return ret;
}




