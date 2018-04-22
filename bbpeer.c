#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// Project
#include "bbpeer.h"

// Defines
#define ERROR -1
#define SUCCESS 0

#define BUFFER_SIZE 256

#define RED "\x1b[31m"
#define BLUE   "\x1B[34m"
#define YELLOW   "\x1B[33m"
#define RESET "\x1B[0m"


static ClientData ring;
struct addrinfo *server;
static const uint32_t TOKEN = 0;

static pthread_t token_Thread;   

int main(int argc, char **argv) 
{
	char buffer[BUFFER_SIZE];

	if (argc != 3)
	{
		fprintf(stderr, RED"Input Error: "RESET "Anticipated input -->"
				"./bbpeer <Server Hostname> <Port>\n");
		exit(EXIT_FAILURE);
	}
	printf(BLUE "Debug:"RESET " Connecting to [%s].\n", argv[1]);
	printf(BLUE "Debug:"RESET " Port # [%s].\n", argv[2]);

	// Create client socket
	int sockfd;
	struct sockaddr_in destination;
	sockfd = createClientSocket(argv[1], atoi(argv[2]), &destination);
	printf(BLUE "Debug:"RESET " Successfully created client socket. \n");

	// Bind the Socket
	if (bindClientSocket(sockfd, 0) == ERROR)
		return ERROR;

	// Gather peer info
	struct sockaddr_in peer;
	requestpeer(sockfd, server->ai_addr);

	// Determine who gets the token first
  handshake(sockfd);

  // Start token passing thread.
  pthread_create(&token_Thread, NULL, tokenPassing_Thread, sockfd);

  // Display bulletin options
  displayMenu();  

	// Join the server
	// ssize_t numBytesSent;
	// numBytesSent = sendto(sockfd, "TOKEN", sizeof "TOKEN", 0, (struct sockaddr *)&destination, sizeof(struct sockaddr_in));
	// if (numBytesSent < 0)
	// {
	// 	perror(RED"Client-to-Server Error: "RESET "sendto() - request to join server failed.\n");
	// 	return ERROR;
	// }
	// printf(BLUE "Debug:"RESET " Successfully sendto() server. \n");

	// // Receive server response
	// bzero(buffer, BUFFER_SIZE);
	// ssize_t numBytesReceived;
	// socklen_t clientlen;
	// clientlen = sizeof(struct sockaddr_in);
	// numBytesReceived = recvfrom(sockfd, buffer, INET6_ADDRSTRLEN, 0, (struct sockaddr *)&destination, &clientlen);
	// if (numBytesReceived < 0)
	// {
	// 	perror(RED"Client-to-Server Error: "RESET "recvfrom() - response from server failed.\n");
	// 	return ERROR;
	// }
	// printf(BLUE "Debug:"RESET " Successfully recvfrom() server. \n");

	printf("Debug: BBPeer disconnected.\n");
	return 0;
}

int createClientSocket(char *_hostName, int _port, struct sockaddr_in *_dest)
{
	struct addrinfo hints;
	struct addrinfo *p;
	char portBuffer[6];

	// Clear socket
	memset(_dest, 0, sizeof(struct sockaddr_in));
	_dest->sin_port = htons((uint16_t)_port);
	_dest->sin_family = AF_INET;
	
	// Get the host information.
	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	snprintf(portBuffer, sizeof(portBuffer), "%i", _port);
	if (getaddrinfo(_hostName, portBuffer, &hints, &server) != 0)
	{
		perror(RED"Client Error: "RESET "getaddrinfo() - failed to get host information.\n");
		return ERROR;
	}

	// Connect
	int sockfd = ERROR;
	for (p = server; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd > 0)
		{
			memcpy(_dest, p->ai_addr, p->ai_addrlen);
			break;
		}
		sockfd = ERROR;
	}

	// freeaddrinfo(server);
	return sockfd;
}

