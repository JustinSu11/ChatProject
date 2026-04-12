#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>

struct sockaddr_in* createIPv4Address(const char* ip, int port);
int createTCPIpv4Socket();


int connectToServer(int socket, struct sockaddr_in* address);

typedef enum { ROUTE_NONE, ROUTE_REPLY, ROUTE_BROADCAST } route_type_t;
typedef struct { route_type_t type; char *payload; size_t len; } route_result_t;
route_result_t route_message(const char *msg, int from_fd);
void free_route_result(route_result_t *r);



#endif // UTILS_H