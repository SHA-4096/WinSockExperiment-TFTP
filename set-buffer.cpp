#include"tftp.h"


int TFTPCLI::SetRequestBuffer(int op, int type, char* filename) {
	//���ɱ��ģ����浽buffer��
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = op;//����opcode
	if (op == READ_REQUEST) {
		cout << "Downloading data:"
			<< filename
			<< endl;
	}
	else if (op == WRITE_REQUEST) {
		cout << "Uploading data:"
			<< filename
			<< endl;
		//���ļ���ֱ��WRQ��ɺ�ر�
		WRQFileS.open(filename, ios::in | ios::binary);
	}
	else {
		cout << "Not a valid opcode!" << endl;
		return -1;
	}
	//op=RRQ��WRQ��ʱ��ִ��
	strcpy_s(SendBuffer + OP_LEN, 500, filename);

	int fLen = strlen(filename) + 1;//���ļ��������ȣ�������������
	//����type
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
	//����Ack����
	memset(SendBuffer, 0, sizeof(SendBuffer));
	SendBuffer[1] = ACK;
	SendBuffer[3] = blocknum;
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
	//����opcode
	SendBuffer[1] = DATA;
	//����blocknum
	SendBuffer[3] = blocknum;
	//��ȡ���ݲ�����packet����,����һ����СΪDataPakSize�����ݰ�
	WRQFileS.read(SendBuffer + 4, DataPakSize - 4);
	int count = WRQFileS.gcount();
	
	SendBufLen = 4 + count;
	if (count < DataPakSize - 4) {
		return 1;
	}
	return 0;
}