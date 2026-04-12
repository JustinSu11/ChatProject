//Server Implementation
#define _POSIX_C_SOURCE 200809L
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
#include <pthread.h>





int main(){

    // Create a TCP socket for IPv4 communication
    int server_socket = createTCPIpv4Socket(); 

    struct sockaddr_in* address = createIPv4Address("0.0.0.0", 8080);


    // bind() — attach it to an IP + port
    bind(server_socket, (struct sockaddr*)address, sizeof(*address)); 

    // Server accepting incoming connections with a backlog of 10
    listen(server_socket, 10);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Wait for a client to connect and accept the connection
    int client_fd = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len); 

    char buffer[1024];

    
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    int client_port = ntohs(client_addr.sin_port);
    printf("Accepted connection from %s:%d\n", ip_str, client_port);

    while (1) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("Client says: %s\n", buffer);
            // continue to receive more messages from this client
        } 
        if (bytes == 0) {
            // client closed connection 
            printf("Client closed connection\n");
            break;
        } else {
            perror("recv");
            break;
        }
    }


    close(client_fd);
    close(server_socket);

   

}
