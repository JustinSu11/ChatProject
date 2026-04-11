//Server Implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "utils.h"
#include <malloc.h>
#include <string.h>


int main(){

    // Create a TCP socket for IPv4 communication
    int server_socket = createTCPIpv4Socket(); 

    struct sockaddr_in address = createIPv4Address("0.0.0.0", 8080);



    // bind() — attach it to an IP + port
    bind(server_socket, (struct sockaddr*)&address, sizeof(address)); 

    // Server accepting incoming connections with a backlog of 10
    listen(server_socket, 10);

    // Wait for a client to connect and accept the connection
    int client_fd = accept(server_socket, NULL, NULL); 

    char buffer[1024];



    // receive data from the client and store it in the buffer
    int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes > 0) {
        buffer[bytes] = '\0'; // null-terminate
        printf("Client says: %s\n", buffer);
    }

    char* response = "Hello from the server!"; // Generic response message 
    // send a response back to the client
    send(client_fd, response, strlen(response), 0); 

    close(client_fd);
    close(server_socket);

   

}
