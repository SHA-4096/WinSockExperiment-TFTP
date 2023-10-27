#pragma once

#include<winsock2.h>
#include<iostream>
#include<WS2tcpip.h>
#include<iostream>


#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define READ_REQUEST 1
#define WRITE_REQUEST 2
#define DATA 3
#define ACK 4
#define TFTP_ERROR 5
#define OP_LEN 2
#define MODE_OCTET 0
#define MODE_NETASCII 1
#define RECV_LOOP_MAX 10
#define TIMEOUT_SEC 10
#define BUFFER_SIZE 500

class TFTPCLI {
public:
	WSADATA wsaData;
	struct sockaddr_in serverAddr;
	struct sockaddr_in recvAddr;//用来存储接收信息的来源
	int InitSocket();
	int GetFileFromRemote(char* host, char* filename, u_short port);
//	int PutFileToRemote();
private:
	SOCKET clientSocketFd;
	char SendBuffer[BUFFER_SIZE];
	char RecvBuffer[BUFFER_SIZE*10];
	int SetServerAddr(char* host, u_short port);
	int CreateSocket();
	int SetRequestBuffer(int op, int type, char* filename);//设置BUFFER内容为请求报文
	int SetAckBuffer(int blocknum);
	int SetErrorBuffer(int errcode,char* errmsg);
	int SendBufferToServer();
	int RecvFromServer();
	int CloseSocket();
};


int TFTPCLI::InitSocket() {
	//初始化socket
	int nRC = WSAStartup(0x0101, &wsaData);
	if (nRC)
	{
		printf("Server initialize winsock error!\n");
		return -1;
	}
	if (wsaData.wVersion != 0x0101)
	{
		printf("Server's winsock version error!\n");
		WSACleanup();
		return -1;
	}
	printf("Server's winsock initialized !\n");
	return 0;
}

int TFTPCLI::SetServerAddr(char* host, u_short port) {
	//设置远程服务器的地址以及端口
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(host);
	serverAddr.sin_port = htons(port);
	return 0;
}


int TFTPCLI::CreateSocket() {
	//创建socket
	clientSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocketFd < 0) {
		perror("Create Socket Failed");
		return -1;
	}
	cout << "Create Socket Success" << endl;
	return 0;
}

int TFTPCLI::SetRequestBuffer(int op, int type, char* filename) {
	//生成报文，保存到buffer中
	memset(SendBuffer, 0, sizeof(SendBuffer));
	if (op == READ_REQUEST) {
		cout << "Downloading data:"
			<< filename;
	}
	else {
		cout << "Uploading data:"
			<< filename;

	}
	SendBuffer[1] = op;
	strcpy_s(SendBuffer +  OP_LEN, 500, filename);
	int fLen = strlen(filename)+1;//求文件名长度
	if (type == MODE_OCTET) {
		strcpy_s(SendBuffer + OP_LEN + fLen, BUFFER_SIZE, "octet");
		return 0;
	}
	else {
		strcpy_s(SendBuffer + OP_LEN + fLen, BUFFER_SIZE, "netascii");
		return 0;
	}
	return -1;

}

int TFTPCLI::SetAckBuffer(int blocknum) {
	//设置Ack报文
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = ACK;
	SendBuffer[3] = blocknum;
	return 0;
}

int TFTPCLI::SetErrorBuffer(int errcode,char* errmsg) {
	//设置Error报文
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = TFTP_ERROR;
	SendBuffer[3] = errcode;
	strcpy(SendBuffer + 4, errmsg);
	return 0;
}

int TFTPCLI::SendBufferToServer() {
	//将buffer的内容发送到远程服务器
	int stat = sendto(clientSocketFd, SendBuffer, BUFFER_SIZE, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	int err = WSAGetLastError();
	if (stat < 0) {
		cout << "Error ocured while sending packet,code=" << err << endl;
		return -1;
	}
	return 0;
}

int TFTPCLI::CloseSocket() {
	//关闭socket
	closesocket(clientSocketFd);
	return 0;
}

int TFTPCLI::RecvFromServer()
{
	memset(SendBuffer, 0, sizeof(SendBuffer));//清空buffer
	int addrlen = sizeof(recvAddr);
	int stat = recvfrom(clientSocketFd, RecvBuffer, sizeof(RecvBuffer),0, (struct sockaddr*)&recvAddr, &addrlen);
	int err = WSAGetLastError();
	if (stat < 0) {
		cout << "Error ocured while receving packet,code=" << err << endl;
		return -1;
	}

	return 0;
}


int TFTPCLI::GetFileFromRemote(char* host,char* filename,u_short port) {
	CreateSocket();
	SetServerAddr(host, port);
	SetRequestBuffer(READ_REQUEST, MODE_NETASCII, filename);
	SendBufferToServer();
	int cnt = 1;
	for (;;) {
		RecvFromServer();
		SetAckBuffer(cnt);//发送第cnt个包的ACK
		cnt++;
		SendBufferToServer();
	}
	CloseSocket();
}