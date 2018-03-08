#include <stdlib.h>

#include "Sender.h"

#define CHANNEL_IP_ADDRESS_ARGUMENT_INDEX 1
#define CHANNEL_PORT_NUM_ARGUMENT_INDEX 2
#define INPUT_FILE_TO_TRANSFER_ARGUMENT_INDEX 3
#define SOCKET_PROTOCOL 0
#define BINDING_SUCCEEDED 0 // todo check

void InitSender(char *argv[]) {
	Sender.ChannelIPAddress = argv[CHANNEL_IP_ADDRESS_ARGUMENT_INDEX];
	Sender.ChannelPortNum = atoi(argv[CHANNEL_PORT_NUM_ARGUMENT_INDEX]);
	Sender.InputFileToTransfer = argv[INPUT_FILE_TO_TRANSFER_ARGUMENT_INDEX];
	Sender.ChannelSocketService.sin_family = AF_INET;
	Sender.ChannelSocketService.sin_addr.s_addr = inet_addr(Sender.ChannelIPAddress);
	Sender.ChannelSocketService.sin_port = htons(Sender.ChannelPortNum);

	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR) {
		printf("Error %ld at WSAStartup().\nExiting...\n", StartupRes);
		exit(ERROR_CODE);
	}

	Sender.ChannelSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Sender.ChannelSocket == INVALID_SOCKET) {
		printf("InitSender failed to create socket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

/*void BindToPort() { // todo
	int BindingReturnValue;
	Sender.ChannelSocketService.sin_family = AF_INET;
	Sender.ChannelSocketService.sin_addr.s_addr = inet_addr(Sender.ChannelIPAddress);
	Sender.ChannelSocketService.sin_port = htons(Sender.ChannelPortNum);
	BindingReturnValue = bind(Sender.ChannelSocket, (SOCKADDR*)&Sender.ChannelSocketService,
							  sizeof(Sender.ChannelSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		printf("BindToPort failed to bind.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}*/

void CloseSocketsAndWsaData() {
	int CloseSocketReturnValue;
	CloseSocketReturnValue = closesocket(Sender.ChannelSocket); // todo add if adding more sockets
	if (CloseSocketReturnValue == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData failed to close socket. Error Number is %d\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
	if (WSACleanup() == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
}