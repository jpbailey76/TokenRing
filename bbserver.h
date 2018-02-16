#ifndef BBSERVER_H_   
#define BBSERVER_H_

#include <netinet/in.h>

int createServer();

in_port_t getPort(struct sockaddr *sa);


#endif // BBSERVER_H_