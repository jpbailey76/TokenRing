#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
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

// Error Handling
#define ERROR -1
#define SUCCESS 0
#define BUFFER_SIZE 256

// Output Styling
#define RED "\x1b[31m"
#define BLUE   "\x1B[34m"
#define YELLOW   "\x1B[33m"
#define RESET "\x1B[0m"

// Server ring struct
ClientData ring;
int sockfd;

// Token & token thread
const uint32_t TOKEN = 0;
pthread_t token_Thread;   

/*
 * token_Mutex: 			used to block critical acces to the TOKEN variable.
 * menu_Access:				used to notify threads that the menu has gained access 
 * 										to critical data. (TOKEN)
 * tokenRing_Access:	used to notify threads that the token ring has gained access 
 * 										to critical data. (TOKEN)
 * 										
 * tokenReady:		boolean status var to notify if the token is ready to be used(no one is using it).
 * tokenNeeded:		boolean status var to store the state of if we need the token or not. 					
 */
pthread_mutex_t token_Mutex;
pthread_cond_t menu_Access;
pthread_cond_t tokenRing_Access;
bool tokenReady;
bool tokenNeeded;
bool connectedToRing;

// Bulletin Board File
const char *BULLETIN_BOARD;

// Debug flag
const bool DEBUG = true;

int main(int argc, char **argv) 
{
	// Server info
	struct addrinfo *server;

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
  connectedToRing = true;

  // Start token passing thread.
  tokenReady = true;
  tokenNeeded = false;
  pthread_mutex_init(&token_Mutex, NULL);
  pthread_cond_init(&menu_Access, NULL);
  pthread_cond_init(&tokenRing_Access, NULL);
  int threadStatus = -1;
  threadStatus = pthread_create(&token_Thread, NULL, tokenPassing_Thread, NULL);
  if(threadStatus != 0)
  {
  	printf(RED"ERROR: "RESET
  					"main() - Token thread failed to create!\n");
  	return ERROR;
  }

  // Display bulletin options
  displayMenu();  

  if(DEBUG)
  {
  	printf(BLUE"Debug: "RESET
  			 "BBPeer disconnected.\n");
  }

  // Cleanup all data
  cleanup();

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

  printf(YELLOW"Waiting for everyone to join. . .\n"RESET);

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
  printf(YELLOW"You have connected!\n"RESET);

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
  while(1) 
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

    // Determine who has the token first based off of their
    // IP Address + Port combination
    comparison = compare(&ring.client, &peer);
    if (comparison == 0) 
    {
      printf("You are the first token handler. . .\n");
      break;
    } 
    else if (comparison > 0) 
    {
      sendto(sockfd, &peer, sizeof peer, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
    } 
  }
}

int compare(struct sockaddr_in *firstPeer, struct sockaddr_in *secondPeer)
{
	// First peer less than second
  if (firstPeer->sin_addr.s_addr < secondPeer->sin_addr.s_addr) 
  {
    return -1;
  } 
  // Peers have the same address
  else if (firstPeer->sin_addr.s_addr == secondPeer->sin_addr.s_addr) 
  {
  	// First peer has the lower port.
    if (firstPeer->sin_port < secondPeer->sin_port)
    {
      return -1;
    }
    // Ports are the same, thus they are the same address AND port
    else if (firstPeer->sin_port == secondPeer->sin_port)
    {
      return 0;
    }
    // Right peer has the lower port.
    else
    {
      return 1;
    }
  }

  return 1;
}

