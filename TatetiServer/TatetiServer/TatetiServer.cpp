// TatetiServer.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include<winsock2.h>
#include<iostream>
using namespace std;

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 512  //Max length of buffer

int port = 0;
SOCKET s;
int slen;

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
	Move
};

struct Message {
public:
	MessageType type;
	char* msg;
};

void SendMsg(sockaddr_in adr,Message* msg) {

	char m[BUFLEN];
	memset(m, '\0', BUFLEN);
	memcpy(m, &msg, sizeof(msg));
	//memcpy(m + sizeof(msg->type), &msg->msg, sizeof(msg->msg));
	//memcpy(m, &msg->type, sizeof(msg->type));

	Message* message = reinterpret_cast<Message*>(m);
	if (sendto(s, m, sizeof(m), 0, (struct sockaddr*) &adr, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
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
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
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
			exit(EXIT_FAILURE);
		}


		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);


		Message message;

		memcpy(&message, buf, recv_len);
		//Message* message = reinterpret_cast<Message*>(buf);

		Message* m = new Message();
		switch (message.type)
		{
		case MessageType::Connect:
			if (connectedPlayers < 2) {
				Player* p = new Player();
				p->address = si_other;
				players[connectedPlayers] = p;
				connectedPlayers++;
				m->type = MessageType::Success;
				m->msg = "Conectado exitosamente";
				SendMsg(si_other,m);
			}
			else
			{
				m->type = MessageType::Fail;
				m->msg = "ERROR: Server lleno";
				SendMsg(si_other, m);
			}
			break;
		default:
			m->type = MessageType::Fail;
			m->msg = "ERROR: Comando desconocido";
			SendMsg(si_other, m);
			break;
		}

		strlen(buf);

		//now reply the client with the same data
		if (sendto(s, buf, recv_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

