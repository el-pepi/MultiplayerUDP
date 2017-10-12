// TatetiServer.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include<winsock2.h>
#include<iostream>
#include<sstream>
using namespace std;

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 512  //Max length of buffer

int port = 0;
SOCKET s;
int slen;

int values[9] = {0};
int turn = 0;

class Player {
public:
	sockaddr_in address;
};

enum MessageType
{
	Connect,
	Fail,
	Success,
	Name,
	Move,
	Update
};

struct Message {
public:
	MessageType type;
	char msg[256];
};

string GetTable() {
	string s = "";
	for (int i = 0; i < 9; i++)
	{
		if (values[i] == 0) {
			s += i;
		}if (values[i] == -1) {
			s += "X";
		}if (values[i] == 1) {
			s += "O";
		}if (i % 3 == 0) {
			s += "\n";
		}
	}
	return s;
}

void SendMsg(sockaddr_in adr, Message msg) {

	char m[BUFLEN];

	memcpy(m, &msg, sizeof(Message));

	Message message;
	memcpy(&message, m, sizeof(Message));


	if (sendto(s, m, sizeof(Message), 0, (struct sockaddr*) &adr, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		system("pause");
		exit(EXIT_FAILURE);
	}
}

int main()
{
	struct sockaddr_in server, si_other;
	int recv_len;
	char buf[BUFLEN];
	WSADATA wsa;

	slen = sizeof(si_other);

	printf("Enter port: ");
	cin >> port;


	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		system("pause");
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
		system("pause");
		exit(EXIT_FAILURE);
	}
	printf("Socket created.\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		system("pause");
		exit(EXIT_FAILURE);
	}
	puts("Bind done");



	int playerTurn = 0;
	int connectedPlayers = 0;
	Player* players[2];


	//keep listening for data
	while (1)
	{
		printf("Waiting for data...");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			system("pause");
			exit(EXIT_FAILURE);
		}


		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

		Message message;

		memcpy(&message, buf, recv_len);

		printf("Data: %s\n", message.msg);

		Message m;
		switch (message.type)
		{
		case MessageType::Connect:
			if (connectedPlayers < 2) {
				Player* p = new Player();
				p->address = si_other;
				players[connectedPlayers] = p;
				m.type = MessageType::Success;
				if (connectedPlayers == 0) {
					strcpy_s(m.msg, "X");
				}
				else
				{
					strcpy_s(m.msg, "O");
				}
				SendMsg(p->address,m);

				connectedPlayers++;
				if (connectedPlayers == 2) {
					strcpy_s(m.msg, "Game start!");
					for (int i = 0; i < 2; i++)
					{
						SendMsg(players[i]->address, m);
					}
				}
			}
			else
			{
				m.type = MessageType::Fail;
				strcpy_s(m.msg, "ERROR: Server lleno");
				SendMsg(si_other, m);
			}
			break;
		case MessageType::Move:
			int mov = (int)(message.msg[0] - 48);
			if (message.msg[1] == '\0' && mov > 0 && mov < 10) {
				SendMsg(si_other, m);
				if (values[mov] == 0) {
					if (turn == 0) {
						turn = 1;
						values[mov] = -1;
					}
					else {
						turn = 0;
						values[mov] = 1;
					}
					m.type = MessageType::Update;
					strcpy_s(m.msg, GetTable().c_str());
					for (int i = 0; i < 2; i++)
					{
						SendMsg(players[i]->address, m);
					}
				}
				else {
					m.type = MessageType::Fail;
					strcpy_s(m.msg, "ERROR: Position taken");
					SendMsg(si_other, m);
				}
			}
			else
			{
				m.type = MessageType::Fail;
				strcpy_s(m.msg, "ERROR: Unvalid position");
				SendMsg(si_other, m);
			}
			break;
		default:
			m.type = MessageType::Fail;
			strcpy_s(m.msg, "ERROR: Comando desconocido");
			SendMsg(si_other, m);
			break;
		}
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

