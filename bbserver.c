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
#include "bbserver.h"

// Defines
#define ERROR -1
#define SUCCESS 0

#define BUFFER_SIZE 256
#define RED "\x1b[31m"
#define BLUE   "\x1B[34m"
#define YELLOW   "\x1B[33m"
#define RESET "\x1B[0m"

int main(int argc, char **argv) 
{
	int sockfd;
	if (argc != 2)
	{
		fprintf(stderr, RED"Input Error: "RESET "Anticipated input --> ./bbserver <# of hosts>\n");
		exit(EXIT_FAILURE);
	}

	if ((sockfd = createServer()) == ERROR)
		exit(EXIT_FAILURE);
	runServer(sockfd, atoi(argv[1]));

	printf("Server Closed.\n");
	return 0;
}

int createServer()
{
	int status, sockfd;
	struct addrinfo hints, *serverInfo;

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM;  // UDP sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ((status = getaddrinfo(NULL, "6969", &hints, &serverInfo)) != 0) 
	{
		fprintf(stderr, RED"Server Error: "RESET "getaddrinfo() error = [%s]\n", gai_strerror(status));
		return ERROR;
	}

	// Make the socket
	sockfd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
	if (sockfd == -1)
	{
		perror(RED"Server Error: "RESET "Socket creation failed.\n");
		return ERROR;
	}

	// Reset
	int yes = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	// Bind the socket
	if (bind(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
	{
		close(sockfd);
		perror(RED"Server Error: "RESET "Failed to bind socket.\n");
		return ERROR;
	}

	// Display server information
	char hostName[BUFFER_SIZE];
	gethostname(hostName, BUFFER_SIZE - 1);
	printf(YELLOW"        Server Information        \n");
	printf("==================================\n"RESET);
	printf("Host Address:\t %s\n", hostName);
	printf("Port        :\t %d\n", ntohs(getPort((struct sockaddr *)serverInfo->ai_addr)));
	printf(YELLOW"==================================\n"RESET);

	freeaddrinfo(serverInfo);

	return sockfd;
}


void *get_in_addr(struct sockaddr *_sa)
{
	if (_sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)_sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)_sa)->sin6_addr);
}

in_port_t getPort(struct sockaddr *_sa)
{
	if (_sa->sa_family == AF_INET) {
		return (((struct sockaddr_in*)_sa)->sin_port);
	}

	return (((struct sockaddr_in6*)_sa)->sin6_port);
}

void runServer(int _sockfd, int _numClients)
{
	char buffer[BUFFER_SIZE];
	char ipBuffer[INET6_ADDRSTRLEN];
	struct sockaddr_storage clientAddr[_numClients];
	socklen_t addr_len;
	int numBytes = 0;

	int i = 0;
	for(i = 0; i < _numClients; i++)
	{
		printf("Waiting for a connection...\n");
		addr_len = sizeof(clientAddr);
		numBytes = recvfrom(_sockfd, buffer, sizeof(buffer), 0,
			       (struct sockaddr*)&clientAddr[i], &addr_len);
		if (numBytes == -1)
		{
			printf("numBytes = [%d]\n", numBytes);
			perror(RED"Server Error: "RESET "recvfrom() failed. \n");
			exit(1);
		}
		const char *ipAddress;
		ipAddress = inet_ntop(clientAddr[i].ss_family, 
							  get_in_addr((struct sockaddr*)&clientAddr[i]), 
							  ipBuffer, 
							  sizeof(ipBuffer));
		printf(YELLOW"\nA host from %s has connected with: \t%s\t\n"RESET, ipAddress, buffer);
	}

	close(_sockfd);
}
