#ifndef BBSERVER_H_   
#define BBSERVER_H_

#include <netinet/in.h>

/**
 *	Contains a bbpeer information.
 *	client	 - A bbpeer's address information for connecting to.
 *	neighbor - The address information of client's neighbor in the ring.
 */
typedef struct bbpeer_info 
{
    struct sockaddr_in client;
    struct sockaddr_in neighbor;
} PeerInfo;

/**
 *	Contains information related to the server.
 *	numClients - Number of clients to create the ring with.
 *	port	   - The port the server is hosting on.
 */
typedef struct bbserver_info 
{
    int numClients;
    int port;
} ServerInfo;

/**
 *	Creates an array containing PeerInfo structs.
 *
 *	@param  serverInfo - structure of server information (number of peers, port)
 *	@return			   - an array of bbpeers.
 */
PeerInfo *createPeerArray(ServerInfo *serverInfo);

/**
*	Checks the arguments that the program was ran with.
*	If it's valid we continue, if not we throw an error.
*
*	@param argc - Number of arguments passed in.
*	@param argv - Array containing the arguments.
*/
void verifyInput(int argc, char **argv, ServerInfo *server);

/**
 *	Hosts the bbserver on the given port.
 *
 *	@param  server - Structure containing server information.
 *	@return        - socket file descriptor
 */
int createServer(ServerInfo *server);

/**
 *	Given a sockaddr this function returns it's port.
 *
 *	@param  sa	- sockaddr to get the port from.
 *	@return     - an address port.
 */
in_port_t getPort(struct sockaddr *sa);

/**
 *	Runs a loop and waits for connections to the server. As peers connect to the server
 *	it notifies them of their neighbor and establishes the token ring.
 *
 *	@param sockfd    - Server socket file descriptor.
 *	@param peerArray - Array containing information of peers in the ring.
 *	@param server    - Contains information about the server.
 */
void runServer(int sockfd, PeerInfo *peerArray, ServerInfo *server);

/**
 *	Binds a socket given an addrinfo structure and returns a file descriptor.
 *	@param  res - addrinfo to bind
 *	@return     - Server socket file descriptor.
 */
int bindSocket(struct addrinfo *res);

#endif // BBSERVER_H_