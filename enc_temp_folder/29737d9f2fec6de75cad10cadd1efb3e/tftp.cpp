#include"tftp.h"



int TFTPCLI::InitSocket() {
	//��ʼ��socket
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
	//����Զ�̷������ĵ�ַ�Լ��˿�
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(host);
	serverAddr.sin_port = htons(port);
	return 0;
}

int TFTPCLI::CreateSocket() {
	//����socket
	clientSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocketFd < 0) {
		perror("Create Socket Failed");
		return -1;
	}
	cout << "Create Socket Success" << endl;
	return 0;
}

/// <summary>
/// ���������÷��ͺͽ��ճ�ʱ
/// </summary>
/// <param name="sendTimeout"></param>
/// <param name="recvTimeout"></param>
/// <returns></returns>
int TFTPCLI::SetSocketTimeout(int* sendTimeout, int* recvTimeout) {

	//���÷��ͳ�ʱ
	if (SOCKET_ERROR == setsockopt(clientSocketFd, SOL_SOCKET, SO_SNDTIMEO, (char*)sendTimeout, sizeof(int)))
	{
		fprintf(stderr, "Set SO_SNDTIMEO error !\n");
	}

	//���ý��ճ�ʱ
	if (SOCKET_ERROR == setsockopt(clientSocketFd, SOL_SOCKET, SO_RCVTIMEO, (char*)recvTimeout, sizeof(int)))
	{
		fprintf(stderr, "Set SO_RCVTIMEO error !\n");
	}
	return 0;
}

