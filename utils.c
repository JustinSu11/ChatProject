#include "utils.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Allocates memory for a sockaddr_in structure and returns address
struct sockaddr_in* createIPv4Address(const char *ip, int port) {
    
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET; // IPv4
    address->sin_port = htons(port); // Convert port to network byte order
    inet_pton(AF_INET, ip, &address->sin_addr); // Convert IP address from text to binary form
    return address; 
}


int createTCPIpv4Socket() {
    //Create a socket with the following parameters: Domain: AF_INET (IPv4), 
    // Type: SOCK_STREAM (TCP), Protocol: 0 (default)
    return socket(AF_INET, SOCK_STREAM, 0);
}
