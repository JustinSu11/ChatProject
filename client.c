#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "utils.h"
#include <stdbool.h>




int main(){
    
    char buffer[1024] = {0};
    int client_socket = createTCPIpv4Socket(); // Create a TCP socket for IPv4 communication

    const char* ip = "127.0.0.1"; // Example IP address to connect to
    struct sockaddr_in *address = createIPv4Address(ip,8080); // Create address structure for the server

    struct timeval timeout = {5, 0}; // Set timeout to 5 seconds for both send and receive operations
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* Check socket creation */
    if (client_socket < 0) {
        perror("socket");
        return 1;
    }

    /* Attempt to connect and abort on failure */
    if (connectToServer(client_socket, address) != 0) {
        perror("connectToServer");
        close(client_socket);
        return 1;
    }




    char *line = NULL;
    size_t lineSize = 0;
    printf("Type message)\n");
   
    while (true) {
        ssize_t charCount = getline(&line, &lineSize, stdin);
        if (charCount <= 0) {
            if (charCount == -1) perror("getline");
            break;
        }

        if (strcmp(line, "exit\n") == 0) {
            break;
        }

        ssize_t amountWasSent = send(client_socket, line, (size_t)charCount, 0);
        if (amountWasSent < 0) {
            perror("send");
            break;
        }
    }


        





    int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);

    if (bytes > 0)
        printf("Response from server:\n%s\n", buffer);

    close(client_socket); 
    free(line);
}