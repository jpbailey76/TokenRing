#ifndef BBPEER_H_   
#define BBPEER_H_

#include <netinet/in.h>

typedef struct ClientData {
    struct sockaddr_in client;
    struct sockaddr_in peer;
} ClientData;


int createClientSocket(char *hostName, int port, struct sockaddr_in *dest);

int bindClientSocket(int sockfd, int port);

void requestpeer(struct sockaddr_in *peer, const struct sockaddr *server)

#endif // BBPEER_H_