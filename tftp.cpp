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

int TFTPCLI::SendBufferToServer() {
	//��buffer�����ݷ��͵�Զ�̷�����
	int stat = sendto(clientSocketFd, SendBuffer, SendBufLen, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	int err = WSAGetLastError();
	if (stat < 0) {
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
	memset(SendBuffer, 0, sizeof(SendBuffer));//���buffer
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
	RecvBufLen = stat;//��ȡ���յ��ı��ĳ���
	return 0;
}

/// <summary>
/// ʹ��Octetģʽ��Զ�̷�������ȡ�ļ�
/// </summary>
/// <param name="host">Զ�̷�������ַ</param>
/// <param name="filename">Ҫ��ȡ���ļ���</param>
/// <param name="port">Ŀ��˿�</param>
/// <returns>0:success !0:fail</returns>
int TFTPCLI::GetFileFromRemote(char* host, char* filename, u_short port) {
	CreateSocket();
	SetServerAddr(host, port);
	SetRequestBuffer(READ_REQUEST, MODE_OCTET, filename);
	SendBufferToServer();
	//�������ļ�
	RRQFileS.open(filename, ios::out | ios::binary);
	RRQFileS.close();
	//��ѭ��
	int blocknum, errorcode, prevBlocknum;
	prevBlocknum = 0;
	for (;;) {
		RecvFromServer();//��server��ȡ����
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
				RRQFileS.open(filename, ios::out | ios::app | ios::binary);
				RRQFileS.write(RecvBuffer + 4, RecvBufLen - 4);
				RRQFileS.close();//TODO ����֮����ԼӸ��ļ���ɶ��
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

		Sleep(100);
		SendBufferToServer();
	}
	CloseSocket();
}

int TFTPCLI::PutFileToRemote(char* host, char* filename, u_short port) {
	CreateSocket();
	SetServerAddr(host, port);
	SetRequestBuffer(WRITE_REQUEST, MODE_OCTET, filename);
	SendBufferToServer();
	int dataPacketBlock = 0,pakStatus=0;//pakStatus��ǵ�ǰ���ݰ��Ƿ�Ϊ���һ��
	for (;;) {
		int stat = RecvFromServer();//��server��ȡ����
		if(stat == 1 || pakStatus == 1){
			//���յ���RST��Ϣ�������ݰ��������
			cout << "Finished Uploading Data" << endl;
			//TODO �ȴ�����ACK�������ش�
			CloseSocket();
			WRQFileS.close();
			return 0;
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

			}
			else {
				//�յ�����Ч��ACK��ֱ������
				continue;
			}
			break;
		case TFTP_ERROR:
			cout << "Server reported Error!" << endl;
			return -1;
			break;
		}
		//Sleep(500);
		SendBufferToServer();
	}
}
