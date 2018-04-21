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

	printf("DEBUG: numPeers = [%d]\tport = [%d]\n", server.numClients, server.port);

	if ((sockfd = createServer(&server)) == ERROR)
		exit(EXIT_FAILURE);

	// Create peer array and run server.
  PeerT *peerArray = new_parray(&server);
	runServer(sockfd, peerArray, &server);

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

	memset((void *)&hints, 0, (size_t) sizeof(hints)); // make sure the struct is empty
	hints.ai_family = AF_INET;     // IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM;  // UDP sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if ((status = getaddrinfo(NULL, port, &hints, &serverInfo)) != 0) 
	{
		fprintf(stderr, RED"Server Error: "RESET "getaddrinfo() error = [%s]\n", gai_strerror(status));
		return ERROR;
	}

	// Make the socket
	/*sockfd = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);
	if (sockfd == -1)
	{
		perror(RED"Server Error: "RESET "Socket creation failed.\n");
		return ERROR;
	}*/

	// Bind
	if ((sockfd = bindSocket(serverInfo)) < 0)
	{
		perror("Error: Failed to bind to socket :");
		exit(EXIT_FAILURE);
	}

	//if (bind(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) == -1)
	//{
	//	close(sockfd);
	//	perror(RED"Server Error: "RESET "Failed to bind socket.\n");
	//	return ERROR;
	//}

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

void runServer(int _sockfd, PeerT *_peerArray, PortNT *_server)
{
	char buffer[BUFFER_SIZE];
	char responseBuff[BUFFER_SIZE];
	char ipBuffer[INET6_ADDRSTRLEN];
	struct sockaddr_storage clientAddr[_server->numClients];
	socklen_t addr_len;
	int numBytes = 0;

	int i = 0, j = 0;
	for(i = 0; i < _server->numClients; i++)
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

	for (i = 0; i < _server->numClients; i++)
	{
		sendto(_sockfd, responseBuff, strlen(responseBuff), 0, (struct sockaddr*)&clientAddr[i], sizeof(struct sockaddr));
		printf("Sending response to a host.\n ");
	}


	printf("Sending Token Ring Position to Clients \n");
	size_t len;
  for (i = 0; i < _server->numClients; i++)
  {
      j = (i + 1) % _server->numClients;
      memcpy(&_peerArray[i].peer, &_peerArray[j].client, sizeof (struct sockaddr_in));
      printf("%08X:%d\t%08X:%d\n\n",
             _peerArray[i].client.sin_addr.s_addr,
             _peerArray[i].client.sin_port,
             _peerArray[i].peer.sin_addr.s_addr,
             _peerArray[i].peer.sin_port);
      len = sendto(_sockfd, &_peerArray[i], sizeof (PeerT), 0,
              (struct sockaddr *) &_peerArray[i].client, sizeof (struct sockaddr_in));
      if (7 != len)
          perror("Error: Message length invalid\n");
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
