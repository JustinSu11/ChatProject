/*
 * utils.h
 * -------
 * Public interface for the socket utility helpers in utils.c.
 * Include this header in any file that needs to create sockets or
 * build address structures.
 *
 * Memory contract for createIPv4Address:
 *   - Server path (bind): the returned struct is used by value — no heap
 *     allocation, no free() needed.
 *   - Client path (connectToServer): the pointer variant in client.c
 *     allocates on the heap and the caller must free() it. See client.c.
 *
 * Dependencies: <netinet/in.h>
 */

#ifndef UTILS_H
#define UTILS_H

#include <netinet/in.h>

/*
 * createIPv4Address(ip, port)
 * ---------------------------
 * Returns a sockaddr_in value initialised for the given IP string and port.
 * Pass "0.0.0.0" to bind to all interfaces; pass a specific IP to connect.
 */
struct sockaddr_in createIPv4Address(const char *ip, int port);

/*
 * createTCPIpv4Socket()
 * ----------------------
 * Creates and returns a TCP/IPv4 socket file descriptor.
 * Returns -1 on failure.
 */
int createTCPIpv4Socket(void);

/*
 * connectToServer(socket_fd, address)
 * ------------------------------------
 * Connects socket_fd to the server at *address.
 * Returns 0 on success, -1 on failure (socket is closed on failure).
 */
int connectToServer(int socket_fd, struct sockaddr_in *address);

#endif /* UTILS_H */
