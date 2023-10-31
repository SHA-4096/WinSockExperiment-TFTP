#include "tftp.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1



int main() {
	TFTPCLI* cli = new(TFTPCLI);
	cli->InitSocket();
	char logFileName[] = "log.log";
	cli->InitLog(logFileName);
	char host[20] = "127.0.0.1";
	char filename[20] = "EUPL-EN-111.pdf";
//	char filename[20] = "test.txt";
	u_short port = 69;
	cli->GetFileFromRemote(host,filename,port,MODE_OCTET);
//	cli->PutFileToRemote(host, filename, port,MODE_OCTET);
	return  0;
}