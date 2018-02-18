#ifndef BBPEER_H_   
#define BBPEER_H_

#include <netinet/in.h>

int createClientSocket(char *hostName, int port, struct sockaddr_in *dest);

int bindClientSocket(int sockfd, int port);

#endif // BBPEER_H_