/*
 * server.c
 * --------
 * Entry point and main accept loop for the multi-client chat server.
 * For each accepted TCP connection the server spawns a dedicated pthread
 * that runs handle_client() — reading messages and routing them until the
 * client disconnects or sends /quit.
 *
 * Startup sequence:
 *   1. Parse optional port from argv[1] (defaults to DEFAULT_PORT).
 *   2. Initialise the async logger.
 *   3. Create, bind, and listen on the server socket.
 *   4. Install SIGINT/SIGTERM handlers for graceful shutdown.
 *   5. Accept loop: accept() -> registry_add() -> pthread_create().
 *   6. On shutdown: join all threads, close server fd, shutdown logger.
 *
 * Dependencies: common.h, utils.h, registry.h, router.h, logger.h,
 *               <stdio.h>, <stdlib.h>, <string.h>, <unistd.h>,
 *               <signal.h>, <pthread.h>, <sys/socket.h>, <netinet/in.h>
 */

#include "common.h"
#include "utils.h"
#include "registry.h"
#include "router.h"
#include "logger.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * server_running
 * --------------
 * Volatile flag checked by the accept loop. Set to 0 by the signal handler
 * to break the loop and trigger graceful shutdown.
 */
static volatile int server_running = 1;

/*
 * handle_signal(sig)
 * ------------------
 * Signal handler for SIGINT and SIGTERM. Sets server_running to 0 so the
 * accept loop exits on the next iteration. Does not call non-async-safe
 * functions (no printf, no malloc) as required by POSIX.
 */
static void handle_signal(int sig) {
    (void)sig;
    server_running = 0;
}

/*
 * client_thread_arg
 * -----------------
 * Small struct used to pass the accepted fd into the client thread without
 * relying on shared state or casting an int through a void pointer.
 */
typedef struct {
    int fd;
} client_thread_arg;

/*
 * handle_client(arg)
 * ------------------
 * Body of each per-client pthread. Receives messages from the client in a
 * loop and passes each one to route_message(). Exits and cleans up when:
 *   - recv() returns 0 (client closed the connection), or
 *   - recv() returns -1 (socket error), or
 *   - route_message() returns 1 (client sent /quit).
 *
 * Frees the arg struct, removes the client from the registry, and closes
 * the socket before the thread exits.
 */
static void *handle_client(void *arg) {
    client_thread_arg *cta = (client_thread_arg *)arg;
    int fd = cta->fd;
    free(cta);

    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            /* Client disconnected or socket error — clean up and exit. */
            char nick[MAX_NICK_LEN];
            registry_get_nick(fd, nick, MAX_NICK_LEN);
            logger_log("%s disconnected (connection closed)", nick);
            break;
        }

        buffer[bytes] = '\0';

        /* route_message returns 1 when the client sent /quit. */
        if (route_message(fd, buffer) == 1) {
            break;
        }
    }

    registry_remove(fd);
    close(fd);
    return NULL;
}

/*
 * main(argc, argv)
 * ----------------
 * Server entry point. Reads an optional port from argv[1], sets up the
 * listening socket, and runs the accept loop until a signal is received.
 */
int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
    }

    /* Initialise async logger before any other module logs anything. */
    logger_init();
    logger_log("Server starting on port %d", port);

    /* Create and configure the listening socket. */
    int server_fd = createTCPIpv4Socket();
    if (server_fd < 0) {
        fprintf(stderr, "Failed to create server socket.\n");
        return 1;
    }

    /*
     * SO_REUSEADDR allows the server to restart immediately after a crash or
     * deliberate shutdown without waiting for the OS TIME_WAIT period (~60s).
     */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = createIPv4Address("0.0.0.0", port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    /* Install signal handlers for graceful shutdown on Ctrl+C / SIGTERM. */
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    logger_log("Listening for connections (max %d clients)", MAX_CLIENTS);
    printf("[Server] Chat server running on port %d. Press Ctrl+C to stop.\n",
           port);

    /* Accept loop — runs until server_running is cleared by a signal. */
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            /* accept() will be interrupted by the signal; that's expected. */
            if (!server_running) break;
            perror("accept");
            continue;
        }

        /* Allocate thread arg — freed inside handle_client(). */
        client_thread_arg *cta = malloc(sizeof(client_thread_arg));
        if (!cta) {
            fprintf(stderr, "malloc failed, rejecting client.\n");
            close(client_fd);
            continue;
        }
        cta->fd = client_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, cta) != 0) {
            perror("pthread_create");
            free(cta);
            close(client_fd);
            continue;
        }

        /*
         * Detach the thread so its resources are freed automatically when it
         * exits. We do not need to join per-client threads — logger_shutdown()
         * waits until all logging is done, and the registry tracks activity.
         */
        pthread_detach(tid);

        /* Register after detach so the thread is already running. */
        if (registry_add(client_fd, tid) < 0) {
            logger_log("Registry full — rejected incoming connection");
            close(client_fd);
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        logger_log("New client connected from %s (fd=%d)", ip_str, client_fd);

        /* Welcome message sent directly to the new client. */
        const char *welcome =
            "[Server] Welcome to the chat! Type /help for commands.\n";
        send(client_fd, welcome, strlen(welcome), MSG_NOSIGNAL);
    }

    logger_log("Server shutting down");
    close(server_fd);
    logger_shutdown();

    printf("[Server] Shutdown complete.\n");
    return 0;
}
