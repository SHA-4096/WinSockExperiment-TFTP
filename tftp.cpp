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
			return 1;
		}
		cout << "Error ocured while receving packet/server timed out,code=" << err << endl;
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
	int blocknum, errorcode, prevBlocknum;
	prevBlocknum = 0;
	for (;;) {
		//��ʱ�ش�����
		int retries = 0;
		while (retries < MAX_RETRIES) {
			int state = RecvFromServer();//��server��ȡ����
			if (state < 0) {
				cout << "Failed to receive packet,retrying " << retries << endl;
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
			return -1;
		}

		//�ɹ����յ����ݰ���ִ��
		int op = RecvBuffer[1];

		switch (op) {
		case DATA:
			//ͨ������յ��ı��ĳ��ȼ�鴫���Ƿ����

			//���ñ���
			blocknum = RecvBuffer[3];
			cout << "Got Block " << blocknum << endl;
			serverAddr.sin_port = recvAddr.sin_port;//����Ŀ�Ķ˿�ΪS-TID
			SetAckBuffer(blocknum);//����ACK��blocknum
			//д���ļ�
			if (prevBlocknum == blocknum - 1) {
				cout << "Wrote Block " << blocknum << endl;
				//�����ٶȲ����ü�ʱ��
				CalcSpeed();
				SetCurrentTime();
				//����ٶ�
				cout << "Current Speed:"
					<< GetSpeed()
					<< " Bytes/s"
					<< endl;
				RRQFileS.write(RecvBuffer + 4, RecvBufLen - 4);
				prevBlocknum = blocknum;

			}
			if (RecvBufLen < DataPakSize) {
				cout << "Finished Receving Packet!"<<endl;
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
			return -1;
		}

		
		int op = RecvBuffer[1];
		switch (op) {
		case ACK:
			cout << "Got ACK for block" << int(RecvBuffer[3])<<endl;
			serverAddr.sin_port = recvAddr.sin_port;//����Ŀ�Ķ˿�ΪS-TID
			if (RecvBuffer[3] == dataPacketBlock) {
				//�յ�����һ������ACK���������
				dataPacketBlock++;
				pakStatus = SetDataBuffer(dataPacketBlock);
				//�����ٶȲ����ü�ʱ��
				CalcSpeed();
				SetCurrentTime();
				//����ٶ�
				cout << "Current Speed:"
					<< GetSpeed()
					<< " Bytes/s"
					<< endl;
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
			return -1;
			break;
		}

		if (pakStatus == 1) {
			//���ݰ��������
			cout << "Finished Uploading Data" << endl;
			//TODO �ȴ�����ACK�������ش�
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
/// ���õ�ǰ�Ĵ����ٶ�,��λΪ�ֽ�
/// </summary>
/// <returns>�ɹ�����0</returns>
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
/// ���ü�ʱ��ʼ��
/// </summary>
/// <returns></returns>
int TFTPCLI::SetCurrentTime() {
	// ��ȡϵͳ����ʱ��ĺ��뼶ʱ���
	LastPacketSentTime = GetTickCount();
	return 0;
}

double TFTPCLI::GetSpeed() {
	return TransmitSpeed;
}