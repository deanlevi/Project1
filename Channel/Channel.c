#include <stdlib.h>

#include "Channel.h"

#define LOCAL_PORT_NUM_ARGUMENT_INDEX 1
#define RECEIVER_IP_ADDRESS_ARGUMENT_INDEX 2
#define RECEIVER_PORT_NUM_ARGUMENT_INDEX 3
#define ERROR_PROBABILITY_ARGUMENT_INDEX 4
#define RANDOM_SEED_ARGUMENT_INDEX 5
#define SOCKET_PROTOCOL 0
#define BINDING_SUCCEEDED 0
#define BUFFER_LENGTH 2 // todo

void InitChannel(char *argv[]) {
	Channel.LocalPortNum = atoi(argv[LOCAL_PORT_NUM_ARGUMENT_INDEX]);
	Channel.ReceiverIPAddress = argv[RECEIVER_IP_ADDRESS_ARGUMENT_INDEX];
	Channel.ReceiverPortNum = atoi(argv[RECEIVER_PORT_NUM_ARGUMENT_INDEX]);
	Channel.ErrorProbability = atoi(argv[ERROR_PROBABILITY_ARGUMENT_INDEX]);
	Channel.RandomSeed = atoi(argv[RANDOM_SEED_ARGUMENT_INDEX]);

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
	Channel.ReceiverSocketService.sin_family = AF_INET;
	Channel.ReceiverSocketService.sin_addr.s_addr = inet_addr(Channel.ReceiverIPAddress);
	Channel.ReceiverSocketService.sin_port = htons(Channel.ReceiverPortNum);
	BindingReturnValue = bind(Channel.ReceiverSocket, (SOCKADDR*)&Channel.ReceiverSocketService,
							  sizeof(Channel.ReceiverSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		printf("BindToPort failed to bind.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	Channel.ChannelSocketService.sin_family = AF_INET;
	Channel.ChannelSocketService.sin_addr.s_addr = inet_addr(INADDR_ANY);
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
	fd_set Allfds;
	fd_set Readfds;
	int Status;
	FD_ZERO(&Allfds);
	FD_SET(Channel.ReceiverSocket, &Allfds);
	FD_SET(Channel.ChannelSocket, &Allfds);

	while (TRUE) {
		Readfds = Allfds;
		Status = select(0, &Readfds, NULL, NULL, NULL);
		if (Status == SOCKET_ERROR) {
			printf("HandleTraffic select failure.\n");
			CloseSocketsAndWsaData();
			exit(ERROR_CODE);
		}
		else if (Status == 0) {
			continue;
		}
		else {
			if (FD_ISSET(Channel.ChannelSocket, &Readfds)) {
				HandleReceiveFromSender(); // todo
			}
			if (FD_ISSET(Channel.ReceiverSocket, &Readfds)) {
				HandleReceiveFromReceiver(); // todo
				break;
			}
		}
	}
	// todo need to close connection/s
}

void HandleReceiveFromSender() {
	char ReceivedBuffer[BUFFER_LENGTH];
	int ReceivedBufferLength = recvfrom(Receiver.ListeningSocket, &ReceivedBuffer, BUFFER_LENGTH, SEND_RECEIVE_FLAGS, NULL, NULL);
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