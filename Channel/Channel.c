#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>

#include "Channel.h"

#define LOCAL_PORT_NUM_ARGUMENT_INDEX 1
#define RECEIVER_IP_ADDRESS_ARGUMENT_INDEX 2
#define RECEIVER_PORT_NUM_ARGUMENT_INDEX 3
#define ERROR_PROBABILITY_ARGUMENT_INDEX 4
#define RANDOM_SEED_ARGUMENT_INDEX 5
#define SOCKET_PROTOCOL 0
#define BINDING_SUCCEEDED 0
#define SEND_RECEIVE_FLAGS 0
#define CHUNK_SIZE 8
#define ERROR_CONST 65536 // = 2^16
#define MESSAGE_LENGTH 20
#define SEND_MESSAGES_WAIT 100

void InitChannel(char *argv[]);
void BindToPort();
void HandleTraffic();
void HandleReceiveFromSender(unsigned long long ReceivedBuffer, int ReceivedBufferLength);
void InsertErrors(unsigned long long *ReceivedBuffer);
void HandleReceiveFromReceiver(char *MessageFromReceiver, int ReceivedBufferLength);
void CloseSocketsAndWsaData();

void InitChannel(char *argv[]) {
	Channel.LocalPortNum = atoi(argv[LOCAL_PORT_NUM_ARGUMENT_INDEX]);
	Channel.ReceiverIPAddress = argv[RECEIVER_IP_ADDRESS_ARGUMENT_INDEX];
	Channel.ReceiverPortNum = atoi(argv[RECEIVER_PORT_NUM_ARGUMENT_INDEX]);
	Channel.ErrorProbability = atoi(argv[ERROR_PROBABILITY_ARGUMENT_INDEX]);
	Channel.RandomSeed = atoi(argv[RANDOM_SEED_ARGUMENT_INDEX]);
	Channel.ReceiverSocketService.sin_family = AF_INET;
	Channel.ReceiverSocketService.sin_addr.s_addr = inet_addr(Channel.ReceiverIPAddress);
	Channel.ReceiverSocketService.sin_port = htons(Channel.ReceiverPortNum);
	srand(Channel.RandomSeed);

	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR) {
		printf("Error %ld at WSAStartup().\nExiting...\n", StartupRes);
		exit(ERROR_CODE);
	}

	Channel.ReceiverSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Channel.ReceiverSocket == INVALID_SOCKET) {
		printf("InitChannel failed to create ReceiverSocket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	Channel.ChannelSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Channel.ChannelSocket == INVALID_SOCKET) {
		printf("InitChannel failed to create ChannelSocket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void BindToPort() { // todo check binding
	int BindingReturnValue;
	/*Channel.ReceiverSocketService.sin_family = AF_INET; // todo check
	Channel.ReceiverSocketService.sin_addr.s_addr = inet_addr(Channel.ReceiverIPAddress);
	Channel.ReceiverSocketService.sin_port = htons(Channel.ReceiverPortNum);
	BindingReturnValue = bind(Channel.ReceiverSocket, (SOCKADDR*)&Channel.ReceiverSocketService,
							  sizeof(Channel.ReceiverSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		printf("BindToPort failed to bind.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}*/
	Channel.ChannelSocketService.sin_family = AF_INET;
	Channel.ChannelSocketService.sin_addr.s_addr = INADDR_ANY;
	Channel.ChannelSocketService.sin_port = htons(Channel.LocalPortNum);
	BindingReturnValue = bind(Channel.ChannelSocket, (SOCKADDR*)&Channel.ChannelSocketService,
							  sizeof(Channel.ChannelSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		printf("BindToPort failed to bind.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void HandleTraffic() {
	//struct timeval Tv; // todo remove
	//Tv.tv_sec = 20; // todo remove
	fd_set Allfds;
	fd_set Readfds;
	int Status;
	FD_ZERO(&Allfds);
	FD_SET(Channel.ReceiverSocket, &Allfds);
	FD_SET(Channel.ChannelSocket, &Allfds);

	unsigned long long ReceivedBuffer;
	char MessageFromReceiver[MESSAGE_LENGTH];
	int FromLen = sizeof(Channel.SenderSocketService);
	int ReceivedBufferLength = recvfrom(Channel.ChannelSocket, &ReceivedBuffer, CHUNK_SIZE, SEND_RECEIVE_FLAGS,
								   	   (SOCKADDR*)&Channel.SenderSocketService, &FromLen);
	if (ReceivedBufferLength == SOCKET_ERROR) {
		printf("HandleTraffic failed to recvfrom. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	printf("HandleTraffic received from sender %llu\n", ReceivedBuffer); // todo remove
	HandleReceiveFromSender(ReceivedBuffer, ReceivedBufferLength); // todo

	while (TRUE) {
		Readfds = Allfds;
		Status = select(0, &Readfds, NULL, NULL, NULL);
		if (Status == SOCKET_ERROR) {
			printf("HandleTraffic select failure. Error Number is %d\n", WSAGetLastError());
			CloseSocketsAndWsaData();
			exit(ERROR_CODE);
		}
		else if (Status == 0) {
			continue;
		}
		else {
			if (FD_ISSET(Channel.ChannelSocket, &Readfds)) {
				ReceivedBufferLength = recvfrom(Channel.ChannelSocket, &ReceivedBuffer, CHUNK_SIZE, SEND_RECEIVE_FLAGS, NULL, NULL);
				if (ReceivedBufferLength == SOCKET_ERROR) {
					printf("HandleTraffic failed to recvfrom. Error Number is %d\n", WSAGetLastError());
					CloseSocketsAndWsaData();
					exit(ERROR_CODE);
				}
				printf("HandleTraffic received from sender %llu\n", ReceivedBuffer); // todo remove
				HandleReceiveFromSender(ReceivedBuffer, ReceivedBufferLength); // todo
			}
			if (FD_ISSET(Channel.ReceiverSocket, &Readfds)) {
				ReceivedBufferLength = recvfrom(Channel.ReceiverSocket, MessageFromReceiver, MESSAGE_LENGTH, SEND_RECEIVE_FLAGS, NULL, NULL);
				if (ReceivedBufferLength == SOCKET_ERROR) {
					printf("HandleTraffic failed to recvfrom. Error Number is %d\n", WSAGetLastError());
					CloseSocketsAndWsaData();
					exit(ERROR_CODE);
				}
				printf("HandleTraffic received from receiver %s\n", MessageFromReceiver); // todo remove
				HandleReceiveFromReceiver(MessageFromReceiver, ReceivedBufferLength); // todo
				break;
			}
		}
	}
	// todo need to close connection/s
}

void HandleReceiveFromSender(unsigned long long ReceivedBuffer, int ReceivedBufferLength) { // todo
	InsertErrors(&ReceivedBuffer);
	int SentBufferLength = sendto(Channel.ReceiverSocket, &ReceivedBuffer, ReceivedBufferLength, SEND_RECEIVE_FLAGS,
								 (SOCKADDR*)&Channel.ReceiverSocketService, sizeof(Channel.ReceiverSocketService));
	if (SentBufferLength == SOCKET_ERROR) {
		printf("HandleReceiveFromSender failed to sendto. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	printf("HandleReceiveFromSender sent to receiver %llu\n", ReceivedBuffer); // todo remove
}

void InsertErrors(unsigned long long *ReceivedBuffer) { // todo check bonus
	if (Channel.ErrorProbability == 0) {
		return;
	}
	unsigned int TotalNumberOfOptionsToError = ERROR_CONST / Channel.ErrorProbability;
	int IndexInBuffer = 0;
	unsigned long long ErrorMask = 0;
	int NumberOfErrors = 0; // todo remove
	for (; IndexInBuffer < (CHUNK_SIZE * 8); IndexInBuffer++) {
		if ((rand() % TotalNumberOfOptionsToError) == 1) {
			ErrorMask += 1;
			NumberOfErrors++;
		}
		ErrorMask = ErrorMask << 1;
	}
	*ReceivedBuffer = *ReceivedBuffer ^ ErrorMask;
	if (NumberOfErrors != 0) {
		printf("InsertErrors insert %d errors, pure data with error is %llu\n", NumberOfErrors, *ReceivedBuffer & 0x1FFFFFFFFFFFF); // todo remove
	}
}

void HandleReceiveFromReceiver(char *MessageFromReceiver, int ReceivedBufferLength) { // todo
	int SentBufferLength = sendto(Channel.ChannelSocket, MessageFromReceiver, ReceivedBufferLength, SEND_RECEIVE_FLAGS,
								 (SOCKADDR*)&Channel.SenderSocketService, sizeof(Channel.SenderSocketService));
	if (SentBufferLength == SOCKET_ERROR) {
		printf("HandleReceiveFromReceiver failed to sendto. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	Sleep(SEND_MESSAGES_WAIT);
}

void CloseSocketsAndWsaData() {
	int CloseSocketReturnValue;
	CloseSocketReturnValue = closesocket(Channel.ReceiverSocket); // todo add if adding more sockets
	if (CloseSocketReturnValue == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData failed to close ListeningToReceiverSocket. Error Number is %d\n", WSAGetLastError());
		exit(ERROR_CODE); // todo fix all prints
	}
	CloseSocketReturnValue = closesocket(Channel.ChannelSocket); // todo add if adding more sockets
	if (CloseSocketReturnValue == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData failed to close ListeningToSenderSocket. Error Number is %d\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
	if (WSACleanup() == SOCKET_ERROR) {
		printf("CloseSocketsAndWsaData Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
}