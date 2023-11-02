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
#define MAX_RETRIES 10 //��������ش�����Ϊ10

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
	int TFTPState;//��¼��ǰ��״̬
	char MsgBuf[BUFFER_SIZE];//��ǰ��־�Ļ�����
private:
	//�������������
	char SendBuffer[BUFFER_SIZE];
	int SendBufLen;//������bufferʱͬʱ���ã��ڷ���buffer���ݵ�ʱ��ʹ��
	char RecvBuffer[BUFFER_SIZE];
	int RecvBufLen;
	int SetRequestBuffer(int op, int type, char* filename);//����BUFFER����Ϊ������
	int SetDataBuffer(int blocknum);
	int SetAckBuffer(int blocknum);
	int SetErrorBuffer(int errcode, char* errmsg);

	//socket�������
	SOCKET clientSocketFd;
	struct sockaddr_in recvAddr;//�����洢������Ϣ����Դ
	int SetServerAddr(char* host, u_short port);
	int CreateSocket();
	int SetSocketTimeout(int* sendTimeout, int* recvTimeout);
	int SendBufferToServer();
	int RecvFromServer();
	int CloseSocket();
	//�ļ��������
	ofstream RRQFileS;
	ifstream WRQFileS;

	//������Ϣ���
	std::chrono::steady_clock::time_point LastPacketSentTime;//������¼�����ٶ�
	double TransmitSpeed;
	int SetCurrentTime();
	int CalcSpeed();
	int STID;

	//��־���
	fstream LogFileS;
	int LogFatal(char* msg);
	int LogInfo(char* msg);
	int LogWarn(char* msg);
	time_t LogTimeObj;
	
	 
};
