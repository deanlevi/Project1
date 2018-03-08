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
#define USER_INPUT_LENGTH 10

void InitReceiver(char *argv[]) {
	Receiver.LocalPortNum = atoi(argv[LOCAL_PORT_NUM_ARGUMENT_INDEX]);
	Receiver.OutputFileName = argv[OUTPUT_FILE_NAME_ARGUMENT_INDEX];
	
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR) {
		printf("Error %ld at WSAStartup().\nExiting...\n", StartupRes);
		exit(ERROR_CODE);
	}

	Receiver.ReceiverSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Receiver.ReceiverSocket == INVALID_SOCKET) {
		printf("InitReceiver failed to create socket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void BindToPort() {
	int BindingReturnValue;
	Receiver.ReceiverSocketService.sin_family = AF_INET;
	Receiver.ReceiverSocketService.sin_addr.s_addr = inet_addr(INADDR_ANY);
	Receiver.ReceiverSocketService.sin_port = htons(Receiver.LocalPortNum);
	BindingReturnValue = bind(Receiver.ReceiverSocket, (SOCKADDR*)&Receiver.ReceiverSocketService,
							  sizeof(Receiver.ReceiverSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		printf("BindToPort failed to bind.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void HandleConnectionWithChannel() {
	bool GotEndFromUser = false;
	char ReceivedBuffer[BUFFER_LENGTH];
	int ReceivedBufferLength = recvfrom(Receiver.ReceiverSocket, &ReceivedBuffer, BUFFER_LENGTH, SEND_RECEIVE_FLAGS,
								   	   &Receiver.ChannelSocketService, NULL);
	SavePortAndIPOfChannel();
	CreateOutputFile();
	WriteInputToOutputFile(ReceivedBuffer, ReceivedBufferLength); // todo
	ProcessInput(ReceivedBuffer, ReceivedBufferLength); // todo

	fd_set Allfds;
	fd_set Readfds;
	int Status;
	FD_ZERO(&Allfds);
	FD_SET(Receiver.ReceiverSocket, &Allfds);
	FD_SET(stdin, &Allfds);

	while (TRUE) {
		Readfds = Allfds;
		Status = select(0, &Readfds, NULL, NULL, NULL);
		if (Status == SOCKET_ERROR) {
			printf("HandleConnectionWithChannel select failure.\n");
			CloseSocketsAndWsaData();
			exit(ERROR_CODE);
		}
		else if (Status == 0) {
			continue;
		}
		else {
			if (FD_ISSET(Receiver.ReceiverSocket, &Readfds)) {
				HandleReceiveFromChannel(); // todo
			}
			if (FD_ISSET(stdin, &Readfds)) {
				HandleUserInput(&GotEndFromUser); // todo
				if (GotEndFromUser) {
					break;
				}
			}
		}

	}
	SendInformationToChannel(); // todo
	//printf(); // todo
}

void SavePortAndIPOfChannel() { // todo - remove
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

void HandleReceiveFromChannel() {
	char ReceivedBuffer[BUFFER_LENGTH];
	int ReceivedBufferLength = recvfrom(Receiver.ReceiverSocket, &ReceivedBuffer, BUFFER_LENGTH, SEND_RECEIVE_FLAGS, NULL, NULL);

	WriteInputToOutputFile(ReceivedBuffer, ReceivedBufferLength); // todo // todo check if write before/after correcting error
	ProcessInput(ReceivedBuffer, ReceivedBufferLength); // todo
}

void HandleUserInput(bool *GotEndFromUser) {
	char UserInput[USER_INPUT_LENGTH];
	scanf("%s", UserInput);
	if (strcmp(UserInput, "End") == 0) {
		*GotEndFromUser = true;
	}
	else {
		printf("Not a valid input. To finish enter 'End'.\n");
	}
}

void CloseSocketsAndWsaData() { // todo check
	int CloseSocketReturnValue;
	CloseSocketReturnValue = closesocket(Receiver.ReceiverSocket); // todo add if adding more sockets
	if (CloseSocketReturnValue == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData failed to close socket. Error Number is %d\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
	if (WSACleanup() == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
}