#ifndef BBPEER_H_   
#define BBPEER_H_

#include <netinet/in.h>

typedef struct ClientData 
{
    struct sockaddr_in client;
    struct sockaddr_in peer;
} ClientData;

int createClientSocket(char *hostName, int port, struct sockaddr_in *dest);

int bindClientSocket(int sockfd, int port);

void requestpeer(int sockfd, const struct sockaddr *server);

void handshake(int sockfd);

/**
 * Compares two IP addresses 
 * @param  left  [description]
 * @param  right [description]
 * @return       [description]
 */
int compare(struct sockaddr_in *left, struct sockaddr_in *right);

#endif // BBPEER_H_