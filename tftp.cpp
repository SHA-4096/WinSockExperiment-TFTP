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

/// <summary>
/// 按毫秒设置发送和接收超时
/// </summary>
/// <param name="sendTimeout"></param>
/// <param name="recvTimeout"></param>
/// <returns></returns>
int TFTPCLI::SetSocketTimeout(int* sendTimeout, int* recvTimeout) {

	//设置发送超时
	if (SOCKET_ERROR == setsockopt(clientSocketFd, SOL_SOCKET, SO_SNDTIMEO, (char*)sendTimeout, sizeof(int)))
	{
		fprintf(stderr, "Set SO_SNDTIMEO error !\n");
	}

	//设置接收超时
	if (SOCKET_ERROR == setsockopt(clientSocketFd, SOL_SOCKET, SO_RCVTIMEO, (char*)recvTimeout, sizeof(int)))
	{
		fprintf(stderr, "Set SO_RCVTIMEO error !\n");
	}
	return 0;
}

int TFTPCLI::SendBufferToServer() {
	//将buffer的内容发送到远程服务器
	int bytesSent = sendto(clientSocketFd, SendBuffer, SendBufLen, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	int err = WSAGetLastError();
	if (bytesSent < 0) {
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
	memset(RecvBuffer, 0, sizeof(RecvBuffer));//清空buffer
	int addrlen = sizeof(recvAddr);
	int bytesrcv = recvfrom(clientSocketFd, RecvBuffer, sizeof(RecvBuffer), 0, (struct sockaddr*)&recvAddr, &addrlen);
	int err = WSAGetLastError();
	if (bytesrcv < 0) {
		if (err == 10054) {
			cout << "Got RST from server" << endl;
			return 1;
		}
		cout << "Error ocured while receving packet/server timed out,code=" << err << endl;
		return -1;
	}
	RecvBufLen = bytesrcv;//获取接收到的报文长度
	
	return 0;
}

/// <summary>
/// 使用Octet模式从远程服务器获取文件
/// </summary>
/// <param name="host">远程服务器地址</param>
/// <param name="filename">要获取的文件名</param>
/// <param name="port">目标端口</param>
/// <returns>0:success !0:fail</returns>
int TFTPCLI::GetFileFromRemote(char* host, char* filename, u_short port,int mode) {
	CreateSocket();//创建socket
	int timeout = 1000;//设置socket超时时间
	SetSocketTimeout(&timeout,&timeout );
	SetServerAddr(host, port);
	SetRequestBuffer(READ_REQUEST, mode, filename);
	SendBufferToServer();
	//主循环
	int blocknum, errorcode, prevBlocknum;
	prevBlocknum = 0;
	for (;;) {
		//超时重传机制
		int retries = 0;
		while (retries < MAX_RETRIES) {
			int state = RecvFromServer();//从server获取数据
			if (state < 0) {
				cout << "Failed to receive packet,retrying " << retries << endl;
				SendBufferToServer();//出错则重传
				retries++;
			}
			else if (state == 1) {
				//服务端已经重置连接了
				return -1;
			}else {
				break;
			}
		}
		if (retries == MAX_RETRIES) {
			cout << "Failed after " << MAX_RETRIES << " retries, exiting" << endl;
			return -1;
		}

		//成功接收到数据包后执行
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
				//计算速度并重置计时器
				CalcSpeed();
				SetCurrentTime();
				//输出速度
				cout << "Current Speed:"
					<< GetSpeed()
					<< " Bytes/s"
					<< endl;
				RRQFileS.write(RecvBuffer + 4, RecvBufLen - 4);
				prevBlocknum = blocknum;

			}
			if (RecvBufLen < DataPakSize) {
				cout << "Finished Receving Packet!"<<endl;
				SendBufferToServer();//发送ACK
				RRQFileS.close();//关闭文件
				CloseSocket();//关闭连接
				return 0;
			}
			break;
		case TFTP_ERROR:
			errorcode = RecvBuffer[3];
			break;
		}

		//Sleep(100);
		SendBufferToServer();
		
	}
	CloseSocket();
}

int TFTPCLI::PutFileToRemote(char* host, char* filename, u_short port,int mode) {
	CreateSocket();
	int timeout = 1000;//设置socket超时时间
	SetSocketTimeout(&timeout, &timeout);

	SetServerAddr(host, port);
	SetRequestBuffer(WRITE_REQUEST, mode, filename);
	SendBufferToServer();
	bool firstResp = false;//标记是否为第一次回应，是的话检查TID

	int dataPacketBlock = 0,pakStatus=0;//pakStatus标记当前数据包是否为最后一个
	for (;;) {
		//超时重传机制
		int retries = 0;
		while (retries < MAX_RETRIES) {
			int state = RecvFromServer();//从server获取数据
			if (state < 0) {
				cout << "Failed to receive packet,retrying " << retries << endl;
				SendBufferToServer();//出错则重传
				retries++;
			}
			else if (state == 1) {
				//服务端已经重置连接了
				return -1;
			}else {
				break;
			}
		}
		if (retries == MAX_RETRIES) {
			cout << "Failed after " << MAX_RETRIES << " retries, exiting" << endl;
			return -1;
		}

		
		int op = RecvBuffer[1];
		switch (op) {
		case ACK:
			cout << "Got ACK for block" << int(RecvBuffer[3])<<endl;
			serverAddr.sin_port = recvAddr.sin_port;//设置目的端口为S-TID
			if (RecvBuffer[3] == dataPacketBlock) {
				//收到了上一个包的ACK则继续发送
				dataPacketBlock++;
				pakStatus = SetDataBuffer(dataPacketBlock);
				//计算速度并重置计时器
				CalcSpeed();
				SetCurrentTime();
				//输出速度
				cout << "Current Speed:"
					<< GetSpeed()
					<< " Bytes/s"
					<< endl;
			}
			else {
				//收到了无效的ACK，直接跳过
				continue;
			}
			break;
		case TFTP_ERROR:
			cout << "Server reported Error!" 
				<<"code="
				<<int(RecvBuffer[3])
				<< endl;
			return -1;
			break;
		}

		if (pakStatus == 1) {
			//数据包发送完毕
			cout << "Finished Uploading Data" << endl;
			//TODO 等待最后的ACK，否则重传
			CloseSocket();
			WRQFileS.close();
			return 0;
		}
		//Sleep(500);
		SendBufferToServer();
		cout << "Current Speed:"
			<< GetSpeed() 
			<<" Bytes/s"
			<< endl;
	}
}

/// <summary>
/// 设置当前的传输速度,单位为字节
/// </summary>
/// <returns>成功返回0</returns>
int TFTPCLI::CalcSpeed() {
	if (LastPacketSentTime == 0) {
		return 0;
	}
	else {
		TransmitSpeed = double(DataPakSize - 4) / (double(GetTickCount() - LastPacketSentTime)/1000.00);
		return 0;
	}
}

/// <summary>
/// 设置计时开始点
/// </summary>
/// <returns></returns>
int TFTPCLI::SetCurrentTime() {
	// 获取系统启动时间的毫秒级时间戳
	LastPacketSentTime = GetTickCount();
	return 0;
}

double TFTPCLI::GetSpeed() {
	return TransmitSpeed;
}