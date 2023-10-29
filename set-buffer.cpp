#include"tftp.h"


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
		//打开文件，直到WRQ完成后关闭
		WRQFileS.open(filename, ios::in | ios::binary);
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

/// <summary>
/// 
/// </summary>
/// <param name="blocknum"></param>
/// <returns>正常数据包：0；终止数据包：1</returns>
int TFTPCLI::SetDataBuffer(int blocknum) {
	//设置opcode
	SendBuffer[1] = DATA;
	//设置blocknum
	SendBuffer[3] = blocknum;
	//读取数据并塞到packet里面,创建一个大小为DataPakSize的数据包
	WRQFileS.read(SendBuffer + 4, DataPakSize - 4);
	int count = WRQFileS.gcount();
	
	SendBufLen = 4 + count;
	if (count < DataPakSize - 4) {
		return 1;
	}
	return 0;
}