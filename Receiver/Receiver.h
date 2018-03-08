#ifndef RECEIVER_H
#define RECEIVER_H

#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define ERROR_CODE (int)(-1)

typedef struct _ReceiverProperties {
	int LocalPortNum;
	char *OutputFileName;

	SOCKET ReceiverSocket;
	SOCKADDR_IN ReceiverSocketService;

	SOCKADDR_IN ChannelSocketService;
	int ChannelPortNum;
	char *ChannelIPAddress;
}ReceiverProperties;

ReceiverProperties Receiver;

void InitReceiver(char *argv[]);
void BindToPort();
void HandleConnectionWithChannel();
void CloseSocketsAndWsaData();

#endif