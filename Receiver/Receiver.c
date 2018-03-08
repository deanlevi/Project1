#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "Receiver.h"

#define LOCAL_PORT_NUM_ARGUMENT_INDEX 1
#define OUTPUT_FILE_NAME_ARGUMENT_INDEX 2
#define SOCKET_PROTOCOL 0
#define BINDING_SUCCEEDED 0
#define BUFFER_LENGTH 2 // todo
#define SEND_RECEIVE_FLAGS 0

void InitReceiver(char *argv[]) {
	Receiver.LocalPortNum = atoi(argv[LOCAL_PORT_NUM_ARGUMENT_INDEX]);
	Receiver.OutputFileName = argv[OUTPUT_FILE_NAME_ARGUMENT_INDEX];
	
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR) {
		printf("Error %ld at WSAStartup().\nExiting...\n", StartupRes);
		exit(ERROR_CODE);
	}

	Receiver.ListeningSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Receiver.ListeningSocket == INVALID_SOCKET) {
		printf("InitReceiver failed to create socket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void BindToPort() {
	int BindingReturnValue;
	Receiver.ListeningSocketService.sin_family = AF_INET;
	Receiver.ListeningSocketService.sin_addr.s_addr = inet_addr(INADDR_ANY);
	Receiver.ListeningSocketService.sin_port = htons(Receiver.LocalPortNum);
	BindingReturnValue = bind(Receiver.ListeningSocket, (SOCKADDR*)&Receiver.ListeningSocketService,
							  sizeof(Receiver.ListeningSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void HandleConnectionWithChannel() {
	char ReceivedBuffer[BUFFER_LENGTH];
	int ReceivedBufferLength = recvfrom(Receiver.ListeningSocket, &ReceivedBuffer, BUFFER_LENGTH, SEND_RECEIVE_FLAGS,
								   	   &Receiver.ChannelSocketService, NULL);
	SavePortAndIPOfChannel();
	CreateOutputFile();
	while (TRUE) {
		WriteInputToOutputFile(ReceivedBuffer, ReceivedBufferLength); // todo
		ProcessInput(ReceivedBuffer, ReceivedBufferLength); // todo
		if (CheckForEnd(ReceivedBuffer, ReceivedBufferLength)) { // todo
			break;
		}
		ReceivedBufferLength = recvfrom(Receiver.ListeningSocket, &ReceivedBuffer, BUFFER_LENGTH, SEND_RECEIVE_FLAGS, NULL, NULL);
	}
	SendInformationToChannel(); // todo
	//printf(); // todo
}

void SavePortAndIPOfChannel() {
	Receiver.ChannelPortNum = (int)ntohs(Receiver.ChannelSocketService.sin_port);
	Receiver.ChannelIPAddress = inet_ntoa(Receiver.ChannelSocketService.sin_addr);
}

void CreateOutputFile() {
	FILE *OutputFilePointer = fopen(Receiver.OutputFileName, "w");
	if (OutputFilePointer == NULL) {
		printf("CreateOutputFile couldn't open output file.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	fclose(OutputFilePointer);
}

void CloseSocketsAndWsaData() { // todo check
	int CloseSocketReturnValue;
	CloseSocketReturnValue = closesocket(Receiver.ListeningSocket); // todo add if adding more sockets
	if (CloseSocketReturnValue == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData failed to close socket. Error Number is %d\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
	if (WSACleanup() == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
}