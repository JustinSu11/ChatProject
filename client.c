/*
 * client.c
 * --------
 * Interactive chat client. Connects to the chat server and runs two
 * concurrent activities:
 *   - A receive thread: continuously reads messages from the server and
 *     prints them to stdout.
 *   - The main thread: reads lines from stdin and sends them to the server.
 *
 * Usage:
 *   ./client [server_ip] [port]
 *   Defaults: 127.0.0.1, DEFAULT_PORT (8080)
 *
 * The client exits cleanly when:
 *   - The user types /quit (sends it to the server, then exits).
 *   - The server closes the connection (receive thread detects EOF).
 *   - stdin reaches EOF (Ctrl+D).
 *
 * Dependencies: common.h, utils.h, <stdio.h>, <stdlib.h>, <string.h>,
 *               <unistd.h>, <pthread.h>, <sys/socket.h>
 */

#include "common.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 * client_running
 * --------------
 * Shared flag between the main thread and the receive thread. Set to 0 by
 * either thread to signal the other to stop and exit cleanly.
 */
static volatile int client_running = 1;

/* Socket fd shared between threads — set once in main before threads start. */
static int sock_fd = -1;

/*
 * receive_thread_fn(arg)
 * ----------------------
 * Body of the background receive thread. Reads incoming data from the server
 * socket in a loop and writes each message directly to stdout. Exits when:
 *   - recv() returns 0 (server closed the connection), or
 *   - recv() returns -1 (socket error or client_running was set to 0 causing
 *     the socket to be closed by the main thread).
 * Sets client_running = 0 before exiting so the main thread also stops.
 */
static void *receive_thread_fn(void *arg) {
    (void)arg;
    char buffer[BUFFER_SIZE];

    while (client_running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            /* Server disconnected or socket closed. */
            if (client_running) {
                printf("\n[Client] Disconnected from server.\n");
            }
            client_running = 0;
            break;
        }

        buffer[bytes] = '\0';
        /* Print server message directly — it is already formatted. */
        printf("%s", buffer);
        fflush(stdout);
    }

    return NULL;
}

/*
 * main(argc, argv)
 * ----------------
 * Client entry point. Parses optional IP and port arguments, connects to the
 * server, spawns the receive thread, then runs the stdin send loop.
 */
int main(int argc, char *argv[]) {
    const char *server_ip = "127.0.0.1";
    int port = DEFAULT_PORT;

    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[2]);
            return 1;
        }
    }

    /* Create socket and connect using the utility helpers from utils.c. */
    sock_fd = createTCPIpv4Socket();
    if (sock_fd < 0) {
        fprintf(stderr, "Failed to create socket.\n");
        return 1;
    }

    struct sockaddr_in address = createIPv4Address(server_ip, port);
    if (connectToServer(sock_fd, &address) != 0) {
        return 1;
    }

    printf("[Client] Connected to %s:%d. Type /help for commands.\n",
           server_ip, port);

    /* Spawn the receive thread to print incoming server messages. */
    pthread_t recv_tid;
    if (pthread_create(&recv_tid, NULL, receive_thread_fn, NULL) != 0) {
        fprintf(stderr, "Failed to create receive thread.\n");
        close(sock_fd);
        return 1;
    }

    /*
     * Main thread: send loop.
     * Reads one line at a time from stdin and sends it to the server.
     * Exits on /quit, EOF (Ctrl+D), or if the receive thread signals
     * client_running = 0 (server disconnected).
     */
    char input[BUFFER_SIZE];
    while (client_running) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            /* EOF — treat like /quit. */
            client_running = 0;
            break;
        }

        /* Strip trailing newline for comparison, but keep it for the send. */
        char trimmed[BUFFER_SIZE];
        snprintf(trimmed, sizeof(trimmed), "%s", input);
        trimmed[strcspn(trimmed, "\r\n")] = '\0';

        if (strlen(trimmed) == 0) {
            continue;  /* Skip blank lines. */
        }

        /* Send raw input (including newline) to the server. */
        send(sock_fd, input, strlen(input), MSG_NOSIGNAL);

        /* Local /quit: send it first so the server can broadcast the goodbye,
         * then break so this side exits cleanly. */
        if (strcmp(trimmed, "/quit") == 0) {
            client_running = 0;
            break;
        }
    }

    /*
     * Shutdown: close the socket so the receive thread's recv() returns 0
     * and it exits on its own, then join it to avoid a resource leak.
     */
    close(sock_fd);
    pthread_join(recv_tid, NULL);

    printf("[Client] Goodbye.\n");
    return 0;
}
