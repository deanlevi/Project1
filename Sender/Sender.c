#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>

#include "Sender.h"

#define CHANNEL_IP_ADDRESS_ARGUMENT_INDEX 1
#define CHANNEL_PORT_NUM_ARGUMENT_INDEX 2
#define INPUT_FILE_TO_TRANSFER_ARGUMENT_INDEX 3
#define SOCKET_PROTOCOL 0
#define INPUT_CHUNK_SIZE 7 // 7 bytes = 56 bits
#define OUTPUT_CHUNK_SIZE 8 // 8 bytes = 64 bits
#define NUM_OF_SPARE_BITS_FROM_EACH_CHUNK 7 // 56 - 49 = 7
#define NUM_OF_DATA_BITS_IN_ONE_CHUNK 49
#define NUM_OF_ERROR_BITS 15
#define NUM_OF_BITS_IN_A_ROW_COLUMN 7
#define SEND_RECEIVE_FLAGS 0
#define MESSAGE_LENGTH 20
#define SEND_MESSAGES_WAIT 100

void InitSender(char *argv[]);
void AddErrorFixingBits(unsigned long long *DataToSend);
void SendData(unsigned long long DataToSend);
void ParseParameter(int *ParameterToUpdate, int *StartIndex, int *EndIndex, char MessageFromChannel[MESSAGE_LENGTH]);
void CloseSocketsAndWsaData();

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

