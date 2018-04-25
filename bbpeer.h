#ifndef BBPEER_H_   
#define BBPEER_H_

#include <netinet/in.h>

/**
 * 
 */
typedef struct TokenRing 
{
    struct sockaddr_in client;
    struct sockaddr_in neighbor;
} TokenRing;

/**
 * [verifyInput description]
 * @param argc [description]
 * @param argv [description]
 */
void verifyInput(int argc, char **argv);

/**
 * [getServerInfo description]
 * @param  addr [description]
 * @param  port [description]
 * @return      [description]
 */
struct addrinfo* getServerInfo(const char *addr, const char *port);

/**
 * [bindClientSocket description]
 * @param  sockfd [description]
 * @param  port   [description]
 * @return        [description]
 */
int bindClientSocket(int sockfd, int port);

/**
 * [requestPeer description]
 * @param server [description]
 */
void requestPeer(const struct sockaddr *server);

/**
 * [handshake description]
 */
void handshake();

/**
 * Compares two IP addresses 
 * @param  left  [description]
 * @param  right [description]
 * @return       [description]
 */
int compare(struct sockaddr_in *firstPeer, struct sockaddr_in *secondPeer);

/**
 * [tokenPassing_Thread description]
 * @param  arg [description]
 * @return     [description]
 */
void * tokenPassing_Thread(void *arg);

/**
 * [peerExit description]
 * @param request [description]
 */
void peerExit(TokenRing *request);

/**
 * [displayMenu description]
 */
void displayMenu();

/**
 * [writeToBulletin description]
 * @return [description]
 */
int writeToBulletin();

/**
 * [getNumMessages description]
 * @return [description]
 */
int getNumMessages();

/**
 * [readFromBulletin description]
 * @return [description]
 */
int readFromBulletin();

/**
 * [printAllFromBulletin description]
 * @return [description]
 */
int printAllFromBulletin();

/**
 * [exitRing description]
 */
void exitRing();

/**
 * [cleanup description]
 */
void cleanup();

#endif // BBPEER_H_