int TFTPCLI::SendBufferToServer() {
	//��buffer�����ݷ��͵�Զ�̷�����
	int bytesSent = sendto(clientSocketFd, SendBuffer, SendBufLen, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	int err = WSAGetLastError();
	if (bytesSent < 0) {
		cout << "Error ocured while sending packet,code=" << err << endl;
		sprintf(MsgBuf, "Error ocured while sending packet,code=%d", err);
		LogFatal(MsgBuf);
		return -1;
	}
	return 0;
}

int TFTPCLI::CloseSocket() {
	//�ر�socket
	closesocket(clientSocketFd);
	return 0;
}

/// <summary>
/// ��server��ȡ����
/// </summary>
/// <returns>�������գ�0�����յ�RST��1����������-1</returns>
int TFTPCLI::RecvFromServer() {
	//��server��ȡ���ݣ�������-1�����򷵻�0
	memset(RecvBuffer, 0, sizeof(RecvBuffer));//���buffer
	int addrlen = sizeof(recvAddr);
	int bytesrcv = recvfrom(clientSocketFd, RecvBuffer, sizeof(RecvBuffer), 0, (struct sockaddr*)&recvAddr, &addrlen);
	int err = WSAGetLastError();
	if (bytesrcv < 0) {
		if (err == 10054) {
			cout << "Got RST from server" << endl;
			sprintf(MsgBuf,"Got RST from server\n");
			LogFatal(MsgBuf);
			return 1;
		}
		cout << "Error ocured while receving packet/server timed out,code=" << err << endl;
		sprintf(MsgBuf, "Error  / server timed out, code = %d" ,err );
		LogWarn(MsgBuf);

		return -1;
	}
	RecvBufLen = bytesrcv;//��ȡ���յ��ı��ĳ���
	
	return 0;
}

/// <summary>
/// ʹ��Octetģʽ��Զ�̷�������ȡ�ļ�
/// </summary>
/// <param name="host">Զ�̷�������ַ</param>
/// <param name="filename">Ҫ��ȡ���ļ���</param>
/// <param name="port">Ŀ��˿�</param>
/// <returns>0:success !0:fail</returns>
int TFTPCLI::GetFileFromRemote(char* host, char* filename, u_short port,int mode) {
	CreateSocket();//����socket
	int timeout = 1000;//����socket��ʱʱ��
	SetSocketTimeout(&timeout,&timeout );
	SetServerAddr(host, port);
	SetRequestBuffer(READ_REQUEST, mode, filename);
	SendBufferToServer();
	//��ѭ��
	int blocknum;
	int errorcode, prevBlocknum;
	prevBlocknum = 0;
	for (;;) {
		//��ʱ�ش�����
		int retries = 0;
		while (retries < MAX_RETRIES) {
			int state = RecvFromServer();//��server��ȡ����
			if (state < 0) {
				cout << "Failed to receive packet,retrying " << retries << endl;
				sprintf(MsgBuf,"Failed to receive packet,retrying %d\n", retries);
				LogWarn(MsgBuf);
				SendBufferToServer();//�������ش�
				retries++;
			}
			else if (state == 1) {
				//������Ѿ�����������
				return -1;
			}else {
				break;
			}
		}
		if (retries == MAX_RETRIES) {
			cout << "Failed after " << MAX_RETRIES << " retries, exiting" << endl;
			sprintf(MsgBuf,"Failed after %d retries,exiting",MAX_RETRIES);
			LogFatal(MsgBuf);
			return -1;
		}

		//�ɹ����յ����ݰ���ִ��
		int op = RecvBuffer[1];

		switch (op) {
		case DATA:
			//ͨ������յ��ı��ĳ��ȼ�鴫���Ƿ����

			//���ñ���
			blocknum = (u_short(RecvBuffer[3]) % 256) + (u_short(RecvBuffer[2]) % 256) * 256;
			//cout <<"======" << int(RecvBuffer[2])<<endl;
			//cout << "Got Block " << blocknum << endl;
			sprintf(MsgBuf, "Got Block %d", blocknum+((prevBlocknum+1)/65535)*65535);
			LogInfo(MsgBuf);
			serverAddr.sin_port = recvAddr.sin_port;//����Ŀ�Ķ˿�ΪS-TID
			SetAckBuffer(blocknum);//����ACK��blocknum
			//д���ļ�
			if (prevBlocknum%65535 == (blocknum+65534)%65535) {
				//cout << "Wrote Block " << blocknum << endl;
				sprintf(MsgBuf,"Wrote Block %d", blocknum + ((prevBlocknum+1) / 65535)*65535);
				LogInfo(MsgBuf);
				//�����ٶȲ����ü�ʱ��
				CalcSpeed();
				SetCurrentTime();
				//����ٶ�
				if (!(blocknum % 1000) || GetSpeed() < SPEED_THRESHHOLD) {
					//��������ٶȵ�Ƶ��
					system("cls");
					printf("Current Speed:%lf Bytes/s\n", GetSpeed());
				}
				/*cout << "Current Speed:"
					<< GetSpeed()
					<< " Bytes/s"
					<< endl;*/
				RRQFileS.write(RecvBuffer + 4, RecvBufLen - 4);
				prevBlocknum = blocknum;

			}
			if (RecvBufLen < DataPakSize) {
				cout << "Finished Receving Packet!"<<endl;
				sprintf(MsgBuf, "Finished Receving Packet!");
				LogInfo(MsgBuf);
				SendBufferToServer();//����ACK
				RRQFileS.close();//�ر��ļ�
				CloseSocket();//�ر�����
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
	int timeout = 1000;//����socket��ʱʱ��
	SetSocketTimeout(&timeout, &timeout);

	SetServerAddr(host, port);
	SetRequestBuffer(WRITE_REQUEST, mode, filename);
	SendBufferToServer();
	bool firstResp = false;//����Ƿ�Ϊ��һ�λ�Ӧ���ǵĻ����TID

	int dataPacketBlock = 0,pakStatus=0;//pakStatus��ǵ�ǰ���ݰ��Ƿ�Ϊ���һ��
	for (;;) {
		//��ʱ�ش�����
		int retries = 0;
		while (retries < MAX_RETRIES) {
			int state = RecvFromServer();//��server��ȡ����
			if (state < 0) {
				cout << "Failed to receive packet,retrying " << retries << endl;
				sprintf(MsgBuf, "Failed to receive packet,retrying %d\n", retries);
				LogWarn(MsgBuf);
				SendBufferToServer();//�������ش�
				retries++;
			}
			else if (state == 1) {
				//������Ѿ�����������
				return -1;
			}else {
				break;
			}
		}
		if (retries == MAX_RETRIES) {
			cout << "Failed after " << MAX_RETRIES << " retries, exiting" << endl;
			sprintf(MsgBuf, "Failed after %d retries,exiting", MAX_RETRIES);
			LogFatal(MsgBuf);
			return -1;
		}

		int blocknum;
		int op = RecvBuffer[1];
		switch (op) {
		case ACK:
			blocknum = (u_short(RecvBuffer[3]) % 256) + (u_short(RecvBuffer[2]) % 256) * 256;
			//TODO �޸����ĳ���128������� Done
			//cout << "Got ACK for block" << blocknum << endl;
			sprintf(MsgBuf, "Got ACK for block %d", blocknum + (dataPacketBlock / 65535) * 65535);
			LogInfo(MsgBuf);
			serverAddr.sin_port = recvAddr.sin_port;//����Ŀ�Ķ˿�ΪS-TID
			if (blocknum% 65535 == dataPacketBlock% 65535) {
				//�յ�����һ������ACK���������
				dataPacketBlock++;
				pakStatus = SetDataBuffer(dataPacketBlock);
				//�����ٶȲ����ü�ʱ��
				CalcSpeed();
				SetCurrentTime();
				//����ٶ�
				if (!(blocknum % 1000)||GetSpeed()<SPEED_THRESHHOLD) {
					//��������ٶȵ�Ƶ��
					system("cls");
					printf("Current Speed:%lf Bytes/s\n", GetSpeed());
				}
				
/*				cout << "Current Speed:"
					<< GetSpeed()
					<< " Bytes/s"
					<< endl;*/
			}
			else {
				//�յ�����Ч��ACK��ֱ������
				continue;
			}
			break;
		case TFTP_ERROR:
			cout << "Server reported Error!" 
				<<"code="
				<<int(RecvBuffer[3])
				<< endl;
			sprintf(MsgBuf, "Server reported error,code=%d", int(RecvBuffer[3]));
			LogFatal(MsgBuf);
			return -1;
			break;
		}

		if (pakStatus == 1) {
			//���ݰ��������
			cout << "Finished Uploading Data" << endl;
			sprintf(MsgBuf, "Finished Uploading Data", retries);
			LogInfo(MsgBuf);
			//TODO �ȴ�����ACK�������ش�
			CloseSocket();
			WRQFileS.close();
			return 0;
		}
		//Sleep(500);
		SendBufferToServer();
	}
}

/// <summary>
/// ���õ�ǰ�Ĵ����ٶ�,��λΪ�ֽ�
/// </summary>
/// <returns>�ɹ�����0</returns>
int TFTPCLI::CalcSpeed() {
		auto end = chrono::steady_clock::now();
		auto last = chrono::duration_cast<chrono::microseconds>(end - LastPacketSentTime);
		TransmitSpeed = double(DataPakSize - 4) / double(last.count() / 1000000.00);
		return 0;
}

/// <summary>
/// ���ü�ʱ��ʼ��
/// </summary>
/// <returns></returns>
int TFTPCLI::SetCurrentTime() {
	// ��ȡ΢�뼶ʱ���
	LastPacketSentTime = chrono::steady_clock::now();
	return 0;
}

double TFTPCLI::GetSpeed() {
	return TransmitSpeed;
}


int TFTPCLI::InitLog(char filename[]) {
	LogFileS.open(filename, ios::out);//���ļ�
	return 0;
}

int TFTPCLI::LogInfo(char* msg) {
	time(&LogTimeObj);
	char* time = ctime(&LogTimeObj);
	time[strlen(time) - 1] = 0;
	LogFileS << "[Info]"
		<< "["
		<< time
		<< "]"
		<< msg
		<< endl;
	return 0;
}

int TFTPCLI::LogWarn(char* msg) {
	time(&LogTimeObj);
	char* time = ctime(&LogTimeObj);
	time[strlen(time) - 1] = 0;
	LogFileS << "[Warn]"
		<<"["
		<< time
		<<"]"
		<< msg
		<< endl;
	return 0;
}

int TFTPCLI::LogFatal(char* msg) {
	time(&LogTimeObj);
	char* time = ctime(&LogTimeObj);
	time[strlen(time) - 1] = 0;
	LogFileS << "[Fatal]"
		<< "["
		<< time
		<< "]"
		<< msg
		<< endl;
	return 0;
}

int TFTPCLI::SetRequestBuffer(int op, int type, char* filename) {
	//���ɱ��ģ����浽buffer��
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = op;//����opcode
	if (op == READ_REQUEST) {
		cout << "Downloading data:"
			<< filename
			<< endl;
		//���ļ���ֱ��WRQ��ɺ�ر�
		switch (type) {
		case MODE_OCTET:
			RRQFileS.open(filename, ios::out | ios::binary);
			break;
		case MODE_NETASCII:
			RRQFileS.open(filename, ios::out);
			break;
		}
	}
	else if (op == WRITE_REQUEST) {
		cout << "Uploading data:"
			<< filename
			<< endl;
		//���ļ���ֱ��RRQ��ɺ�ر�
		switch (type) {
		case MODE_OCTET:
			WRQFileS.open(filename, ios::in | ios::binary);
			break;
		case MODE_NETASCII:
			WRQFileS.open(filename, ios::in);
			break;
		}

	}
	else {
		cout << "Not a valid opcode!" << endl;
		return -1;
	}
	strcpy_s(SendBuffer + OP_LEN, 500, filename);

	int fLen = strlen(filename) + 1;//���ļ��������ȣ�������������
	//����type
	if (type == MODE_OCTET) {
		strcpy_s(SendBuffer + OP_LEN + fLen, BUFFER_SIZE, "octet");
		SendBufLen = 8 + fLen;
		return 0;
	}
	else {
		strcpy_s(SendBuffer + OP_LEN + fLen, BUFFER_SIZE, "netascii");
		SendBufLen = 11 + fLen;
		return 0;
	}
	cout << "Error when generating Request!" << endl;
	return -1;

}

int TFTPCLI::SetAckBuffer(int blocknum) {
	//����Ack����
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = ACK;
	SendBuffer[3] = blocknum % 256;
	SendBuffer[2] = blocknum / 256;
	SendBufLen = 10;
	return 0;
}

int TFTPCLI::SetErrorBuffer(int errcode, char* errmsg) {
	//����Error����
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = TFTP_ERROR;
	SendBuffer[3] = errcode;
	strcpy(SendBuffer + 4, errmsg);
	SendBufLen = 10 + strlen(errmsg);
	return 0;
}

/// <summary>
/// 
/// </summary>
/// <param name="blocknum"></param>
/// <returns>�������ݰ���0����ֹ���ݰ���1</returns>
int TFTPCLI::SetDataBuffer(int blocknum) {
	memset(SendBuffer, 0, sizeof(SendBuffer));
	//����opcode
	SendBuffer[1] = DATA;
	//����blocknum
	SendBuffer[3] = blocknum;
	SendBuffer[2] = blocknum / 256;
	//��ȡ���ݲ�����packet����,����һ����СΪDataPakSize�����ݰ�
	WRQFileS.read(SendBuffer + 4, DataPakSize - 4);
	int count = WRQFileS.gcount();

	SendBufLen = 4 + count;
	if (count == 0) {
		return 1;
	}
	return 0;
}