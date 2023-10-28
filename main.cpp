#include "tftp.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1



int main() {
	TFTPCLI* cli = new(TFTPCLI);
	cli->InitSocket();
	char host[20] = "10.11.69.30";
	char filename[20] = "EUPL-EN.pdf";
	u_short port = 69;
	cli->GetFileFromRemote(host,filename,port);
	return  0;
}