int bindClientSocket(int _sockfd, int _port)
{
	struct sockaddr_in addr;
	memset((char *)&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(_port);

	if (bind(_sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1 )
	{
		perror(RED"Client Error: "RESET "bind() - Failed to bind client socket.\n");
		return ERROR;
	}

	return SUCCESS;
}

void requestpeer(int _sockfd, const struct sockaddr *_server)
{
    const char message[] = "connect";
    int len;

    printf("Waiting for the token ring to form...\n");

    /* send the request */
    len = sendto(_sockfd, message, strlen(message), 0,
                 _server, sizeof (struct sockaddr_in));
    if (strlen(message) != len)
    {
    	perror(RED"Error: "RESET "requestpeer() - Invalid message length.\n");
    	exit(EXIT_FAILURE);
    }
    	
        
    /* handle the response */
    recvfrom(_sockfd, &ring, sizeof ring, 0, NULL, 0);
    puts("Received peer from the server. Negotiating first token holder...");

    /* pause so that all peers have time to get the server message */
    sleep(1);
}

void handshake(int _sockfd)
{
	struct sockaddr_in peer;
  ssize_t len;
  int comparison;

  // Send address to the peer 
  sendto(_sockfd, &ring.client, sizeof ring.client, 0,
         (struct sockaddr *) &ring.peer, sizeof ring.peer);

  /*
   * Repeat the following until we decide to start the token or we get the
   * token from a peer.
   */
  while (1) 
  {
    // Receive a peer address for comparison to our own.
    len = recvfrom(_sockfd, &peer, sizeof peer, 0, NULL, 0);

    // Token received
    if (sizeof (uint32_t) == len)
      break;
    else if (sizeof (peer) != len)
    {
    	perror(RED"Error: "RESET "requestpeer() - Invalid message length.\n");
    	exit(EXIT_FAILURE);
    }

    /*
     * We received a peer address the must be compared to our own. If it is
     * our local address, then we had the lowest value and get the token
     * first. Otherwise, we forward any addresses lower than our own.
     */
    comparison = compare(&ring.client, &peer);
    if (0 == comparison) 
    {
      puts("Initial possession of the token\n");
      break;
    } else if (0 < comparison) 
    {
      puts("Forwarding a lower peer address...");
      sendto(_sockfd, &peer, sizeof peer, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
    } 
  }
}

int compare(struct sockaddr_in *left, struct sockaddr_in *right)
{
    if (left->sin_addr.s_addr < right->sin_addr.s_addr) 
    {
      return -1;
    } 
    else if (left->sin_addr.s_addr == right->sin_addr.s_addr) 
    {
      if (left->sin_port < right->sin_port)
        return -1;
      else if (left->sin_port == right->sin_port)
        return 0;
      else
        return 1;
    }

    return 1;
}

void * tokenPassing_Thread(void *_sockfd)
{
  ClientData peer;
  ssize_t len;
  int sockfd = _sockfd;

  // Pass the token around 
  sendto(sockfd, &TOKEN, sizeof TOKEN, 0,
         (struct sockaddr *) &ring.peer, sizeof ring.peer);

  while (1) 
  {
    // Wait for token
    len = recvfrom(sockfd, &peer, sizeof peer, 0, NULL, 0);

    // Check if client is requesting a leave by sending its address
    // or if it's passing the token.
    if (sizeof (ClientData) == len) 
      handlepeerexit(&peer);
    else if (sizeof TOKEN == len) 
      sendto(sockfd, &TOKEN, sizeof TOKEN, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  }

  // Leave request
  sendto(sockfd, &ring, sizeof ring, 0,
         (struct sockaddr *) &ring.peer, sizeof ring.peer);

  pthread_exit(EXIT_SUCCESS);
}

void displayMenu()
{
	const char menu[] =
        "Menu\n"
        "\n"
        "1. write\n"
        "2. read#\n"
        "3. exit\n"
        "\n"
        "Selection (1-3): ";


  while (true) 
  {
    fputs(MENU, stdout);
    fflush(stdout);

    switch (getch()) {
    case '1':
        break;

    case '2':
        break;

    case '3':
        return;

    default:
        break;
    }
  }   

}