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
#define BUFFER_SIZE 560

#define DataPakSize 512

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
	int SendBufLen;//在设置buffer时同时设置，在发送buffer内容的时候使用
	char RecvBuffer[BUFFER_SIZE];
	int RecvBufLen;
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
	
	int fLen = strlen(filename)+1;//求文件名串长度（包括结束符）
	//设置type
	if (type == MODE_OCTET) {
		strcpy_s(SendBuffer + OP_LEN + fLen, BUFFER_SIZE, "octet");
		SendBufLen = 20 + fLen;
		return 0;
	}
	else {
		strcpy_s(SendBuffer + OP_LEN + fLen, BUFFER_SIZE, "netascii");
		SendBufLen = 20 + fLen;
		return 0;
	}
	cout << "Error when generating Request!" << endl;
	return -1;

}

int TFTPCLI::SetAckBuffer(int blocknum) {
	//设置Ack报文
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = ACK;
	SendBuffer[3] = blocknum;
	SendBufLen = 10;
	return 0;
}

int TFTPCLI::SetErrorBuffer(int errcode,char* errmsg) {
	//设置Error报文
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = TFTP_ERROR;
	SendBuffer[3] = errcode;
	strcpy(SendBuffer + 4, errmsg);
	SendBufLen = 10 + strlen(errmsg);
	return 0;
}

int TFTPCLI::SendBufferToServer() {
	//将buffer的内容发送到远程服务器
	int stat = sendto(clientSocketFd, SendBuffer, SendBufLen, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
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

int TFTPCLI::RecvFromServer(){
	//从server获取数据，出错返回-1，否则返回0
	memset(SendBuffer, 0, sizeof(SendBuffer));//清空buffer
	int addrlen = sizeof(recvAddr);
	int stat = recvfrom(clientSocketFd, RecvBuffer, sizeof(RecvBuffer),0, (struct sockaddr*)&recvAddr, &addrlen);
	int err = WSAGetLastError();
	if (stat < 0) {
		cout << "Error ocured while receving packet,code=" << err << endl;
		return -1;
	}
	RecvBufLen = stat;//获取接收到的报文长度
	return 0;
}


int TFTPCLI::GetFileFromRemote(char* host,char* filename,u_short port) {
	CreateSocket();
	SetServerAddr(host, port);
	SetRequestBuffer(READ_REQUEST, MODE_NETASCII, filename);
	SendBufferToServer();
	for (;;) {
		RecvFromServer();//从server获取数据
		int op = RecvBuffer[1];
		int blocknum, errorcode;
		switch (op) {
		case DATA:
			//通过检查收到的报文长度检查传输是否完成
			if (RecvBufLen < DataPakSize) {
				cout<<"Finished Receving Packet!";
				return 0;
			}

			blocknum = RecvBuffer[3];
			SetAckBuffer(blocknum);//设置ACK的blocknum
			serverAddr.sin_port = recvAddr.sin_port;//设置目的端口为S-TID
			break;
		case TFTP_ERROR:
			errorcode = RecvBuffer[3];
			break;
		}


		SendBufferToServer();
	}
	CloseSocket();
}