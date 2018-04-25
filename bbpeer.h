#ifndef BBPEER_H_   
#define BBPEER_H_

#include <netinet/in.h>

/**
 *	Structure containing the clients info,
*	as well as it's neighbors info.
 */
typedef struct TokenRing 
{
    struct sockaddr_in client;
    struct sockaddr_in neighbor;
} TokenRing;

/**
 *	Checks the arguments that the program was ran with.
 *	If it's valid we continue, if not we throw an error.
 *
 *	@param argc - Number of arguments passed in.
 *	@param argv - Array containing the arguments.
 */
void verifyInput(int argc, char **argv);

/**
 *	Given an address and port, this function returns the addrinfo 
 *	associated with that IP.
 *
 *	@param  ipAddress	- The IP Address.	
 *	@param  port			- The port.	
 *	@return addrinfo     - Address information for the given ipAddress:port combo.
 */
struct addrinfo* getServerInfo(const char *ipAddress, const char *port);

/**
 *	Sends a 'connect' message to the server and requests for neighbor.
 *
 *	@param server - structure containing the server address, port, and address family.
 */
void waitForNeighbor(const struct sockaddr *server);

/**
 *	Loops through the ring and sends peer addresses to all peers.
 *	Then determines which peer gets the token first by finding the peer
 *	with the lowest address:port combo.
 */
void handshake();

/**
 *	Given two addresses this function compares them and returns how they compare
 *	to one another.
 *
 *	@param  firstPeer  - Peer to compare.
 *	@param  secondPeer - Peer to compare.
 *	@return -1	 - firstPeer has the lower address:port.
 *			 0	 - Both peers have the same address:port.
 *			 1	 - secondPeer has the lower address:port.
 */
int compare(struct sockaddr_in *firstPeer, struct sockaddr_in *secondPeer);

/**
 *	Threaded function that runs the event loop of the token being passed around.
 *	While still connected, peers of the token ring will pass the TOKEN to their neighbor continuously.
 *	If a peer decides to leave the token ring they will send their TokenRing struct as a leave request.
 */
void * tokenPassing_Thread(void *arg);

/**
 *	This function takes a TokenRing member, swaps their position in the ring, and passes the token on.
 *
 *	@param request - Contains the peer leaving and their neighbor.
 */
void peerExit(TokenRing *request);

/**
 *	Displays the menu which contains the options available to users.
 */
void displayMenu();

/**
 *	Displays the menu options for writing to the BULLETIN_BOARD, takes the user's input as a message
 *	and stores their message into the BULLETIN_BOARD (defined by arg[3])
 *
 *	@return SUCCESS - File was successfully opened and the message was added to the board.
 *			ERROR	- File failed to open.
 */
int writeToBulletin();

/**
 *	Parses the BULLETIN_BOARD and counts the number of messages it contains.
 *
 *	@return The number of messages in the BULLETIN_BOARD (defined by arg[3])
 */
int getNumMessages();

/**
 *	Displays the menu options for reading from the BULLETIN_BOARD, takes the user's input
 *	on which message number to display and then displays that message.
 *
 *	@return SUCCESS - File was successfully opened and the message displayed.
 *			ERROR	- File failed to open.
 */
int readFromBulletin();

/**
 *	Displays all the messages that are contained in the BULLETIN_BOARD.
 *
 *	@return SUCCESS - File was successfully opened and the messages were displayed.
 *			ERROR	- File failed to open.
 */
int printAllFromBulletin();

/**
 *	Waits for the peer to receive the token and then sets it's connection status to false,
 *	which in turn causes the token passing thread to die.
 */
void exitRing();

/**
 *	Cleans up any data structures used.
 */
void cleanup();

#endif // BBPEER_H_