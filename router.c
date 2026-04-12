/*
 * router.c
 * --------
 * Implements the message routing and command parsing logic. When a client
 * sends data, handle_client() in server.c passes the raw string here.
 * This module formats the output message, queries the registry for
 * sender/recipient information, and calls send() on the appropriate fds.
 *
 * Supported commands:
 *   /nick <name>       — Change the caller's display nickname.
 *   /msg  <nick> <text>— Send a private message to one client.
 *   /quit              — Signal the server to close this connection.
 *   /help              — Send the command list back to the caller only.
 *
 * Dependencies: router.h, registry.h, common.h, logger.h,
 *               <stdio.h>, <string.h>, <sys/socket.h>, <time.h>
 */

#include "router.h"
#include "registry.h"
#include "logger.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* Internal helpers                                                     */
/* ------------------------------------------------------------------ */

/*
 * send_to(fd, msg)
 * ----------------
 * Sends a null-terminated string to a single client socket. Uses MSG_NOSIGNAL
 * so a broken pipe on one client does not kill the entire server process.
 * Silently drops the message if send() fails (client likely disconnected).
 */
static void send_to(int fd, const char *msg) {
    send(fd, msg, strlen(msg), MSG_NOSIGNAL);
}

/*
 * format_message(out, out_len, sender_nick, body)
 * ------------------------------------------------
 * Formats a chat line into out as: "[HH:MM:SS] <sender_nick>: body\n"
 * Reads the current wall-clock time via time() + localtime().
 * out must point to a buffer of at least out_len bytes.
 */
static void format_message(char *out, int out_len,
                            const char *sender_nick, const char *body) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(out, out_len, "[%02d:%02d:%02d] %s: %s\n",
             t->tm_hour, t->tm_min, t->tm_sec, sender_nick, body);
}

/*
 * broadcast(sender_fd, formatted_msg)
 * ------------------------------------
 * Sends formatted_msg to every active client except the sender.
 * Takes a snapshot of the registry so the mutex is not held during send().
 */
static void broadcast(int sender_fd, const char *formatted_msg) {
    Client snapshot[MAX_CLIENTS];
    int count = 0;
    registry_get_all(snapshot, &count);

    for (int i = 0; i < count; i++) {
        if (snapshot[i].fd != sender_fd) {
            send_to(snapshot[i].fd, formatted_msg);
        }
    }
}

/*
 * send_private(sender_fd, target_nick, body)
 * -------------------------------------------
 * Looks up target_nick in the registry. If found, formats a DM line
 * "[HH:MM:SS] [PM from <sender>]: body\n" and sends it to the target fd.
 * Sends an error notice back to the sender if the nick is not found.
 */
