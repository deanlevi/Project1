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

void InitSender(char *argv[]);
void AddErrorFixingBits(unsigned long long *DataToSend);
void SendData(unsigned long long DataToSend);
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
	char *DataToSendAsString; // todo
	int DataToSendNextOffset = 0;
	size_t ReadElements;
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
		AddErrorFixingBits(&DataToSend);
		//DataToSendAsString = (char *)&DataToSend; // todo
		SendData(DataToSend);
		InputChunk = 0;
	}
	fclose(InputFilePointer);
}

void SendData(unsigned long long DataToSend) {
	int SentBufferLength = sendto(Sender.ChannelSocket, &DataToSend, OUTPUT_CHUNK_SIZE, SEND_RECEIVE_FLAGS,
								 (SOCKADDR*)&Sender.ChannelSocketService, sizeof(Sender.ChannelSocketService));
	if (SentBufferLength == SOCKET_ERROR) {
		printf("TempSendData failed to sendto. Error Number is %d\n", WSAGetLastError());
		CloseSocketsAndWsaData();
		exit(ERROR_CODE);
	}
	Sleep(1000); // todo
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
	ErrorBits = ErrorBits + (DiagonalParity << NUM_OF_ERROR_BITS); // ErrorBits are 15 bits
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