void HandleSendFile() {
	FILE *InputFilePointer = fopen(Sender.InputFileToTransfer, "r");
	if (InputFilePointer == NULL) {
		printf("HandleSendFile couldn't open input file.\n");
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	unsigned long long InputChunk = 0, DataToSend = 0, DataToSendNext = 0, TempData = 0;
	int DataToSendNextOffset = 0;
	size_t ReadElements;
	Sleep(SEND_MESSAGES_WAIT); // if starting all at the same time

	while (TRUE) {
		if (DataToSendNextOffset < NUM_OF_DATA_BITS_IN_ONE_CHUNK) {
			ReadElements = fread(&InputChunk, INPUT_CHUNK_SIZE, 1, InputFilePointer);
			if (ReadElements != 1) { // reached EOF
				break;
			}
			DataToSend = DataToSendNext << (NUM_OF_DATA_BITS_IN_ONE_CHUNK - DataToSendNextOffset);
			DataToSendNextOffset = DataToSendNextOffset + NUM_OF_SPARE_BITS_FROM_EACH_CHUNK;
			TempData = InputChunk >> DataToSendNextOffset;
			DataToSend += TempData;
			DataToSendNext = InputChunk - (TempData << DataToSendNextOffset);
		}
		else {
			DataToSend = DataToSendNext;
			DataToSendNext = 0;
			DataToSendNextOffset = 0;
		}
		char *Temp = &InputChunk; // todo remove
		AddErrorFixingBits(&DataToSend);
		SendData(DataToSend);
		InputChunk = 0;
	}
	fclose(InputFilePointer);
}

void SendData(unsigned long long DataToSend) {
	int SentBufferLength = sendto(Sender.ChannelSocket, &DataToSend, OUTPUT_CHUNK_SIZE, SEND_RECEIVE_FLAGS,
								 (SOCKADDR*)&Sender.ChannelSocketService, sizeof(Sender.ChannelSocketService));
	if (SentBufferLength == SOCKET_ERROR) {
		printf("SendData failed to sendto. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	printf("SendData sent %llu  Pure data is %llu\n", DataToSend, DataToSend & 0x1FFFFFFFFFFFF); // todo remove
	Sleep(SEND_MESSAGES_WAIT);
}

void AddErrorFixingBits(unsigned long long *DataToSend) {
	unsigned long long ErrorBits = 0; // todo
	int RowParity = 0, ColumnParity = 0, DiagonalParity = 0, RowParityPositionInDataToSend, ColumnParityPositionInDataToSend,
		RowBit, ColumnBit;

	for (int RowIndexInDataToSend = 0; RowIndexInDataToSend < NUM_OF_BITS_IN_A_ROW_COLUMN; RowIndexInDataToSend++) {
		for (int ColumnIndexInDataToSend = 0; ColumnIndexInDataToSend < NUM_OF_BITS_IN_A_ROW_COLUMN; ColumnIndexInDataToSend++) {
			RowParityPositionInDataToSend = ColumnIndexInDataToSend + NUM_OF_BITS_IN_A_ROW_COLUMN * RowIndexInDataToSend;
			RowBit = (*DataToSend >> RowParityPositionInDataToSend) & 1;
			RowParity = RowParity ^ RowBit;
			ColumnParityPositionInDataToSend = RowIndexInDataToSend + NUM_OF_BITS_IN_A_ROW_COLUMN * ColumnIndexInDataToSend;
			ColumnBit = (*DataToSend >> ColumnParityPositionInDataToSend) & 1;
			ColumnParity = ColumnParity ^ ColumnBit;
		}
		ErrorBits = ErrorBits + (RowParity << RowIndexInDataToSend);
		ErrorBits = ErrorBits + (ColumnParity << (RowIndexInDataToSend + NUM_OF_BITS_IN_A_ROW_COLUMN));
		DiagonalParity = DiagonalParity ^ ColumnParity;
		RowParity = 0;
		ColumnParity = 0;
	}
	ErrorBits = ErrorBits + (DiagonalParity << (NUM_OF_ERROR_BITS - 1)); // ErrorBits are 15 bits
	ErrorBits = ErrorBits << (NUM_OF_BITS_IN_A_ROW_COLUMN*NUM_OF_BITS_IN_A_ROW_COLUMN); // shift 49 to the left
	*DataToSend = *DataToSend + ErrorBits; // make error bits msb of data
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

void HandleReceiveFromChannel() {
	char MessageFromChannel[MESSAGE_LENGTH];
	int ReceivedBufferLength;
	fd_set Allfds;
	fd_set Readfds;
	int Status;
	FD_ZERO(&Allfds);
	FD_SET(Sender.ChannelSocket, &Allfds);

	while (TRUE) {
		Readfds = Allfds;
		Status = select(0, &Readfds, NULL, NULL, NULL);
		if (Status == SOCKET_ERROR) {
			printf("HandleReceiveFromChannel select failure. Error Number is %d\n", WSAGetLastError());
			CloseSocketsAndWsaData();
			exit(ERROR_CODE);
		}
		else if (Status == 0) {
			continue;
		}
		else {
			if (FD_ISSET(Sender.ChannelSocket, &Readfds)) {
				ReceivedBufferLength = recvfrom(Sender.ChannelSocket, MessageFromChannel, MESSAGE_LENGTH, SEND_RECEIVE_FLAGS, NULL, NULL);
				if (ReceivedBufferLength == SOCKET_ERROR) {
					printf("HandleReceiveFromChannel failed to recvfrom. Error Number is %d\n", WSAGetLastError());
					CloseSocketsAndWsaData();
					exit(ERROR_CODE);
				}
				break;
			}
		}
	}
	printf("HandleReceiveFromChannel received from receiver %s\n", MessageFromChannel); // todo remove
	int NumberOfReceivedBytes, NumberOfWrittenBytes, NumberOfErrorsDetected, NumberOfErrorsCorrected;
	int StartIndex = 0, EndIndex = 0;

	ParseParameter(&NumberOfReceivedBytes, &StartIndex, &EndIndex, MessageFromChannel);
	ParseParameter(&NumberOfWrittenBytes, &StartIndex, &EndIndex, MessageFromChannel);
	ParseParameter(&NumberOfErrorsDetected, &StartIndex, &EndIndex, MessageFromChannel);
	ParseParameter(&NumberOfErrorsCorrected, &StartIndex, &EndIndex, MessageFromChannel);

	fprintf(stderr, "received: %d bytes\n", NumberOfReceivedBytes);
	fprintf(stderr, "wrote: %d bytes\n", NumberOfWrittenBytes);
	fprintf(stderr, "detected: %d errors, corrected: %d errors\n", NumberOfErrorsDetected, NumberOfErrorsCorrected);
}

void ParseParameter(int *ParameterToUpdate, int *StartIndex, int *EndIndex, char MessageFromChannel[MESSAGE_LENGTH]) {
	char ParameterAsString[MESSAGE_LENGTH];
	while (MessageFromChannel[*EndIndex] != '\n') {
		(*EndIndex)++;
	}
	strncpy(ParameterAsString, MessageFromChannel + *StartIndex, *EndIndex - *StartIndex);
	*ParameterToUpdate = atoi(ParameterAsString);
	(*EndIndex)++;
	*StartIndex = *EndIndex;
}

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