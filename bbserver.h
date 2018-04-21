#ifndef BBSERVER_H_   
#define BBSERVER_H_

#include <netinet/in.h>

/* Struct that stores neighbor information */
typedef struct bbpeer_info {
    struct sockaddr_in client;
    struct sockaddr_in peer;
} PeerT;

/* Struct to store server port and number of clients*/ 
typedef struct port_N {
    int N;
    int port;
} PortNT;

void verifyInput(int argc, char **argv, PortNT *PN);

int createServer();

void *get_in_addr(struct sockaddr *sa);

in_port_t getPort(struct sockaddr *sa);

void runServer(int sockfd, int numHosts);

int bindSocket(struct addrinfo *res);


#endif // BBSERVER_H_