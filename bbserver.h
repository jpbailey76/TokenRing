#ifndef BBSERVER_H_   
#define BBSERVER_H_

#include <netinet/in.h>

/* Struct that stores neighbor information */
/**
 * 
 */
typedef struct bbpeer_info 
{
    struct sockaddr_in client;
    struct sockaddr_in peer;
} PeerT;

/* Struct to store server port and number of clients*/ 
/**
 * 
 */
typedef struct port_N 
{
    int numClients;
    int port;
} PortNT;

/**
 * [new_parray description]
 * @param  PN [description]
 * @return    [description]
 */
PeerT *new_parray(PortNT *PN);

/**
 * [verifyInput description]
 * @param argc   [description]
 * @param argv   [description]
 * @param server [description]
 */
void verifyInput(int argc, char **argv, PortNT *server);

/**
 * [createServer description]
 * @param  server [description]
 * @return        [description]
 */
int createServer(PortNT *server);

/**
 * [getPort description]
 * @param  sa [description]
 * @return    [description]
 */
in_port_t getPort(struct sockaddr *sa);

/**
 * [runServer description]
 * @param sockfd    [description]
 * @param peerArray [description]
 * @param server    [description]
 */
void runServer(int sockfd, PeerT *peerArray, PortNT *server);

/**
 * [bindSocket description]
 * @param  res [description]
 * @return     [description]
 */
int bindSocket(struct addrinfo *res);

#endif // BBSERVER_H_