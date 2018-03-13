#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>

#include "Receiver.h"

#define LOCAL_PORT_NUM_ARGUMENT_INDEX 1
#define OUTPUT_FILE_NAME_ARGUMENT_INDEX 2
#define SOCKET_PROTOCOL 0
#define BINDING_SUCCEEDED 0
#define SEND_RECEIVE_FLAGS 0
#define USER_INPUT_LENGTH 20
#define CHUNK_SIZE_IN_BYTES 8
#define NUM_OF_THREADS 2
#define TIME_OUT_FOR_SELECT 500000
#define NUM_OF_DATA_BITS 49
#define NUM_OF_ERROR_BITS 15
#define NUM_OF_DATA_BITS_IN_A_ROW_COLUMN 7
#define COLUMN_PARITY_OFFSET 7
#define DATA_TO_WRITE_IN_BYTES 7
#define DATA_TO_WRITE_IN_BITS 56
#define NUMBER_OF_BITS_IN_ONE_BYTE 8
#define MESSAGE_LENGTH 20

void InitReceiver(char *argv[]);
void BindToPort();
void HandleReceiver();
HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id);
void WINAPI ConnectionWithChannelThread();
void WINAPI UserInterfaceThread();
void CreateOutputFile();
void FindAndFixError(unsigned long long *ReceivedBuffer);
void WriteInputToOutputFile(unsigned long long DataToWrite);
void HandleReceiveFromChannel();
void SendInformationToChannel();
void CloseSocketsThreadsAndWsaData();

