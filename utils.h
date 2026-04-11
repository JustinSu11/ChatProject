#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>

struct sockaddr_in* createIPv4Address(const char* ip, int port);
int createTCPIpv4Socket();


int connectToServer(int socket, struct sockaddr_in* address);




#endif // UTILS_H