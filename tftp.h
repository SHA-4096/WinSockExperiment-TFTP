#pragma once

#include<winsock2.h>
#include<iostream>
#include<WS2tcpip.h>
#include<iostream>
#include<fstream>
#include<time.h>
#include<Windows.h>
#include<chrono>
#include<conio.h>
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
#define SPEED_THRESHHOLD 5000
#define MAX_RETRIES 10 //设置最大重传次数为10

#define STATE_SUCCESS 0
#define STATE_FAIL 1
#define STATE_RUNNING 2
#define STATE_SET 3

class TFTPCLI {
public:
	WSADATA wsaData;
	struct sockaddr_in serverAddr;
	int InitSocket();
	int InitLog(char filename[]);
	int GetFileFromRemote(char* host, char* filename, u_short port,int mode);
	int PutFileToRemote(char* host, char* filename, u_short port, int mode);
	double GetSpeed();
	int TFTPState;//记录当前的状态
	char MsgBuf[BUFFER_SIZE];//当前日志的缓冲区
private:
	//缓冲区设置相关
	char SendBuffer[BUFFER_SIZE];
	int SendBufLen;//在设置buffer时同时设置，在发送buffer内容的时候使用
	char RecvBuffer[BUFFER_SIZE];
	int RecvBufLen;
	int SetRequestBuffer(int op, int type, char* filename);//设置BUFFER内容为请求报文
	int SetDataBuffer(int blocknum);
	int SetAckBuffer(int blocknum);
	int SetErrorBuffer(int errcode, char* errmsg);

	//socket操作相关
	SOCKET clientSocketFd;
	struct sockaddr_in recvAddr;//用来存储接收信息的来源
	int SetServerAddr(char* host, u_short port);
	int CreateSocket();
	int SetSocketTimeout(int* sendTimeout, int* recvTimeout);
	int SendBufferToServer();
	int RecvFromServer();
	int CloseSocket();
	//文件操作相关
	ofstream RRQFileS;
	ifstream WRQFileS;

	//传输信息相关
	std::chrono::steady_clock::time_point LastPacketSentTime;//用来记录传输速度
	double TransmitSpeed;
	int SetCurrentTime();
	int CalcSpeed();
	int STID;

	//日志相关
	fstream LogFileS;
	int LogFatal(char* msg);
	int LogInfo(char* msg);
	int LogWarn(char* msg);
	time_t LogTimeObj;
	
	 
};
