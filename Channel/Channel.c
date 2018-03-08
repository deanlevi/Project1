#include <stdlib.h>

#include "Channel.h"

#define LOCAL_PORT_NUM_ARGUMENT_INDEX 1
#define RECEIVER_IP_ADDRESS_ARGUMENT_INDEX 2
#define RECEIVER_PORT_NUM_ARGUMENT_INDEX 3
#define ERROR_PROBABILITY_ARGUMENT_INDEX 4
#define RANDOM_SEED_ARGUMENT_INDEX 5
#define SOCKET_PROTOCOL 0
#define BINDING_SUCCEEDED 0

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

	Channel.ListeningToReceiverSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Channel.ListeningToReceiverSocket == INVALID_SOCKET) {
		printf("InitChannel failed to create ListeningToReceiverSocket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	Channel.ListeningToSenderSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Channel.ListeningToSenderSocket == INVALID_SOCKET) {
		printf("InitChannel failed to create ListeningToSenderSocket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void BindToPort() {
	int BindingReturnValue;
	Channel.ListeningToReceiverSocketService.sin_family = AF_INET;
	Channel.ListeningToReceiverSocketService.sin_addr.s_addr = inet_addr(Channel.ReceiverIPAddress);
	Channel.ListeningToReceiverSocketService.sin_port = htons(Channel.ReceiverPortNum);
	BindingReturnValue = bind(Channel.ListeningToReceiverSocket, (SOCKADDR*)&Channel.ListeningToReceiverSocketService,
							  sizeof(Channel.ListeningToReceiverSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	Channel.ListeningToSenderSocketService.sin_family = AF_INET;
	Channel.ListeningToSenderSocketService.sin_addr.s_addr = inet_addr(INADDR_ANY);
	Channel.ListeningToSenderSocketService.sin_port = htons(Channel.LocalPortNum);
	BindingReturnValue = bind(Channel.ListeningToSenderSocket, (SOCKADDR*)&Channel.ListeningToSenderSocketService,
							  sizeof(Channel.ListeningToSenderSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
}

void HandleTraffic() {
	fd_set Allfds;
	fd_set Readfds;
	int Status;
	FD_ZERO(&Allfds);
	FD_SET(Channel.ListeningToReceiverSocket, &Allfds);
	FD_SET(Channel.ListeningToSenderSocket, &Allfds);

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
			if (FD_ISSET(Channel.ListeningToSenderSocket, &Readfds)) {
				HandleReceiveFromSender(); // todo
			}
			if (FD_ISSET(Channel.ListeningToReceiverSocket, &Readfds)) {
				HandleReceiveFromReceiver(); // todo
				break;
			}
		}
	}
	// todo need to close connection/s
}