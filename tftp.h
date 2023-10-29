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
	struct sockaddr_in recvAddr;//�����洢������Ϣ����Դ
	int InitSocket();
	int GetFileFromRemote(char* host, char* filename, u_short port);
	int PutFileToRemote(char* host, char* filename, u_short port);
private:
	SOCKET clientSocketFd;
	char SendBuffer[BUFFER_SIZE];
	int SendBufLen;//������bufferʱͬʱ���ã��ڷ���buffer���ݵ�ʱ��ʹ��
	char RecvBuffer[BUFFER_SIZE];
	int RecvBufLen;
	//socket�������
	int SetServerAddr(char* host, u_short port);
	int CreateSocket();
	int SetRequestBuffer(int op, int type, char* filename);//����BUFFER����Ϊ������
	int SetAckBuffer(int blocknum);
	int SetErrorBuffer(int errcode,char* errmsg);
	int SendBufferToServer();
	int RecvFromServer();
	int CloseSocket();
	//�ļ��������
	ofstream RRQFileS;
	ifstream WRQFileS;
	//TFTPʵ�����
	int DataPacketBlock;
	
	
};
