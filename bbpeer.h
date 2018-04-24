#ifndef BBPEER_H_   
#define BBPEER_H_

#include <netinet/in.h>

typedef struct ClientData 
{
    struct sockaddr_in client;
    struct sockaddr_in peer;
} ClientData;


void verifyInput(int argc, char **argv);

struct addrinfo* getServerInfo(const char *addr, const char *port);

int bindClientSocket(int sockfd, int port);

void requestPeer(const struct sockaddr *server);

void handshake();

/**
 * Compares two IP addresses 
 * @param  left  [description]
 * @param  right [description]
 * @return       [description]
 */
int compare(struct sockaddr_in *firstPeer, struct sockaddr_in *secondPeer);

void * tokenPassing_Thread(void *arg);

void peerExit(ClientData *request);

void displayMenu();

int writeToBulletin();

int getNumMessages();

int readFromBulletin();

void exitRing();

void cleanup();

#endif // BBPEER_H_