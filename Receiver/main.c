#include <stdio.h>

#include "Receiver.h"

#define SUCCESS_CODE 0 // todo check if move

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Not the right amount of input arguments.\nNeed to give two.\nExiting...\n"); // first is path, other two are inputs
		return ERROR_CODE;
	}
	InitReceiver(argv);
	BindToPort();
	HandleConnectionWithChannel();

	CloseSocketsAndWsaData();
	return SUCCESS_CODE;
}