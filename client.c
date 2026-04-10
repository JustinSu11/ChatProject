#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>


int main(){
    // Create a socket with the following parameters: Domain: AF_INET (IPv4), 
    // Type: SOCK_STREAM (TCP), Protocol: 0 (default)
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

    char* ip = "1.1.1.1"; // IP address for testing purposes

    struct timeval timeout;
    timeout.tv_sec = 3;   // 3-second timeout
    timeout.tv_usec = 0;

    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


    // create a socket address structure to specify the server's address and port
    struct sockaddr_in address;

    address.sin_family = AF_INET; // IPv4
    address.sin_port = htons(80); // Convert the port number to network byte order
    inet_pton(AF_INET, ip, &address.sin_addr.s_addr); // Convert the server's IP address from text to binary form




    int result = connect(client_socket, (struct sockaddr*)&address, sizeof(address));
    if (result != 0) {
        printf("Connection failed.\n");
        close(client_socket);
        return 0;   
    }

    printf("Connected to server at %s:80\n", ip);



    char* message = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
    send(client_socket, message, strlen(message), 0);

    char buffer[1024] = {0};
    int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);

    if (bytes > 0)
        printf("Response from server:\n%s\n", buffer);
    else
        printf("No response received.\n");

    close(client_socket); 

    return 0;
}