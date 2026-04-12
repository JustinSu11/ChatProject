/*
 * router.h
 * --------
 * Public interface for the message router. The router is the core of the
 * chat application's logic: it receives raw input from a client, decides
 * whether it is a command or a plain message, and dispatches it accordingly.
 *
 * The module depends on the registry to resolve nicknames and enumerate
 * active clients for broadcasting.
 *
 * Dependencies: common.h, registry.h
 */

#ifndef ROUTER_H
#define ROUTER_H

/*
 * route_message(sender_fd, raw)
 * ------------------------------
 * Entry point for all inbound data from a connected client. Receives the
 * raw null-terminated string the client sent and decides what to do with it:
 *   - If raw starts with '/', parse it as a command and call the appropriate
 *     handler (/nick, /msg, /quit, /help).
 *   - Otherwise, treat it as a plain chat message and broadcast to all
 *     other active clients.
 *
 * Returns 1 if the client issued /quit (caller should close the connection),
 * 0 otherwise.
 */
int route_message(int sender_fd, const char *raw);

#endif /* ROUTER_H */
