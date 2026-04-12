/*
 * registry.c
 * ----------
 * Implements the thread-safe client registry. Internally, clients are stored
 * in a fixed-size array of Client structs (size MAX_CLIENTS). All public
 * functions acquire registry_mutex before touching the array and release it
 * before returning, ensuring that concurrent calls from multiple handler
 * threads cannot corrupt the state.
 *
 * Dependencies: registry.h, common.h, <stdio.h>, <string.h>, <pthread.h>
 */

#include "registry.h"
#include "common.h"

#include <stdio.h>
#include <string.h>

/* The registry: a flat array of client slots. Slots are reused after removal. */
static Client clients[MAX_CLIENTS];

/* Protects all reads and writes to the clients[] array. */
static pthread_mutex_t registry_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * registry_add(fd, thread)
 * ------------------------
 * Finds the first inactive slot in clients[], fills it with the new client's
 * fd and thread, assigns a default nick "Guest_N", and marks it active.
 * Returns 0 on success, -1 if every slot is already occupied.
 */
int registry_add(int fd, pthread_t thread) {
    pthread_mutex_lock(&registry_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd     = fd;
            clients[i].thread = thread;
            clients[i].active = 1;
            snprintf(clients[i].nick, MAX_NICK_LEN, "Guest_%d", i);
            pthread_mutex_unlock(&registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return -1;  /* registry full */
}

/*
 * registry_remove(fd)
 * -------------------
 * Locates the slot with the matching fd and clears its active flag.
 * The slot becomes available for the next registry_add call.
 * No-ops if no active client matches the given fd.
 */
void registry_remove(int fd) {
    pthread_mutex_lock(&registry_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd == fd) {
            clients[i].active = 0;
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
}

/*
 * registry_set_nick(fd, nick)
 * ---------------------------
 * Finds the active client with the given fd and overwrites their nick field.
 * Uses snprintf to prevent buffer overflow (truncates at MAX_NICK_LEN - 1).
 * Silently does nothing if the fd is not registered.
 */
void registry_set_nick(int fd, const char *nick) {
    pthread_mutex_lock(&registry_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd == fd) {
            snprintf(clients[i].nick, MAX_NICK_LEN, "%s", nick);
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
}

/*
 * registry_get_nick(fd, out_nick, len)
 * ------------------------------------
 * Copies the nickname of the active client with the given fd into out_nick.
 * Returns 0 on success, -1 if the client is not found. The copy is bounded
 * by len to prevent overflow in the caller's buffer.
 */
int registry_get_nick(int fd, char *out_nick, int len) {
    pthread_mutex_lock(&registry_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd == fd) {
            snprintf(out_nick, len, "%s", clients[i].nick);
            pthread_mutex_unlock(&registry_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return -1;
}

/*
 * registry_find_by_nick(nick)
 * ---------------------------
 * Iterates active slots looking for a case-sensitive nick match.
 * Returns the fd of the first match, or -1 if no active client has that nick.
 */
int registry_find_by_nick(const char *nick) {
    pthread_mutex_lock(&registry_mutex);

    int found_fd = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].nick, nick) == 0) {
            found_fd = clients[i].fd;
            break;
        }
    }

    pthread_mutex_unlock(&registry_mutex);
    return found_fd;
}

/*
 * registry_get_all(out, count)
 * ----------------------------
 * Takes a consistent snapshot of all active Client structs under the mutex
 * and writes them into out[]. Sets *count to the number of entries written.
 * out must point to an array with room for at least MAX_CLIENTS elements.
 */
void registry_get_all(Client *out, int *count) {
    pthread_mutex_lock(&registry_mutex);

    *count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            out[(*count)++] = clients[i];
        }
    }

    pthread_mutex_unlock(&registry_mutex);
}
