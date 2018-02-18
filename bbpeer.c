#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Project
#include "bbpeer.h"

// Defines
#define ERROR -1
#define RED "\x1b[31m"
#define BLUE   "\x1B[34m"
#define YELLOW   "\x1B[33m"
#define RESET "\x1B[0m"

int main(int argc, char **argv) 
{
	if (argc != 4)
	{
		fprintf(stderr, RED"Input Error: "RESET "Anticipated input -->"
				"./bbpeer <Server Hostname> <Port>\n");
		exit(EXIT_FAILURE);
	}
	printf(BLUE "Debug:"RESET " Connecting to [%s].\n", argv[1]);
	printf(BLUE "Debug:"RESET " Port # [%s].\n", argv[2]);

	printf("Debug: BBPeer disconnected.\n");
	return 0;
}