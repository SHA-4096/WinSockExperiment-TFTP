#include "tftp.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1



int main() {
	TFTPCLI* cli = new(TFTPCLI);
	cli->InitSocket();
	char host[20] = "10.16.201.32";
	char filename[20] = "EUPL-EN-111.pdf";
//	char filename[20] = "test.txt";
	u_short port = 69;
//	cli->GetFileFromRemote(host,filename,port);
	cli->PutFileToRemote(host, filename, port);
	return  0;
}