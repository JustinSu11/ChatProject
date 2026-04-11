#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <malloc.h>
#include "utils.h"



int main(){
 
    int client_socket = createTCPIpv4Socket(); // Create a TCP socket for IPv4 communication

    const char* ip = "127.0.0.1"; // Example IP address to connect to
    struct sockaddr_in *address = createIPv4Address(ip,80); // Create address structure for the server

    struct timeval timeout = {5, 0}; // Set timeout to 5 seconds for both send and receive operations
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


    char* message = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";

    
    connectToServer(client_socket, address);
    send(client_socket, message, strlen(message), 0);



    char buffer[1024] = {0};
    int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);

    if (bytes > 0)
        printf("Response from server:\n%s\n", buffer);

    close(client_socket); 
    return 0;
}