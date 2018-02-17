#ifndef BBSERVER_H_   
#define BBSERVER_H_

#include <netinet/in.h>

int createServer();

void *get_in_addr(struct sockaddr *sa);

in_port_t getPort(struct sockaddr *sa);

void runServer(int sockfd, int numHosts);


#endif // BBSERVER_H_