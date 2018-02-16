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
#define BUFFER_SIZE 256

int main(int argc, char **argv) 
{
	int sockfd;

	if((sockfd = createServer()) == ERROR)
		exit(EXIT_FAILURE);

	printf("Program completed\n");
	return 0;
}

int createServer()
{
	int status, sockfd;
	struct addrinfo hints, *serverInfo;

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ((status = getaddrinfo(NULL, "6969", &hints, &serverInfo)) != 0) 
	{
		fprintf(stderr, "Server Error: getaddrinfo() error = [%s]\n", gai_strerror(status));
		return ERROR;
	}

	// Make the socket
	if ((sockfd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)
		== -1))
	{
		perror("Server Error: Socket creation failed.\n");
		return ERROR;
	}

	// Bind the socket
	if (bind(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
	{
		close(sockfd);
		perror("Server Error: Failed to bind socket.\n");
		return ERROR;
	}


	/*
	*	Display server information after creation.
	*/
	char *serverIP = getIP(sockfd);

	freeaddrinfo(serverInfo);

	return status;
}

char* getIP(int sockfd)
{
	char ipAddress[INET_ADDRSTRLEN];
	struct sockaddr sock;
	struct sockaddr_in *sock_ptr = (struct sockaddr_in *) &sock;
	socklen_t sock_len;

	sock_len = sizeof(sock_ptr);
	if (getpeername(sockfd, &sock, &sock_len) == -1)
	{
		perror("Server Error: getpeername() failed.\n");
		return -1;
	}

	printf("IP address is: %s\n", inet_ntoa(sock_ptr->sin_addr));
	printf("Port is: %d\n", (int)ntohs(sock_ptr->sin_port));

	return inet_ntoa(sock_ptr->sin_addr);
}
