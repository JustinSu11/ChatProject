/*
 * utils.c
 * -------
 * Low-level socket utility helpers shared by both server.c and client.c.
 * These functions wrap the POSIX socket API to reduce boilerplate: creating
 * a TCP socket, building a sockaddr_in address structure, and connecting to
 * a remote server.
 *
 * Memory note: createIPv4Address() allocates a sockaddr_in on the heap.
 * The caller is responsible for calling free() on the returned pointer once
 * they are done with it (typically after bind/connect).
 *
 * Dependencies: utils.h, <stdlib.h>, <arpa/inet.h>, <sys/socket.h>,
 *               <stdio.h>, <unistd.h>, <string.h>
 */

#include "utils.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

/*
 * createIPv4Address(ip, port)
 * ---------------------------
 * Allocates and returns a heap-allocated sockaddr_in configured for the
 * given IPv4 address string and port number. Uses inet_pton() to convert
 * the dotted-decimal IP string to binary form, and htons() to put the port
 * in network byte order.
 *
 * Pass "0.0.0.0" for ip to bind to all interfaces (server use).
 * Pass a specific IP such as "127.0.0.1" to target a single host (client use).
 *
 * Returns a pointer to the allocated struct. The caller must free() it.
 */
struct sockaddr_in createIPv4Address(const char *ip, int port) {
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);
    return address;
}

/*
 * createTCPIpv4Socket()
 * ----------------------
 * Creates a POSIX TCP socket suitable for IPv4 communication.
 * Equivalent to: socket(AF_INET, SOCK_STREAM, 0)
 *
 * Returns the socket file descriptor on success, or -1 on failure
 * (in which case errno is set by socket()).
 */
int createTCPIpv4Socket(void) {
    return socket(AF_INET, SOCK_STREAM, 0);
}

/*
 * connectToServer(socket_fd, address)
 * ------------------------------------
 * Attempts to connect socket_fd to the server described by address.
 * Prints a success or failure message to stdout. Closes the socket and
 * returns -1 on failure so the caller can detect and handle the error.
 *
 * Returns 0 on success, -1 on failure.
 */
int connectToServer(int socket_fd, struct sockaddr_in *address) {
    int result = connect(socket_fd, (struct sockaddr *)address,
                         sizeof(*address));
    if (result == 0) {
        printf("Connection successful.\n");
    } else {
        perror("connect");
        close(socket_fd);
    }
    return result;
}
