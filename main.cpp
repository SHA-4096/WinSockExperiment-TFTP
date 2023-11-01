#include "tftp.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1



int main() {
	/*
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
*/	
	char host[20];
	u_short port;
	bool setflag = false;
	int mode;
	string input;
	char filename[100];
	string welcome = "Welcome to TFTP client,type 'help' for user instruction.\n";
	string instruction = "TFTP Client Instruction\n"
		"To Access Function, input the following\n"
		"=================================================\n"
		"S : Set Server Infomation\n"
		"R : Get a file from a remote server\n"
		"W : Put file to a remote server\n"
		"state : See the configuration of current client\n"
		"help : Get help instructions\n"
		"exit : exit from the program\n"
		"=================================================\n";
	
	cout << welcome<<endl;

	TFTPCLI* cli = new(TFTPCLI);
	cli->InitSocket();
	char logFileName[] = "log.log";
	cli->InitLog(logFileName);
	while(1){
		cout << "(TFTP)>";
		cin >> input;
		if (input == "help") {
			cout << instruction << endl;
		}
		else if (input == "state") {
			if (!setflag) {
				cout << "Server is not set,please use 'S' to set the server\n";
				continue;
			}
			char msg[100];
			sprintf(msg,"Server address:%s\nServer port:%d\n", host, port);
			cout << msg;
			if (mode == MODE_NETASCII) {
				cout << "Transfer Mode:NETASCII\n";
			}
			else {
				cout << "Transfer Mode:OCTET\n";
			}
		}
		else if (input == "exit") {
			cout << "Bye~" << endl;
			break;
		}
		else if (input == "S") {
			cout << "Input Server Address:";
			cin >> host;
			cout << "Input Server port:";
			cin >> port;
			while (1) {
				cout << "Input Transfer mode\nO:OCTET\nA:NETASCII\n";
				cin >> input;
				if (input == "O") {
					mode = MODE_OCTET;
					break;
				}
				else if (input == "A") {
					mode = MODE_NETASCII;
					break;
				}
				else {
					cout << "Invalid input, please try again!\n" << endl;
				}
			}
			cout << "Set successful!"<<endl;
			setflag = true;
			
		}
		else if (input == "W") {
			if (!setflag) {
				cout << "Server not set,please set server first!" << endl;
				continue;
			}
			cout << "Input Filename:";
			cin >> filename;
			cli->PutFileToRemote(host, filename, port, mode);

		}
		else if (input == "R") {
			if (!setflag) {
				cout << "Server not set,please set server first!" << endl;
				continue;
			}
			cout << "Input Filename:";
			cin >> filename;
			cli->GetFileFromRemote(host, filename, port, mode);
		}else{
			cout << "Invalid input!\n";

		}
		
	}
	return  0;
}