void * tokenPassing_Thread(void *arg)
{
  ClientData peer;
  ssize_t len;

  // We're gaining the token, so signal to the menu(main)
  // thread that we've acquired it.
  pthread_mutex_lock(&token_Mutex); 
	tokenReady = true;
	pthread_cond_signal(&menu_Access);

	// We wait for a signal to access the token. 
	while (tokenNeeded)
	{
		pthread_cond_wait(&tokenRing_Access, &token_Mutex);
	}

	// Token is acquired
	tokenReady = false;
	pthread_mutex_unlock(&token_Mutex);
  
  // Pass the token around
  sendto(sockfd, &TOKEN, sizeof TOKEN, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  while (connectedToRing) 
  {
    // Wait for token
    len = recvfrom(sockfd, &peer, sizeof peer, 0, NULL, 0);

    // Check if client is requesting a leave by sending its address
    // or if it's passing the token.
    if (len == sizeof (ClientData)) 
    {
      peerExit(&peer);
    }
    else if (len == sizeof TOKEN) 
    {
    	// It's our turn to take the token so go ahead
    	// and take it...
			pthread_mutex_lock(&token_Mutex); 
			tokenReady = true;
			pthread_cond_signal(&menu_Access);
			while (tokenNeeded)
			{
				pthread_cond_wait(&tokenRing_Access, &token_Mutex);
			}
			tokenReady = false;
			pthread_mutex_unlock(&token_Mutex);

			// We're done using the token so we pass it on.
      sendto(sockfd, &TOKEN, sizeof TOKEN, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
    }
  }

  // Leave the ring.
  printf(YELLOW"You've been disconnected from the ring.\n"RESET);
  sendto(sockfd, &ring, sizeof ring, 0,(struct sockaddr *) &ring.peer, sizeof ring.peer);
  pthread_exit(SUCCESS);
}

void peerExit(ClientData *_request)
{
  // Our neighbor is leaving, so send to the peer after them.
  if (0 == compare(&_request->client, &ring.peer)) 
  {
    // Swap our neighbor with their neighbor and pass the token
    // to the new neighbor.
    memcpy(&ring.peer, &_request->peer, sizeof ring.peer);
    sendto(sockfd, &TOKEN, sizeof TOKEN, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  } 
  else 
  {
  	// Pass around the leave request.
    sendto(sockfd, _request, sizeof *_request, 0, (struct sockaddr *) &ring.peer, sizeof ring.peer);
  }
	printf(YELLOW"\nA peer has disconnected!\n"RESET);
}

void displayMenu()
{
	const char menu[] =
        YELLOW"\n\tMenu"RESET"\n"
        YELLOW"===================="RESET
        "\n"
        YELLOW"1."RESET " Write to the board. \n"
        YELLOW"2."RESET " Read a message from the board.\n"
        YELLOW"3."RESET " Show all messages.\n"
        YELLOW"4."RESET " Exit\n"
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
			{
		  	writeToBulletin();
			}
			else if(strcmp(inputTok, "2") == 0)
			{
		    readFromBulletin();
			}
			else if(strcmp(inputTok, "3") == 0)	
			{
				printAllFromBulletin();
			}
			else if(strcmp(inputTok, "4") == 0)	
			{
				exitRing();
				return;
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
  if(DEBUG)
  {
  	printf(BLUE"DEBUG: "RESET
  				 "Number of messages before = [%d]\n", messageNumber);
  }
  if (messageNumber <= 0)
  {
  	messageNumber = 1;
  }

  if(DEBUG)
  {
  	printf(BLUE"DEBUG: "RESET
  				 "Number of messages after = [%d]\n", messageNumber);
  }

  const char headerSpecifier[] = "%s%d: %s";
	const char messageHeader[] = "Message #"; 

	// We need the token before we can access the board.
	pthread_mutex_lock(&token_Mutex);
  tokenNeeded = true;
  while (!tokenReady)
  {
    printf(YELLOW"Waiting. The token is in use.\n");

  	// Wait for menu access.
    pthread_cond_wait(&menu_Access, &token_Mutex);
  }
  printf(YELLOW"Token obtained!\n");

  // We've gained access so open the board.
	FILE *fp;
	fp = fopen(BULLETIN_BOARD, "a");
	if(fp == NULL)
	{
		printf(RED"Error: "RESET
				"Unable to open file. It's likely not been created yet. Go ahead and"
				" write a message to it and try again!\n");

		// Done using the token.
		tokenNeeded = false;
		pthread_cond_signal(&tokenRing_Access);
		pthread_mutex_unlock(&token_Mutex);
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

		// Done using the token.
		tokenNeeded = false;
		pthread_cond_signal(&tokenRing_Access);
		pthread_mutex_unlock(&token_Mutex);
		return SUCCESS;
	}

	return ERROR;
}

int getNumMessages()
{
	int messageNumber = 1;
	char ch;

	// We need the token before we can access the board.
	pthread_mutex_lock(&token_Mutex);
  tokenNeeded = true;
  while (!tokenReady)
  {
    printf(YELLOW"Waiting. The token is in use.\n");

  	// Wait for menu access.
    pthread_cond_wait(&menu_Access, &token_Mutex);
  }
  printf(YELLOW"Token obtained!\n");

	// Open file and count messages
	FILE *fp;
	fp = fopen(BULLETIN_BOARD, "r");
	if(fp == NULL)
	{
		printf(YELLOW"Board "RESET "%s" YELLOW" has been created. You may now enter your message.\n"RESET, BULLETIN_BOARD);
		
		// Done using the token.
		tokenNeeded = false;
		pthread_cond_signal(&tokenRing_Access);
		pthread_mutex_unlock(&token_Mutex);
		return ERROR;
	}

  // Count Numlines
  for (ch = getc(fp); ch != EOF; ch = getc(fp))
  {
		if (ch == '\n') 
		{
		  messageNumber = messageNumber + 1;
		}
  }

	// Done using the token.
	fclose(fp);
	tokenNeeded = false;
  pthread_cond_signal(&tokenRing_Access);
  pthread_mutex_unlock(&token_Mutex);
	return messageNumber;
}

int readFromBulletin()
{
  char input[256];
	int numMessages = getNumMessages() - 1;
	if(DEBUG)
	{
  	printf(BLUE"DEBUG: "RESET
  				 "Number of messages before = [%d]\n", numMessages);
	}
  if (numMessages < 0)
  {
  	printf(RED"Error: "RESET
					"No messages to view yet. Go ahead and add one!\n");
  	return ERROR;
  }
  if(DEBUG)
  {
  	printf(BLUE"DEBUG: "RESET
  				 "Number of messages after = [%d]\n", numMessages);
  }


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

	// We need the token before we can access the board.
	pthread_mutex_lock(&token_Mutex);
  tokenNeeded = true;
  while (!tokenReady)
  {
    printf(YELLOW"Waiting. The token is in use.\n");

  	// Wait for menu access.
    pthread_cond_wait(&menu_Access, &token_Mutex);
  }
  printf(YELLOW"Token obtained!\n");

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
				"readFromBulletin() - Unable to open file. It's likely not been created yet. Go ahead and"
				" write a message to it and try again!\n");

		// Done using the token.
    tokenNeeded = false;
    pthread_cond_signal(&tokenRing_Access);
    pthread_mutex_unlock(&token_Mutex);
		return ERROR;
	}

	// Done using the token.
	tokenNeeded = false;
  pthread_cond_signal(&tokenRing_Access);
  pthread_mutex_unlock(&token_Mutex);
	return SUCCESS;
}

void exitRing()
{
	// We need the token before we can access the board.
	pthread_mutex_lock(&token_Mutex);
  tokenNeeded = true;
  while (!tokenReady)
  {
    printf(YELLOW"Waiting. The token is in use.\n");

  	// Wait for menu access.
    pthread_cond_wait(&menu_Access, &token_Mutex);
  }
  printf(YELLOW"Token obtained!\n");

  // Disconnect
  connectedToRing = false;

  // Done using the token.
	tokenNeeded = false;
  pthread_cond_signal(&tokenRing_Access);
  pthread_mutex_unlock(&token_Mutex);
}

void cleanup()
{
	// Release resources and handle threads.
  pthread_join(token_Thread, NULL);
  pthread_cond_destroy(&menu_Access);
  pthread_cond_destroy(&tokenRing_Access);
  pthread_mutex_destroy(&token_Mutex);
  close(sockfd);
}

int printAllFromBulletin()
{
	// We need the token before we can access the board.
	pthread_mutex_lock(&token_Mutex);
  tokenNeeded = true;
  while (!tokenReady)
  {
    printf(YELLOW"Waiting. The token is in use.\n");

  	// Wait for menu access.
    pthread_cond_wait(&menu_Access, &token_Mutex);
  }
  printf(YELLOW"Token obtained!\n");

	// Get message requested
	FILE *fp = fopen(BULLETIN_BOARD, "r");
	int count = 1;
	if(fp != NULL )
	{
	    fputs(fp, stdout);
	    fflush(stdout);
	    fclose(fp);
	}
	else
	{
		printf(RED"Error: "RESET
				"printAllFromBulletin() - Unable to open file.\n");

		// Done using the token.
    tokenNeeded = false;
    pthread_cond_signal(&tokenRing_Access);
    pthread_mutex_unlock(&token_Mutex);
		return ERROR;
	}

	// Done using the token.
	tokenNeeded = false;
  pthread_cond_signal(&tokenRing_Access);
  pthread_mutex_unlock(&token_Mutex);
	return SUCCESS;
}