#include <stdio.h>

#define ERROR_CODE (int)(-1) // todo check if move
#define SUCCESS_CODE 0

int main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("Not the right amount of input arguments.\nNeed to give two.\nExiting...\n"); // first is path, other two are inputs
		return ERROR_CODE;
	}
	InitServer(argv);
	HandleServer();
	CloseSocketsAndThreads();

	return SUCCESS_CODE;
}