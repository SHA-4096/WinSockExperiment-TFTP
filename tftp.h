#pragma once

#include<winsock2.h>
#include<iostream>
#include<WS2tcpip.h>
#include<iostream>
#include<fstream>

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

#define DataPakSize 516

class TFTPCLI {
public:
	WSADATA wsaData;
	struct sockaddr_in serverAddr;
	struct sockaddr_in recvAddr;//用来存储接收信息的来源
	int InitSocket();
	int GetFileFromRemote(char* host, char* filename, u_short port);
	int PutFileToRemote(char* host, char* filename, u_short port);
private:
	SOCKET clientSocketFd;
	char SendBuffer[BUFFER_SIZE];
	int SendBufLen;//在设置buffer时同时设置，在发送buffer内容的时候使用
	char RecvBuffer[BUFFER_SIZE];
	int RecvBufLen;
	//socket操作相关
	int SetServerAddr(char* host, u_short port);
	int CreateSocket();
	int SetRequestBuffer(int op, int type, char* filename);//设置BUFFER内容为请求报文
	int SetAckBuffer(int blocknum);
	int SetErrorBuffer(int errcode,char* errmsg);
	int SendBufferToServer();
	int RecvFromServer();
	int CloseSocket();
	//文件操作相关
	ofstream RRQFileS;
	ifstream WRQFileS;
	//TFTP实现相关
	int DataPacketBlock;
	
	
};
