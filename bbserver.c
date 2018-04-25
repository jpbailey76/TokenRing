#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Project
#include "bbserver.h"

// Flags
#define ERROR -1
#define SUCCESS 0

//	Buffer
#define BUFFER_SIZE 256

// Styling
#define RED "\x1b[31m"
#define BLUE   "\x1B[34m"
#define YELLOW   "\x1B[33m"
#define RESET "\x1B[0m"

// Debug flag
const bool DEBUG = false;

int main(int argc, char **argv) 
{
	// Server info
	int sockfd;
	ServerInfo server;

	// Check program args
	verifyInput(argc, argv, &server);
	if(DEBUG)
	{
		printf(BLUE"DEBUG: "RESET
		"numPeers = [%d]\tport = [%d]\n", server.numClients, server.port);
	}

	// Create server
	if ((sockfd = createServer(&server)) == ERROR)
	{
		printf(RED"ERROR: "RESET
						"Failed to create server!\n");
		exit(EXIT_FAILURE);
	}

	// Create peer array and run server.
  PeerInfo *peerArray = createPeerArray(&server);

  // Run server
	runServer(sockfd, peerArray, &server);

	// Exit
	printf(YELLOW"\nAll peers connected and the ring has been established.\n"
				 "The server is now exiting gracefully.\n"RESET);
	return 0;
}

void verifyInput(int argc, char **argv, ServerInfo *PN)
{
  if (3 > argc) 
  {
      printf(YELLOW"Anticipated input --> "RESET"./bbserver <port #> <# of hosts>\n");
      exit(EXIT_FAILURE);
  }

  // Check for valid port range.
	int port, temp;
  port = strtol(argv[1], NULL, 0);
  if (60000 > port || 60099 < port) 
  {
    fprintf(stderr,RED"Input Error: "RESET "Port must be in the range [60000, 60099]\n");
    printf("Anticipated input --> ./bbserver <port #> <# of hosts>\n");
    exit(EXIT_FAILURE);
  }

  temp = atoi(argv[2]);
  PN->numClients = temp;
  PN->port = port;
}

PeerInfo *createPeerArray(ServerInfo *server) 
{
  PeerInfo *peer;

  peer = malloc(server->numClients*sizeof(PeerInfo));
  if(peer == NULL) 
  {
      fprintf(stderr, "Memory allocation failed!\n");
      exit(EXIT_FAILURE);
  }

  return peer;
}

int createServer(ServerInfo *server)
{
	int status, sockfd;
	struct addrinfo hints, *serverAddrInfo;
	char port[32];
	sprintf(port, "%d", server->port);

	memset((void *)&hints, 0, (size_t) sizeof(hints));
	hints.ai_family = AF_INET;    
	hints.ai_socktype = SOCK_DGRAM;  
	hints.ai_flags = AI_PASSIVE;     

	//
	if ((status = getaddrinfo(NULL, port, &hints, &serverAddrInfo)) != 0) 
	{
		fprintf(stderr, RED"Server Error: "RESET "getaddrinfo() error = [%s]\n", gai_strerror(status));
		return ERROR;
	}

	// Bind the socket 
	if ((sockfd = bindSocket(serverAddrInfo)) < 0)
	{
		perror("Error: Failed to bind to socket :");
		exit(EXIT_FAILURE);
	}

	// Display server information
	char hostName[BUFFER_SIZE];
	gethostname(hostName, BUFFER_SIZE - 1);
	printf(YELLOW"        Server Information        \n");
	printf("==================================\n"RESET);
	printf("Host Address:\t %s\n", hostName);
	printf("Port        :\t %d\n", ntohs(getPort((struct sockaddr *)serverAddrInfo->ai_addr)));
	printf(YELLOW"==================================\n"RESET);

	freeaddrinfo(serverAddrInfo);

	return sockfd;
}

in_port_t getPort(struct sockaddr *_sa)
{
	if (_sa->sa_family == AF_INET) 
	{
		return (((struct sockaddr_in*)_sa)->sin_port);
	}

	return (((struct sockaddr_in6*)_sa)->sin6_port);
}

void runServer(int _sockfd, PeerInfo *_peerArray, ServerInfo *_server)
{
	char buffer[BUFFER_SIZE];
	char ipAddress[INET_ADDRSTRLEN];
	socklen_t addr_len;
	int numBytes = 0;

	int i = 0, j = 0;
	for(i = 0; i < _server->numClients; i++)
	{
		printf(YELLOW"\nWaiting for a connection. . .\n"RESET);
		addr_len = sizeof(_peerArray[i].client);
		numBytes = recvfrom(_sockfd, buffer, sizeof(buffer), 0,
			       (struct sockaddr*)&_peerArray[i].client, &addr_len);
		if (numBytes == -1)
		{
			printf("numBytes = [%d]\n", numBytes);
			perror(RED"Server Error: "RESET "recvfrom() failed. \n");
			exit(1);
		}

    // The ipAddress:Port
    if (NULL == inet_ntop(AF_INET, &_peerArray[i].client.sin_addr, ipAddress, sizeof ipAddress))
        perror(RED"Server Error: "RESET 
	     		"runServer() - Address printing failed. \n");
		printf(YELLOW"\nA host from"RESET" %s:%d "YELLOW"has connected with:"RESET"\t%s\t\n", ipAddress, _peerArray[i].client.sin_port, buffer);
	}

	// Notify clients of thier position in the ring.
  for (i = 0; i < _server->numClients; i++)
  {
      j = (i + 1) % _server->numClients;
      memcpy(&_peerArray[i].neighbor, &_peerArray[j].client, sizeof (struct sockaddr_in));
      sendto(_sockfd, &_peerArray[i], sizeof (PeerInfo), 0, (struct sockaddr *) &_peerArray[i].client, sizeof (struct sockaddr_in));
  }

	close(_sockfd);
}

int bindSocket(struct addrinfo *res)
{
	int sockfd = -1;
	struct addrinfo *p = NULL;

	// Bind socket
	for (p = res; p != NULL; p = p->ai_next) 
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd < 0) 
		{
			perror(RED"Server Error: "RESET "Failed to create socket.\n");
			continue;
		}

		// Reset
		int yes = 1;
		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) 
		{   
			close(sockfd);
			continue;
		}
		break;  
	}

	if (p == NULL) 
		return -1;

	return sockfd;
}
