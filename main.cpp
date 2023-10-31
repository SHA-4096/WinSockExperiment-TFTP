#include "tftp.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1



int main() {
	TFTPCLI* cli = new(TFTPCLI);
	cli->InitSocket();
	char host[20] = "127.0.0.1";
	char filename[20] = "test.txt";
//	char filename[20] = "test.txt";
	u_short port = 69;
	cli->GetFileFromRemote(host,filename,port,MODE_NETASCII);
//	cli->PutFileToRemote(host, filename, port,MODE_NETASCII);
	return  0;
}