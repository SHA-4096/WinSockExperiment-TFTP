#include"tftp.h"



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
	SendBuffer[1] = op;//设置opcode
	if (op == READ_REQUEST) {
		cout << "Downloading data:"
			<< filename
			<< endl;
	}
	else if (op == WRITE_REQUEST) {
		cout << "Uploading data:"
			<< filename
			<< endl;
		DataPacketBlock = 0;
		//打开文件，直到WRQ完成后关闭
		WRQFileS.open(filename, ios::in | ios::binary);
	}
	else if (op == DATA) {
		//设置blocknum
		SendBuffer[3] = DataPacketBlock;
		//读取数据并塞到packet里面,创建一个大小为DataPakSize的数据包
		WRQFileS.read(SendBuffer + 4, DataPakSize - 4);
		int count = WRQFileS.gcount();
		SendBufLen = 4 + count;
		return 0;
	}
	else {
		cout << "Not a valid opcode!" << endl;
		return -1;
	}
	//op=RRQ或WRQ的时候执行
	strcpy_s(SendBuffer + OP_LEN, 500, filename);

	int fLen = strlen(filename) + 1;//求文件名串长度（包括结束符）
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

int TFTPCLI::SetErrorBuffer(int errcode, char* errmsg) {
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

/// <summary>
/// 从server获取数据
/// </summary>
/// <returns>正常接收：0；接收到RST：1；其他错误：-1</returns>
int TFTPCLI::RecvFromServer() {
	//从server获取数据，出错返回-1，否则返回0
	memset(SendBuffer, 0, sizeof(SendBuffer));//清空buffer
	int addrlen = sizeof(recvAddr);
	int stat = recvfrom(clientSocketFd, RecvBuffer, sizeof(RecvBuffer), 0, (struct sockaddr*)&recvAddr, &addrlen);
	int err = WSAGetLastError();
	if (stat < 0) {
		if (err == 10054) {
			cout << "Got RST from server" << endl;
			return 1;
		}
		cout << "Error ocured while receving packet,code=" << err << endl;
		return -1;
	}
	RecvBufLen = stat;//获取接收到的报文长度
	return 0;
}

/// <summary>
/// 使用Octet模式从远程服务器获取文件
/// </summary>
/// <param name="host">远程服务器地址</param>
/// <param name="filename">要获取的文件名</param>
/// <param name="port">目标端口</param>
/// <returns>0:success !0:fail</returns>
int TFTPCLI::GetFileFromRemote(char* host, char* filename, u_short port) {
	CreateSocket();
	SetServerAddr(host, port);
	SetRequestBuffer(READ_REQUEST, MODE_OCTET, filename);
	SendBufferToServer();
	//创建新文件
	RRQFileS.open(filename, ios::out | ios::binary);
	RRQFileS.close();
	//主循环
	int blocknum, errorcode, prevBlocknum;
	prevBlocknum = 0;
	for (;;) {
		RecvFromServer();//从server获取数据
		int op = RecvBuffer[1];

		switch (op) {
		case DATA:
			//通过检查收到的报文长度检查传输是否完成

			//设置报文
			blocknum = RecvBuffer[3];
			cout << "Got Block " << blocknum << endl;
			serverAddr.sin_port = recvAddr.sin_port;//设置目的端口为S-TID
			SetAckBuffer(blocknum);//设置ACK的blocknum
			//写入文件
			if (prevBlocknum == blocknum - 1) {
				cout << "Wrote Block " << blocknum << endl;
				RRQFileS.open(filename, ios::out | ios::app | ios::binary);
				RRQFileS.write(RecvBuffer + 4, RecvBufLen - 4);
				RRQFileS.close();//TODO 或许之后可以加个文件锁啥的
				prevBlocknum = blocknum;

			}
			if (RecvBufLen < DataPakSize) {
				cout << "Finished Receving Packet!"<<endl;
				return 0;
			}
			break;
		case TFTP_ERROR:
			errorcode = RecvBuffer[3];
			break;
		}

		//Sleep(1000);
		SendBufferToServer();
	}
	CloseSocket();
}

int TFTPCLI::PutFileToRemote(char* host, char* filename, u_short port) {
	CreateSocket();
	SetServerAddr(host, port);
	SetRequestBuffer(WRITE_REQUEST, MODE_OCTET, filename);
	SendBufferToServer();
	for (;;) {
		int stat = RecvFromServer();//从server获取数据
		if(stat == 1){
			//接收到了RST消息
			CloseSocket();
			cout << "Finished Uploading Data" << endl;
			return 0;
		}
		int op = RecvBuffer[1];
		switch (op) {
		case ACK:
			cout << "Got ACK for block" << int(RecvBuffer[3])<<endl;
			serverAddr.sin_port = recvAddr.sin_port;//设置目的端口为S-TID
			if (RecvBuffer[3] == DataPacketBlock) {
				//收到了上一个包的ACK则继续发送
				DataPacketBlock++;
				SetRequestBuffer(DATA, MODE_OCTET, filename);
			}
			else {
				//收到了无效的ACK，直接跳过
				continue;
			}
			break;
		case TFTP_ERROR:
			
			break;
		}
		Sleep(50);
		SendBufferToServer();
	}
}