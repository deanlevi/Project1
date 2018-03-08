#ifndef CHANNEL_H
#define CHANNEL_H

#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define ERROR_CODE (int)(-1)

typedef struct _ChannelProperties {
	int LocalPortNum;
	char *ReceiverIPAddress;
	int ReceiverPortNum;
	int ErrorProbability; // todo verify int
	int RandomSeed; // todo verify int

	SOCKET ListeningToReceiverSocket;
	SOCKADDR_IN ListeningToReceiverSocketService;

	SOCKET ListeningToSenderSocket;
	SOCKADDR_IN ListeningToSenderSocketService;

	//SOCKADDR_IN ChannelSocketService;
	//int ChannelPortNum;
	//char *ChannelIPAddress;
}ChannelProperties;

ChannelProperties Channel;

void InitChannel(char *argv[]);
void BindToPort();
void HandleTraffic();
//void HandleConnectionWithChannel();
//void CloseSocketsAndWsaData();

#endif