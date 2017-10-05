// TatetiClient.cpp: define el punto de entrada de la aplicación de consola.
//

#include "stdafx.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<stdio.h>
#include<winsock2.h>
#include<iostream>
using namespace std;

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 512  //Max length of buffer

char ip[15];
int port = 0;

SOCKET s;
int slen;

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

int main()
{
	struct sockaddr_in si_other;
	slen = sizeof(si_other);
	char buf[BUFLEN];
	char message[BUFLEN];
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
	cin >> ip;

	printf("Enter server port: ");
	cin >> port;


	//setup address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(port);
	si_other.sin_addr.S_un.S_addr = inet_addr(ip);

	//start communication
	while (1)
	{
		Message msg;
		msg.type = MessageType::Move;
		msg.msg = "Hola";

		SendMsg(si_other,msg);
		/*
		printf("Enter message : ");
		cin >> message;
		//send the message
		if (sendto(s, message, strlen(message), 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
		{
			printf("sendto() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		*/
		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);
		//try to receive some data, this is a blocking call
		if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		Message* message = reinterpret_cast<Message*>(buf);
		printf(message->msg);
		break;
	}

	closesocket(s);
	WSACleanup();

	return 0;
}

