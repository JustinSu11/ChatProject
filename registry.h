/*
 * registry.h
 * ----------
 * Public interface for the client registry — a thread-safe table that tracks
 * every currently connected client. The server uses this to look up clients
 * by file descriptor or nickname and to iterate over all active connections
 * when broadcasting messages.
 *
 * All functions in this module are thread-safe (internally mutex-protected).
 * Callers do not need to acquire any lock before calling these functions.
 *
 * Dependencies: common.h, <pthread.h>
 */

#ifndef REGISTRY_H
#define REGISTRY_H

#include "common.h"
#include <pthread.h>

/*
 * Client
 * ------
 * Represents a single connected client. One slot in the registry is allocated
 * per accepted connection and freed when the client disconnects or sends /quit.
 *
 * Fields:
 *   fd     - The accepted socket file descriptor for this client.
 *   nick   - The client's current display nickname.
 *   thread - The pthread handling this client's recv loop.
 *   active - Non-zero if the slot is in use; 0 means the slot is free.
 */
typedef struct {
    int       fd;
    char      nick[MAX_NICK_LEN];
    pthread_t thread;
    int       active;
} Client;

/*
 * registry_add(fd)
 * ----------------
 * Registers a newly accepted client with the given socket file descriptor.
 * Assigns a default nickname of the form "Guest_N" where N is the slot index.
 * Returns 0 on success, -1 if the registry is full (MAX_CLIENTS reached).
 */
int registry_add(int fd, pthread_t thread);

/*
 * registry_remove(fd)
 * -------------------
 * Marks the slot belonging to the client with the given fd as inactive,
 * effectively deregistering them. Safe to call from any thread.
 */
void registry_remove(int fd);

/*
 * registry_set_nick(fd, nick)
 * ---------------------------
 * Updates the nickname for the client identified by fd.
 * Truncates to MAX_NICK_LEN - 1 characters if necessary.
 * Does nothing if no active client with that fd is found.
 */
void registry_set_nick(int fd, const char *nick);

/*
 * registry_get_nick(fd, out_nick, len)
 * ------------------------------------
 * Copies the current nickname of the client with the given fd into out_nick.
 * out_nick must point to a buffer of at least len bytes.
 * Returns 0 on success, -1 if the client is not found.
 */
int registry_get_nick(int fd, char *out_nick, int len);

/*
 * registry_find_by_nick(nick)
 * ---------------------------
 * Returns the socket fd of the first active client whose nickname matches nick
 * (case-sensitive). Returns -1 if no match is found.
 */
int registry_find_by_nick(const char *nick);

/*
 * registry_get_all(out, count)
 * ----------------------------
 * Fills the out array with a snapshot of all currently active Client structs.
 * Sets *count to the number of entries written. out must have room for at
 * least MAX_CLIENTS elements. The snapshot is taken under the mutex so it is
 * consistent, but the data may become stale immediately after the call returns.
 */
void registry_get_all(Client *out, int *count);

#endif /* REGISTRY_H */
