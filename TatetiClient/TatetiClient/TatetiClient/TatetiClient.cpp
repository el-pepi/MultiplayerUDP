// TatetiClient.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include<winsock2.h>
#include<iostream>
#include <stdlib.h>
using namespace std;

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 512  //Max length of buffer

char ip[15];
int port = 0;

SOCKET s;
int slen;

string name;

enum MessageType
{
	Connect,
	Fail,
	Success,
	Name,
	Move,
	Update
};

enum GameStates {
	Connecting,
	Waiting,
	MyTurn,
	NotMyTurn
};

GameStates gameState;

struct Message {
public:
	MessageType type;
	char msg[256];
};

void SendMsg(sockaddr_in adr, Message msg) {

	char m[BUFLEN];

	memcpy(m, &msg, sizeof(Message));

	Message message;
	memcpy(&message,m,sizeof(Message));


	if (sendto(s, m, sizeof(Message), 0, (struct sockaddr*) &adr, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
}

void DrawTable() {
	
	system("cls");
	printf("%s%s\n","You are: ",name.c_str());
}

void GetData(char buf[BUFLEN]) {
	Message* m = reinterpret_cast<Message*>(buf);
	switch (gameState)
	{
	case GameStates::Connecting:
		if (m->type == MessageType::Success) {
			name = m->msg;
			system("cls");
			printf("%s%s\n%s\n", "You are: ", m->msg, "Waiting for other player to connect...");
			gameState = GameStates::Waiting;
		}
		else
		{
			printf("%s\n", m->msg);
			system("pause");
			exit(0);
		}
		break;
	case GameStates::Waiting:
		if (m->type == MessageType::Success) {
			DrawTable();
			printf("%s\n", m->msg);
		}
		break;
	}
}

int main()
{
	struct sockaddr_in si_other;
	slen = sizeof(si_other);
	char buf[BUFLEN];
	//char message[BUFLEN];
	WSADATA wsa;

	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("Enter server ip: ");
	cin.getline(ip,sizeof(ip));

	printf("Enter server port: ");
	cin >> port;


	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	si_other.sin_addr.S_un.S_addr = inet_addr(ip);

	Message msg;
	msg.type = MessageType::Connect;
	strcpy_s(msg.msg, "Connecting");
	SendMsg(si_other,msg);

	if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == SOCKET_ERROR)
	{
		printf("recvfrom() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	//DrawTable();
	GetData(buf);

	//start communication
	while (1)
	{
		if (gameState == GameStates::MyTurn) {
			Message m;
			m.type = MessageType::Move;
			printf("Enter position: ");
			cin.get();
			cin.getline(m.msg,sizeof(m.msg));
			SendMsg(si_other,m);
		}

		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);
		//try to receive some data, this is a blocking call
		if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		GetData(buf);
		//Message* message = reinterpret_cast<Message*>(buf);
		//printf("%s\n", message->msg);
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