static void send_private(int sender_fd, const char *target_nick,
                          const char *body) {
    int target_fd = registry_find_by_nick(target_nick);
    if (target_fd == -1) {
        char notice[BUFFER_SIZE];
        snprintf(notice, sizeof(notice),
                 "[Server] No user named '%s' is connected.\n", target_nick);
        send_to(sender_fd, notice);
        return;
    }

    char sender_nick[MAX_NICK_LEN];
    registry_get_nick(sender_fd, sender_nick, MAX_NICK_LEN);

    /* Extra headroom: timestamp (~12) + "[PM from/to " + nick + "]: " + body. */
    char line[BUFFER_SIZE + MAX_NICK_LEN + 64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    snprintf(line, sizeof(line), "[%02d:%02d:%02d] [PM from %s]: %s\n",
             t->tm_hour, t->tm_min, t->tm_sec, sender_nick, body);

    send_to(target_fd, line);

    /* Echo a confirmation back to the sender so they know it was delivered. */
    char confirm[BUFFER_SIZE + MAX_NICK_LEN + 64];
    snprintf(confirm, sizeof(confirm),
             "[%02d:%02d:%02d] [PM to %s]: %s\n",
             t->tm_hour, t->tm_min, t->tm_sec, target_nick, body);
    send_to(sender_fd, confirm);
}

/* ------------------------------------------------------------------ */
/* Command handlers                                                     */
/* ------------------------------------------------------------------ */

/*
 * handle_nick(sender_fd, args)
 * ----------------------------
 * Parses the first whitespace-delimited token from args as the new nickname.
 * Rejects empty or overly long nicks. Notifies the sender and broadcasts
 * the rename event to all other clients.
 */
static void handle_nick(int sender_fd, const char *args) {
    char new_nick[MAX_NICK_LEN];
    if (sscanf(args, "%31s", new_nick) != 1) {
        send_to(sender_fd, "[Server] Usage: /nick <name>\n");
        return;
    }

    char old_nick[MAX_NICK_LEN];
    registry_get_nick(sender_fd, old_nick, MAX_NICK_LEN);
    registry_set_nick(sender_fd, new_nick);

    char notice[BUFFER_SIZE];
    snprintf(notice, sizeof(notice),
             "[Server] %s is now known as %s\n", old_nick, new_nick);
    broadcast(sender_fd, notice);
    send_to(sender_fd, notice);

    logger_log("Nick change: %s -> %s", old_nick, new_nick);
}

/*
 * handle_msg(sender_fd, args)
 * ----------------------------
 * Parses args as "<target_nick> <message text>" and routes a private message.
 * Sends a usage hint back to the caller if the format is wrong.
 */
static void handle_msg(int sender_fd, const char *args) {
    char target[MAX_NICK_LEN];
    char body[BUFFER_SIZE];

    /* sscanf reads the first word as target and the rest as body. */
    if (sscanf(args, "%31s %2047[^\n]", target, body) < 2) {
        send_to(sender_fd, "[Server] Usage: /msg <nick> <message>\n");
        return;
    }

    send_private(sender_fd, target, body);
}

/*
 * handle_help(sender_fd)
 * ----------------------
 * Sends the list of supported commands back to the requesting client only.
 * No registry interaction needed; just a direct write to the sender's fd.
 */
static void handle_help(int sender_fd) {
    const char *help =
        "[Server] Available commands:\n"
        "  /nick <name>         - Change your display nickname\n"
        "  /msg  <nick> <text>  - Send a private message\n"
        "  /quit                - Disconnect from the server\n"
        "  /help                - Show this help message\n";
    send_to(sender_fd, help);
}

/* ------------------------------------------------------------------ */
/* Public entry point                                                   */
/* ------------------------------------------------------------------ */

/*
 * route_message(sender_fd, raw)
 * ------------------------------
 * Strips a trailing newline from raw, then dispatches based on whether the
 * first character is '/'. Command strings are split into a verb and arguments.
 * Plain messages are formatted and broadcast to all other clients.
 *
 * Returns 1 if the client sent /quit (caller should close the connection),
 * 0 otherwise.
 */
int route_message(int sender_fd, const char *raw) {
    /* Strip trailing newline/carriage-return so it doesn't appear in output. */
    char buf[BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "%s", raw);
    buf[strcspn(buf, "\r\n")] = '\0';

    if (buf[0] == '\0') {
        return 0;  /* ignore empty messages */
    }

    if (buf[0] == '/') {
        /* Split "/verb rest" into verb and args. */
        char verb[32] = {0};
        char args[BUFFER_SIZE] = {0};
        sscanf(buf + 1, "%31s %2047[^\n]", verb, args);

        if (strcmp(verb, "nick") == 0) {
            handle_nick(sender_fd, args);
        } else if (strcmp(verb, "msg") == 0) {
            handle_msg(sender_fd, args);
        } else if (strcmp(verb, "quit") == 0) {
            char nick[MAX_NICK_LEN];
            registry_get_nick(sender_fd, nick, MAX_NICK_LEN);
            char notice[BUFFER_SIZE];
            snprintf(notice, sizeof(notice),
                     "[Server] %s has left the chat.\n", nick);
            broadcast(sender_fd, notice);
            logger_log("%s disconnected (/quit)", nick);
            return 1;  /* signal caller to close connection */
        } else if (strcmp(verb, "help") == 0) {
            handle_help(sender_fd);
        } else {
            char err[BUFFER_SIZE];
            snprintf(err, sizeof(err),
                     "[Server] Unknown command '/%s'. Type /help for help.\n",
                     verb);
            send_to(sender_fd, err);
        }
    } else {
        /* Plain chat message — format and broadcast. */
        char nick[MAX_NICK_LEN];
        registry_get_nick(sender_fd, nick, MAX_NICK_LEN);

        char line[BUFFER_SIZE + MAX_NICK_LEN + 32];
        format_message(line, sizeof(line), nick, buf);
        broadcast(sender_fd, line);

        logger_log("<%s> %s", nick, buf);
    }

    return 0;
}
