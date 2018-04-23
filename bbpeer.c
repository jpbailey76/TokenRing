#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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


// Server ring struct
static ClientData ring;
static int sockfd;

// Token & token thread
static const uint32_t TOKEN = 0;
static pthread_t token_Thread;   

// Bulletin Board File
static const char *BULLETIN_BOARD;

// Debug flag
static const bool DEBUG = true;

int main(int argc, char **argv) 
{
	char buffer[BUFFER_SIZE];
	struct addrinfo *server;
	struct sockaddr_in peer;

	// Check the validity of the user input
	verifyInput(argc, argv);
	if(DEBUG)
	{
		printf(BLUE "Debug:"RESET " Connecting to [%s].\n", argv[1]);
		printf(BLUE "Debug:"RESET " Port # [%s].\n", argv[2]);
	}

	// Get server info and create the client socket
	server = getServerInfo(argv[1], argv[2]);
	sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
	if (sockfd < 0)	
	{
		fprintf(stderr, RED"Error: "RESET 
			"main() - Failed to create socket.\n");
    exit(EXIT_FAILURE);
	}
	if(DEBUG)
		printf(BLUE "Debug:"RESET " Successfully created client socket. \n");

	// Gather peer info
	requestPeer(server->ai_addr);

	// Determine who gets the token first
  handshake();

  // Start token passing thread.
  pthread_create(&token_Thread, NULL, tokenPassing_Thread, NULL);

  // Display bulletin options
  displayMenu();  

  if(DEBUG)
  {
  	printf(BLUE"Debug: "RESET
  			 "BBPeer disconnected.\n");
  }
	return 0;
}

void verifyInput(int argc, char **argv)
{
	if (argc < 4) 
	{
    fprintf(stderr, RED"Input Error: "RESET "Anticipated input -->"
				"./bbpeer <Server Hostname> <Port> <file>\n");
    exit(EXIT_FAILURE);
  }

  BULLETIN_BOARD = argv[3];
}

struct addrinfo* getServerInfo(const char *_address, const char *_port)
{
	int status;
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	status = getaddrinfo(_address, _port, &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, RED"Error: "RESET 
			"getServerInfo() - Failed to get address info.>\n");
    exit(EXIT_FAILURE);
	}
	return res;
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

void requestPeer(const struct sockaddr *_server)
{
  const char message[] = "connect";
  int len;

  printf("Waiting for the token ring to form...\n");

  // Request for a peer
  len = sendto(sockfd, message, strlen(message), 0,
               _server, sizeof (struct sockaddr_in));
  if (strlen(message) != len)
  {
  	perror(RED"Error: "RESET 
  		"requestpeer() - Invalid message length.\n");
  	exit(EXIT_FAILURE);
  }
      
  // Recieved a request 
  recvfrom(sockfd, &ring, sizeof ring, 0, NULL, 0);
  printf("Received peer from the server. Negotiating first token holder...\n");

  // Wait for all peers
  sleep(1);
  if(DEBUG)
  	printf(BLUE"Debug: "RESET"After sleep\n");
}

