#include "tftp.h"
#include<thread>
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1

#define TASK_NUM 100
int taskCount;

struct MultiTasking {
	int type;
	char filename[100];
	char host[20];
	int mode;
	u_short port;
	TFTPCLI* cli;
}TSKS[TASK_NUM];


thread threads[TASK_NUM];

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
		"T : Set Multiple Tasks\n"
		"E : Execute Multiple Tasks\n"
		"tasks : View tasks\n"
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
		else if (input == "T") {
			if (!setflag) {
				cout << "Server not set,please set server first!" << endl;
				continue;
			}
			while (1) {
				cout << "Input task type\nW:WRQ\nR:RRQ" << endl;
				cin >> input;
				if (input == "W") {
					TSKS[taskCount].type = WRITE_REQUEST;
					break;
				}
				else if (input == "R") {
					TSKS[taskCount].type = READ_REQUEST;
					break;
				}
				else {
					cout << "Invalid Input!" << endl;
					continue;
				}

			}
			cout << "Input Filename:";
			cin >> filename;
			strcpy(TSKS[taskCount].filename, filename);
			TSKS[taskCount].cli = new(TFTPCLI);
			strcpy(TSKS[taskCount].host, host);
			TSKS[taskCount].port = port;
			TSKS[taskCount].mode = mode;
			taskCount++;
		}
		else if (input == "E") {
			for (int i = 0; i < taskCount; ++i) {
				if (TSKS[i].type == READ_REQUEST) {
					threads[i] = thread(&TFTPCLI::GetFileFromRemote, TSKS[i].cli, TSKS[i].host, TSKS[i].filename, TSKS[i].port, TSKS[i].mode);
					threads[i].join();
					cout << "Started Thread " << i << endl;
				}
				else {
					threads[i] = thread(&TFTPCLI::PutFileToRemote, TSKS[i].cli, TSKS[i].host, TSKS[i].filename, TSKS[i].port, TSKS[i].mode);
					threads[i].join();
					cout << "Started Thread " << i << endl;
				}
			}
			taskCount = 0;
		}
		else if (input == "tasks") {
			for (int i = 0; i < taskCount; ++i) {
				cout << "Task#" << i << endl
					<< "Type:"
					<< (TSKS[i].type == READ_REQUEST ? "RRQ" : "WRQ")
					<< endl
					<< "filename:"
					<< TSKS[i].filename
					<< endl
					<< "Server Address:"
					<< host
					<< endl
					<< "Server Port:"
					<< port
					<< endl;
			}
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
		}
		else{
			cout << "Invalid input!\n";

		}
		
	}
	return  0;
}