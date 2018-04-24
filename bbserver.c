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
#define USAGE "Anticipated input --> ./bbserver <port #> <# of hosts>\n"

int main(int argc, char **argv) 
{
	int sockfd;
	PortNT server;
	verifyInput(argc, argv, &server);

	printf(BLUE"DEBUG: "RESET
		"numPeers = [%d]\tport = [%d]\n", server.numClients, server.port);

	if ((sockfd = createServer(&server)) == ERROR)
	{
		exit(EXIT_FAILURE);
	}

	// Create peer array and run server.
  PeerT *peerArray = new_parray(&server);

	runServer(sockfd, peerArray, &server);

	printf(BLUE"\nDEBUG: "RESET
  			"Client address = [%s] Peer address = [%s]\n", inet_ntoa(peerArray->client.sin_addr), inet_ntoa(peerArray->peer.sin_addr));

	printf("Server Closed.\n");
	return 0;
}

void verifyInput(int argc, char **argv, PortNT *PN)
{
  if (3 > argc) 
  {
      printf(USAGE);
      exit(EXIT_FAILURE);
  }

	int port, temp;
  port = strtol(argv[1], NULL, 0);
  if (1024 > port || 65535 < port) 
  {
    fprintf(stderr,RED"Input Error: "RESET "Port must be in the range [1024, 65535])\n");
    printf(USAGE);
    exit(EXIT_FAILURE);
  }

  temp = atoi(argv[2]);
  PN->numClients = temp;
  PN->port = port;
}

/* Creates a new PeerT variable */
PeerT *new_parray(PortNT *PN) 
{
  PeerT *PT;

  PT = malloc(PN->numClients*sizeof(PeerT));
  if(PT == NULL) 
  {
      fprintf(stderr, "Memory allocation failed!\n");
      exit(EXIT_FAILURE);
  }
  return PT;
}

int createServer(PortNT *server)
{
	int status, sockfd;
	struct addrinfo hints, *serverInfo;
	char port[32];
	sprintf(port, "%d", server->port);

	memset((void *)&hints, 0, (size_t) sizeof(hints));
	hints.ai_family = AF_INET;    
	hints.ai_socktype = SOCK_DGRAM;  
	hints.ai_flags = AI_PASSIVE;     

	if ((status = getaddrinfo(NULL, port, &hints, &serverInfo)) != 0) 
	{
		fprintf(stderr, RED"Server Error: "RESET "getaddrinfo() error = [%s]\n", gai_strerror(status));
		return ERROR;
	}

	// Bind
	if ((sockfd = bindSocket(serverInfo)) < 0)
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
	if (_sa->sa_family == AF_INET) 
	{
		return (((struct sockaddr_in*)_sa)->sin_port);
	}

	return (((struct sockaddr_in6*)_sa)->sin6_port);
}

void runServer(int _sockfd, PeerT *_peerArray, PortNT *_server)
{
	char buffer[BUFFER_SIZE];
	char ipAddress[INET_ADDRSTRLEN];
	socklen_t addr_len;
	int numBytes = 0;

	int i = 0, j = 0;
	for(i = 0; i < _server->numClients; i++)
	{
		printf("Waiting for a connection...\n");
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
		printf(YELLOW"\nA host from %s:%d has connected with:"RESET"\t%s\t\n", ipAddress, _peerArray[i].client.sin_port, buffer);
	}

	printf("Sending Token Ring Position to Clients \n");
	size_t len;
  for (i = 0; i < _server->numClients; i++)
  {
      j = (i + 1) % _server->numClients;
      memcpy(&_peerArray[i].peer, &_peerArray[j].client, sizeof (struct sockaddr_in));
      len = sendto(_sockfd, &_peerArray[i], sizeof (PeerT), 0,
              (struct sockaddr *) &_peerArray[i].client, sizeof (struct sockaddr_in));
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