void handshake()
{
	struct sockaddr_in peer;
  ssize_t len;
  int comparison;

  // Send address to the peer 
  sendto(sockfd, &ring.client, sizeof ring.client, 0,
         (struct sockaddr *) &ring.peer, sizeof ring.peer);
  if(DEBUG)
  {
  	printf(BLUE"Debug: "RESET
  			 "Handshake sent.\n");
  }

  // Wait for a token, or pass it.
  while (1) 
  {
  	if(DEBUG)
  	{
  		printf(BLUE"Debug: "RESET
  			 "recvfrom() - sockfd = [%d].\n", sockfd);
  	}
  	
    // Receive a peer address for comparison to our own.
    len = recvfrom(sockfd, &peer, sizeof peer, 0, NULL, 0);

    if(DEBUG)
    {
    	printf(BLUE"Debug: "RESET
  			 "Token passing started.\n");
    }

    // Token received
    if (sizeof (uint32_t) == len)
    {
    	printf(YELLOW"Token recieved!\n"RESET);
      break;
    }
    else if (sizeof (peer) != len)
    {
    	perror(RED"Error: "RESET 
    		"handshake() - Invalid message length.\n");
    	exit(EXIT_FAILURE);
    }

    comparison = compare(&ring.client, &peer);
    if (comparison == 0) 
    {
      printf("Initial possession of the token\n");
      break;
    } 
    else if (comparison > 0) 
    {
      printf("Forwarding a lower peer address...\n");
      sendto(sockfd, &peer, sizeof peer, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
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

void * tokenPassing_Thread(void *arg)
{
  ClientData peer;
  ssize_t len;

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
      peerExit(sockfd, &peer);
    else if (sizeof TOKEN == len) 
      sendto(sockfd, &TOKEN, sizeof TOKEN, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  }

  // Leave request
  sendto(sockfd, &ring, sizeof ring, 0,
         (struct sockaddr *) &ring.peer, sizeof ring.peer);

  pthread_exit(EXIT_SUCCESS);
}

void peerExit(int _sockfd, ClientData *_request)
{
  // Our neighbor is leaving, so send to the peer after them.
  if (0 == compare(&_request->client, &ring.peer)) 
  {
    // Swap our neighbor with their neighbor and pass the token
    // to the new neighbor.
    memcpy(&ring.peer, &_request->peer, sizeof ring.peer);
    sendto(_sockfd, &TOKEN, sizeof TOKEN, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  } 
  else 
  {
  	// Pass around the leave request.
    sendto(_sockfd, _request, sizeof *_request, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  }
}

void displayMenu()
{
	const char menu[] =
        YELLOW"\n\tMenu"RESET"\n"
        YELLOW"===================="RESET
        "\n"
        YELLOW"1."RESET " Write\n"
        YELLOW"2."RESET " Read#\n"
        YELLOW"3."RESET " Exit\n"
        "\n"
        YELLOW"Selection: "RESET;
  char input[256];

  while (1) 
  {
    fputs(menu, stdout);
    fflush(stdout);

    if(fgets(input, sizeof input, stdin) != NULL)
		{
			char *inputTok = strtok(input, "\n");

			if(strcmp(inputTok, "1") == 0)
		  	writeToBulletin();
			else if(strcmp(inputTok, "2") == 0)
			{
		    readFromBulletin();
			}
			else if(strcmp(inputTok, "3") == 0)	
			{
				printf("Not added yet.\n");
				//exit();
			}
		}
  }   
}

int writeToBulletin()
{
	const char header[] =
        YELLOW"Enter Message: "RESET;
  char message[256];
  int messageNumber = getNumMessages();

  // Check if this is a new file, if so
  // assume this is the first message.
  if (messageNumber < 0)
  	messageNumber = 1;

  if(DEBUG)
  	printf(BLUE"DEBUG: "RESET
  				 "Number of messages = [%d]\n", messageNumber);

  const char headerSpecifier[] = "%s%d: %s";
	const char messageHeader[] = "Message #"; 

	FILE *fp;
	fp = fopen(BULLETIN_BOARD, "a");
	if(fp == NULL)
	{
		printf(RED"Error: "RESET
				"Unable to open file.\n");
		return ERROR;
	}

  fputs(header, stdout);
  fflush(stdout);
  if(fgets(message, sizeof message, stdin) != NULL)
	{
		char buffer[256];

		sprintf(buffer, headerSpecifier, messageHeader, messageNumber, message);
		fprintf(fp, buffer);
	  fclose(fp);

		printf(YELLOW"Message Added.\n"RESET);
		return SUCCESS;
	}

	return ERROR;
}

int getNumMessages()
{
	int messageNumber = 1;
	char ch;

	// Open file and count messages
	FILE *fp;
	fp = fopen(BULLETIN_BOARD, "r");
	if(fp == NULL)
	{
		printf(YELLOW"Board "RESET "%s" YELLOW" has been created. You may now enter your message.\n"RESET, BULLETIN_BOARD);
		return ERROR;
	}

  // Count Numlines
  for (ch = getc(fp); ch != EOF; ch = getc(fp))
      if (ch == '\n') 
          messageNumber = messageNumber + 1;

	fclose(fp);
	return messageNumber;
}

int readFromBulletin()
{
  char input[256];
	int numMessages = getNumMessages();
	if (numMessages < 0)
		numMessages = 1;

	// Display options
	const char header[] =
        YELLOW"\nWhich message would you like to view? (1 - %d): "RESET;
  char buffer[256];
  sprintf(buffer, header, numMessages);
	fputs(buffer, stdout);

	// Get message to display
	if(fgets(input, sizeof input, stdin) == NULL)
		return ERROR;

	int messageToView;
	messageToView = atoi(input);

	if(messageToView < 1 || numMessages < messageToView)
	{
		printf(RED"Error: "RESET
					"Invalid message to view. Range is (1 - %d)", numMessages);
		return ERROR;
	}

	// Get message requested
	FILE *fp = fopen(BULLETIN_BOARD, "r");
	int count = 1;
	if(fp != NULL )
	{
	    char buffer[256]; 
	    while (fgets(buffer, sizeof buffer, fp) != NULL)
	    {
	        if(count == messageToView)
	        {
	            printf(YELLOW"Message selected:\n"RESET);
	            printf("%s\n", buffer);
	            break;
	        }
	        else
	        {
	            count++;
	        }
	    }
	    fclose(fp);
	}
	else
	{
    printf(RED"Error: "RESET
				"readFromBulletin() - Unable to open file.\n");
		return ERROR;
	}

	return SUCCESS;
}