void InitReceiver(char *argv[]) {
	Receiver.LocalPortNum = atoi(argv[LOCAL_PORT_NUM_ARGUMENT_INDEX]);
	Receiver.OutputFileName = argv[OUTPUT_FILE_NAME_ARGUMENT_INDEX];
	Receiver.ConnectionWithChannelThreadHandle = NULL;
	Receiver.UserInterfaceThreadHandle = NULL;
	Receiver.GotEndFromUser = false;
	Receiver.NumberOfErrorsDetected = 0;
	Receiver.NumberOfErrorsCorrected = 0;
	Receiver.NumberOfReceivedBytes = 0;
	Receiver.NumberOfWrittenBits = 0;
	Receiver.NumberOfSpareDataBits = 0;
	Receiver.SpareDataBitsForNextChunk = 0;
	
	WSADATA wsaData;
	int StartupRes = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (StartupRes != NO_ERROR) {
		printf("Error %ld at WSAStartup().\nExiting...\n", StartupRes);
		exit(ERROR_CODE);
	}

	Receiver.ReceiverSocket = socket(AF_INET, SOCK_DGRAM, SOCKET_PROTOCOL);
	if (Receiver.ReceiverSocket == INVALID_SOCKET) {
		printf("InitReceiver failed to create socket. Error Number is %d\n", WSAGetLastError());
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
}

void BindToPort() {
	int BindingReturnValue;
	Receiver.ReceiverSocketService.sin_family = AF_INET;
	Receiver.ReceiverSocketService.sin_addr.s_addr = INADDR_ANY;
	Receiver.ReceiverSocketService.sin_port = htons(Receiver.LocalPortNum);
	BindingReturnValue = bind(Receiver.ReceiverSocket, (SOCKADDR*)&Receiver.ReceiverSocketService,
							  sizeof(Receiver.ReceiverSocketService));
	if (BindingReturnValue != BINDING_SUCCEEDED) {
		printf("BindToPort failed to bind.\n");
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
}

void HandleReceiver() {
	Receiver.ConnectionWithChannelThreadHandle = CreateThreadSimple((LPTHREAD_START_ROUTINE)ConnectionWithChannelThread,
																	 NULL,
																	&Receiver.ConnectionWithChannelThreadID);
	Receiver.UserInterfaceThreadHandle = CreateThreadSimple((LPTHREAD_START_ROUTINE)UserInterfaceThread,
															 NULL,
															&Receiver.UserInterfaceThreadID);
	HANDLE ThreadsArray[NUM_OF_THREADS] = { Receiver.ConnectionWithChannelThreadHandle, Receiver.UserInterfaceThreadHandle };
	DWORD wait_code;
	wait_code = WaitForMultipleObjects(NUM_OF_THREADS, ThreadsArray, TRUE, INFINITE);
	if (WAIT_OBJECT_0 != wait_code) {
		printf("HandleReceiver failed to WaitForMultipleObjects.\n");
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
}

HANDLE CreateThreadSimple(LPTHREAD_START_ROUTINE p_start_routine, LPVOID p_thread_parameters, LPDWORD p_thread_id) {
	HANDLE thread_handle;

	thread_handle = CreateThread(
		NULL,                /*  default security attributes */
		0,                   /*  use default stack size */
		p_start_routine,     /*  thread function */
		p_thread_parameters, /*  argument to thread function */
		0,                   /*  use default creation flags */
		p_thread_id);        /*  returns the thread identifier */

	if (NULL == thread_handle) {
		printf("CreateThreadSimple failed to create thread.\n");
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
	return thread_handle;
}

void WINAPI ConnectionWithChannelThread() {
	unsigned long long ReceivedBuffer;
	int FromLen = sizeof(Receiver.ChannelSocketService);
	int ReceivedBufferLength = recvfrom(Receiver.ReceiverSocket, &ReceivedBuffer, CHUNK_SIZE_IN_BYTES, SEND_RECEIVE_FLAGS,
								   	   (SOCKADDR*)&Receiver.ChannelSocketService, &FromLen);
	if (ReceivedBufferLength == SOCKET_ERROR || ReceivedBufferLength != CHUNK_SIZE_IN_BYTES) {
		printf("ConnectionWithChannelThread failed to recvfrom. Error Number is %d\n", WSAGetLastError());
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
	printf("ConnectionWithChannelThread received from channel %llu\n", ReceivedBuffer); // todo remove
	CreateOutputFile();
	Receiver.NumberOfReceivedBytes += ReceivedBufferLength;
	FindAndFixError(&ReceivedBuffer);
	WriteInputToOutputFile(ReceivedBuffer);
	struct timeval Tv;
	//Tv.tv_sec = 20; // todo check
	Tv.tv_usec = TIME_OUT_FOR_SELECT;
	fd_set Allfds;
	fd_set Readfds;
	int Status;
	FD_ZERO(&Allfds);
	FD_ZERO(&Readfds);
	FD_SET(Receiver.ReceiverSocket, &Allfds);

	while (!Receiver.GotEndFromUser) {
		Readfds = Allfds;
		Status = select(0, &Readfds, NULL, NULL, &Tv);
		if (Status == SOCKET_ERROR) {
			printf("ConnectionWithChannelThread select failure. Error Number is %d\n", WSAGetLastError());
			CloseSocketsThreadsAndWsaData();
			exit(ERROR_CODE);
		}
		else if (Status == 0) {
			continue;
		}
		else {
			if (FD_ISSET(Receiver.ReceiverSocket, &Readfds)) {
				HandleReceiveFromChannel();
			}
		}
	}
	SendInformationToChannel();
	fprintf(stderr, "received: %d bytes\n", Receiver.NumberOfReceivedBytes);
	fprintf(stderr, "wrote: %d bytes\n", Receiver.NumberOfWrittenBits / NUMBER_OF_BITS_IN_ONE_BYTE);
	fprintf(stderr, "detected: %d errors, corrected: %d errors\n", Receiver.NumberOfErrorsDetected, Receiver.NumberOfErrorsCorrected);
}

void WINAPI UserInterfaceThread() {
	char UserInput[USER_INPUT_LENGTH];
	while (TRUE) {
		scanf("%s", UserInput);
		if (strcmp(UserInput, "End") == 0) {
			Receiver.GotEndFromUser = true;
			break;
		}
		else {
			printf("Not a valid input. To finish enter 'End'.\n");
		}
	}
}

void CreateOutputFile() {
	FILE *OutputFilePointer = fopen(Receiver.OutputFileName, "w");
	if (OutputFilePointer == NULL) {
		printf("CreateOutputFile couldn't open output file.\n");
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
	fclose(OutputFilePointer);
}

void FindAndFixError(unsigned long long *ReceivedBuffer) {
	unsigned long long ReceivedErrorBits = *ReceivedBuffer >> NUM_OF_DATA_BITS;
	unsigned long long CalculatedErrorBits = 0, XoredErrorBits;
	unsigned long long DataBits = *ReceivedBuffer - (ReceivedErrorBits << NUM_OF_DATA_BITS);
	int RowParity = 0, ColumnParity = 0, DiagonalParity = 0, RowParityPositionInData, ColumnParityPositionInData,
		RowBit, ColumnBit;

	for (int RowIndexInData = 0; RowIndexInData < NUM_OF_DATA_BITS_IN_A_ROW_COLUMN; RowIndexInData++) {
		for (int ColumnIndexInData = 0; ColumnIndexInData < NUM_OF_DATA_BITS_IN_A_ROW_COLUMN; ColumnIndexInData++) {
			RowParityPositionInData = ColumnIndexInData + NUM_OF_DATA_BITS_IN_A_ROW_COLUMN * RowIndexInData;
			RowBit = (DataBits >> RowParityPositionInData) & 1;
			RowParity = RowParity ^ RowBit;
			ColumnParityPositionInData = RowIndexInData + NUM_OF_DATA_BITS_IN_A_ROW_COLUMN * ColumnIndexInData;
			ColumnBit = (DataBits >> ColumnParityPositionInData) & 1;
			ColumnParity = ColumnParity ^ ColumnBit;
		}
		CalculatedErrorBits = CalculatedErrorBits + (RowParity << RowIndexInData);
		CalculatedErrorBits = CalculatedErrorBits + (ColumnParity << (RowIndexInData + NUM_OF_DATA_BITS_IN_A_ROW_COLUMN));
		DiagonalParity = DiagonalParity ^ ColumnParity;
		RowParity = 0;
		ColumnParity = 0;
	}
	CalculatedErrorBits = CalculatedErrorBits + (DiagonalParity << (NUM_OF_ERROR_BITS - 1));
	XoredErrorBits = CalculatedErrorBits ^ ReceivedErrorBits;
	if (XoredErrorBits == 0) { // no error detected
		return;
	}
	Receiver.NumberOfErrorsDetected++;
	int NumberOfRowErrors = 0, NumberOfColumnErrors = 0, IndexOfRowError = 0, IndexOfColumnError = 0,
		CurrentRowErrorBit, CurrentColumnErrorBit;
	for (int IndexInXoredErrorBits = 0; IndexInXoredErrorBits < COLUMN_PARITY_OFFSET; IndexInXoredErrorBits++) {
		CurrentRowErrorBit = (XoredErrorBits >> IndexInXoredErrorBits) & 1;
		CurrentColumnErrorBit = (XoredErrorBits >> (IndexInXoredErrorBits + COLUMN_PARITY_OFFSET)) & 1;
		if (CurrentRowErrorBit != 0) {
			NumberOfRowErrors++;
			IndexOfRowError = IndexInXoredErrorBits;
		}
		if (CurrentColumnErrorBit != 0) {
			NumberOfColumnErrors++;
			IndexOfColumnError = IndexInXoredErrorBits;
		}
	}
	if (NumberOfRowErrors == 1 && NumberOfColumnErrors == 1) { // todo check condition
		int ErrorBitPosition = IndexOfRowError * COLUMN_PARITY_OFFSET + IndexOfColumnError;
		unsigned long long FixMask = 1 << ErrorBitPosition;
		DataBits = DataBits ^ FixMask;
		*ReceivedBuffer = DataBits; // no need to add error bits
		Receiver.NumberOfErrorsCorrected++;
		printf("FindAndFixError fixed data is %llu\n", *ReceivedBuffer); // todo remove
	}
}

void WriteInputToOutputFile(unsigned long long ReceivedBuffer) { // todo fix
	FILE *OutputFilePointer = fopen(Receiver.OutputFileName, "a");
	if (OutputFilePointer == NULL) {
		printf("WriteInputToOutputFile couldn't open output file.\n");
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
	int WroteElements;
	unsigned long long ErrorBits = (ReceivedBuffer >> NUM_OF_DATA_BITS);
	unsigned long long DataBits = ReceivedBuffer - (ErrorBits << NUM_OF_DATA_BITS);
	unsigned long long DataToWrite;

	//Receiver.NumberOfSpareDataBits += NUM_OF_DATA_BITS;
	if (Receiver.NumberOfSpareDataBits + NUM_OF_DATA_BITS < DATA_TO_WRITE_IN_BITS) {
		/*Receiver.SpareDataBitsForNextChunk = Receiver.SpareDataBitsForNextChunk << (CHUNK_SIZE_IN_BITS - Receiver.NumberOfCurrentDataBits);
		Receiver.SpareDataBitsForNextChunk;
		
		ReceivedBuffer = ReceivedBuffer << Receiver.NumberOfCurrentDataBits;
		Receiver.SpareDataBitsForNextChunk = ReceivedBuffer;*/
		Receiver.NumberOfSpareDataBits += NUM_OF_DATA_BITS;
		Receiver.SpareDataBitsForNextChunk = DataBits;
		return; // not enough data to write
	}
	DataToWrite = Receiver.SpareDataBitsForNextChunk << (DATA_TO_WRITE_IN_BITS - Receiver.NumberOfSpareDataBits);
	Receiver.NumberOfSpareDataBits = Receiver.NumberOfSpareDataBits + NUM_OF_DATA_BITS - DATA_TO_WRITE_IN_BITS;
	DataToWrite += DataBits >> Receiver.NumberOfSpareDataBits;
	Receiver.SpareDataBitsForNextChunk = DataBits - ((DataBits >> Receiver.NumberOfSpareDataBits) << Receiver.NumberOfSpareDataBits);
	char *Temp = &DataToWrite;
	WroteElements = fwrite(&DataToWrite, DATA_TO_WRITE_IN_BYTES, 1, OutputFilePointer);
	if (WroteElements != 1) {
		printf("WriteInputToOutputFile error in writing to file.\n");
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
	fclose(OutputFilePointer);
	Receiver.NumberOfWrittenBits += NUM_OF_DATA_BITS;
}

void HandleReceiveFromChannel() {
	unsigned long long ReceivedBuffer;
	int ReceivedBufferLength = recvfrom(Receiver.ReceiverSocket, &ReceivedBuffer, CHUNK_SIZE_IN_BYTES, SEND_RECEIVE_FLAGS, NULL, NULL);
	if (ReceivedBufferLength == SOCKET_ERROR) {
		printf("HandleReceiveFromChannel failed to recvfrom. Error Number is %d\n", WSAGetLastError());
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
	printf("HandleReceiveFromChannel received from channel %llu\n", ReceivedBuffer); // todo remove
	Receiver.NumberOfReceivedBytes += ReceivedBufferLength;
	WriteInputToOutputFile(ReceivedBuffer); // todo check if write before/after correcting error
}

void SendInformationToChannel() {
	char MessageToChannel[MESSAGE_LENGTH];
	sprintf(MessageToChannel, "%d\n%d\n%d\n%d\n", Receiver.NumberOfReceivedBytes, Receiver.NumberOfWrittenBits/NUMBER_OF_BITS_IN_ONE_BYTE,
			Receiver.NumberOfErrorsDetected, Receiver.NumberOfErrorsCorrected);
	int SentBufferLength = sendto(Receiver.ReceiverSocket, MessageToChannel, strlen(MessageToChannel), SEND_RECEIVE_FLAGS,
								 (SOCKADDR*)&Receiver.ChannelSocketService, sizeof(Receiver.ChannelSocketService));
	if (SentBufferLength == SOCKET_ERROR) {
		printf("SendInformationToChannel failed to sendto. Error Number is %d\n", WSAGetLastError());
		CloseSocketsThreadsAndWsaData();
		exit(ERROR_CODE);
	}
}

void CloseSocketsThreadsAndWsaData() { // todo check // todo add threads
	int CloseSocketReturnValue;
	DWORD ret_val;
	CloseSocketReturnValue = closesocket(Receiver.ReceiverSocket); // todo add if adding more sockets
	if (CloseSocketReturnValue == SOCKET_ERROR) {
		printf("CloseSocketsThreadsAndWsaData failed to close socket. Error Number is %d\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
	if (Receiver.ConnectionWithChannelThreadHandle != NULL) {
		ret_val = CloseHandle(Receiver.ConnectionWithChannelThreadHandle);
		if (FALSE == ret_val) {
			printf("CloseSocketsThreadsAndWsaData failed to close ConnectionWithChannelThreadHandle.\n");
			exit(ERROR_CODE);
		}
	}
	if (Receiver.UserInterfaceThreadHandle != NULL) {
		ret_val = CloseHandle(Receiver.UserInterfaceThreadHandle);
		if (FALSE == ret_val) {
			printf("CloseSocketsThreadsAndWsaData failed to close UserInterfaceThreadHandle.\n");
			exit(ERROR_CODE);
		}
	}
	if (WSACleanup() == SOCKET_ERROR) {
		printf("CloseSocketsThreadsAndWsaData Failed to close Winsocket, error %ld. Ending program.\n", WSAGetLastError());
		exit(ERROR_CODE);
	}
}