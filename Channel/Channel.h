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

	SOCKET ReceiverSocket;
	SOCKADDR_IN ReceiverSocketService;

	SOCKET ChannelSocket;
	SOCKADDR_IN ChannelSocketService;
}ChannelProperties;

ChannelProperties Channel;

void InitChannel(char *argv[]);
void BindToPort();
void HandleTraffic();
void CloseSocketsAndWsaData();

#endif