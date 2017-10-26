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

class Player {
public:
	sockaddr_in address;
};

int port = 0;
SOCKET s;
int slen;

int values[9] = {0};
int turn = 0;
int connectedPlayers = 0;
Player* players[2];


enum MessageType
{
	Connect,
	Fail,
	Success,
	Name,
	Move,
	Update,
	SetTurn,
	Win,
	Lose,
	Tie
};

struct Message {
public:
	MessageType type;
	char msg[256];
};

string GetTable() {
	string s = "";
	std::stringstream str;
	for (int i = 0; i < 9; i++)
	{
		if (values[i] == 0) {
			str << i+1;
			//s += i;
		}if (values[i] == -1) {
			str << "X";
			//s += "X";
		}if (values[i] == 1) {
			//s += "O";
			str << "O";
		}if ((i + 1) % 3 == 0) {
			//s += "\n";
			str << "\n";
		}
		else {
			str << "|";
		}
	}
	printf(str.str().c_str());
	return str.str();
}

void SendMsg(sockaddr_in adr,string msg, MessageType type) {

	char m[BUFLEN];

	Message message;
	message.type = type;
	strcpy_s(message.msg, msg.c_str());

	memcpy(m, &message, sizeof(Message));

	if (sendto(s, m, sizeof(Message), 0, (struct sockaddr*) &adr, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		system("pause");
		exit(EXIT_FAILURE);
	}
}

void EndGame(int winner) {
	SendMsg(players[winner]->address, "You win!", MessageType::Win);
	SendMsg(players[turn]->address, "You lose...", MessageType::Lose);

	for (int i = 0; i < 9; i++)
	{
		values[i] = 0;
	}

	connectedPlayers = 0;
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

		switch (message.type)
		{
		case MessageType::Connect:
			if (connectedPlayers < 2) {
				Player* p = new Player();
				p->address = si_other;
				players[connectedPlayers] = p;
				if (connectedPlayers == 0) {
					SendMsg(p->address, "X",MessageType::Success);
				}
				else
				{
					SendMsg(p->address, "O", MessageType::Success);
				}

				connectedPlayers++;
				if (connectedPlayers == 2) {
					for (int i = 0; i < 2; i++)
					{
						SendMsg(players[i]->address, "Game start!", MessageType::Success);
						SendMsg(players[i]->address, GetTable(), MessageType::Update);
					}
					SendMsg(players[turn]->address, "Your turn!", MessageType::SetTurn);
				}
			}
			else
			{
				SendMsg(si_other, "ERROR: Server lleno",MessageType::Fail);
			}
			break;
		case MessageType::Move:
			int mov = (int)(message.msg[0] - 48) - 1;
			bool finished = false;
			if (message.msg[1] == '\0' && mov >= 0 && mov <= 9) {
				if (values[mov] == 0) {
					int oldTurn = turn;
					if (turn == 0) {
						turn = 1;
						values[mov] = -1;
					}
					else {
						turn = 0;
						values[mov] = 1;
					}
					for (int i = 0; i < 2; i++)
					{
						SendMsg(players[i]->address, GetTable(), MessageType::Update);
					}


					for (int i = 0; i < 3; i++)
					{
						if (values[i] == 0) {
							continue;
						}
						if (values[i] == values[i + 3] && values[i] == values[i + 6]) {
							finished = true;
							EndGame(oldTurn);
							break;
						}
					}
					if (finished) {
						continue;
					}
					for (int i = 0; i < 3; i++)
					{
						if (values[i*3] == 0) {
							continue;
						}
						if (values[i*3] == values[i*3 + 1] && values[i*3] == values[i*3 + 2]) {
							finished = true;
							EndGame(oldTurn);
							break;
						}
					}
					if (finished) {
						continue;
					}

					if (values[0] != 0 && values[0] == values[4] && values[0] == values[8]) {
						finished = true;
						EndGame(oldTurn);
					}
					if (finished) {
						continue;
					}
					if (values[2] != 0 && values[2] == values[4] && values[2] == values[6]) {
						finished = true;
						EndGame(oldTurn);
					}
					if (finished) {
						continue;
					}
					bool tied = true;
					for (int i = 0; i < 9; i++)
					{
						if (values[i] == 0) {
							tied = false;
							break;
						}
					}
					if (tied) {
						finished = true;
						for (int i = 0; i < 2; i++)
						{
							SendMsg(players[i]->address, "Game tied.", MessageType::Tie);
						}

						for (int i = 0; i < 9; i++)
						{
							values[i] = 0;
						}

						connectedPlayers = 0;
					}
					if (finished == false) {
						SendMsg(players[turn]->address, "Your turn!", MessageType::SetTurn);
					}
				}
				else {
					SendMsg(si_other, "ERROR: Position taken", MessageType::Fail);
				}
			}
			else
			{
				SendMsg(si_other, "ERROR: Unvalid position" , MessageType::Fail);
			}
			break;
		}
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

